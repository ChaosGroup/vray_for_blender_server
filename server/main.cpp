#define VRAY_RUNTIME_LOAD_PRIMARY
#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <windows.h>
#else
	#include <dlfcn.h>
#endif

#include <vraysdk.hpp>

#include "zmq_proxy_server.h"
#include "utils/logger.h"
#include "utils/version.h"
#include <string>

#include <qapplication.h>

struct ArgvSettings {
	ArgvSettings()
	    : port("")
	    , showVFB(false)
	    , checkHearbeat(true)
	    , logLevel(Logger::Warning)
	{}
	std::string port;
	bool showVFB;
	bool checkHearbeat;
	Logger::Level logLevel;
};

bool parseArgv(ArgvSettings & settings, int argc, char * argv[]) {
	for (int c = 1; c < argc; ++c) {
		if (!strcmp(argv[c], "-p") && c + 1 < argc) {
			settings.port = argv[++c];
		} else if (!strcmp(argv[c], "-vfb")) {
			settings.showVFB = true;
		} else if (!strcmp(argv[c], "-log") && c + 1 < argc) {
			std::stringstream strm(argv[++c]);
			int lvl;
			if (strm >> lvl) {
				settings.logLevel = static_cast<Logger::Level>((lvl > Logger::None ? Logger::None : lvl < Logger::Info ? Logger::Info : lvl));
			}
		} else if (!strcmp(argv[c], "-noHeartbeat")) {
			settings.checkHearbeat = false;
		} else {
			return false;
		}
	}
	return settings.port != "";
}

void printInfo() {
	puts("VRayZmqServer");
	printf("Version %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
	puts("");
}

void printHelp() {
	printInfo();
	puts("Arguments:");
	puts("-p <port-num>\tPort number to listen on");
	puts("-vfb\t\tSet show VFB option");
	puts("-log <level>\t1-4, 1 beeing the most verbose");
}


int main(int argc, char *argv[]) {
	ArgvSettings settings;
	if (!parseArgv(settings, argc, argv)) {
		printHelp();
		return 0;
	}

	Logger::getInstance().setCallback([&settings](Logger::Level lvl, const std::string & msg) {
		if (lvl >= settings.logLevel - 1) {
			switch (lvl) {
			case Logger::Debug:
				printf("ZMQ_DEBUG: %s\n", msg.c_str());
				break;
			case Logger::Warning:
				printf("ZMQ_WARNING: %s\n", msg.c_str());
				break;
			case Logger::Error:
				printf("ZMQ_ERROR: %s\n", msg.c_str());
				break;
			case Logger::Info:
				printf("ZMQ_INFO: %s\n", msg.c_str());
				break;
			case Logger::None:
			default:
				// nothing
				break;
			}
		}
	});

#ifdef _WIN32
	const char * os_pathsep = ";";
#else
	const char * os_pathsep = ":";
#endif
	const char * sdkPathName = "VRAY_ZMQSERVER_APPSDK_PATH";

	const char * path = std::getenv(sdkPathName);
	if (!path) {
		printf("Undefined %s, will try to load appsdk from LD_LIBRARY_PATH OR PATH\n", sdkPathName);
	} else {
		// lets make sure we load all vray stuff from the same place
		std::string vrayPath(path);
		vrayPath = vrayPath.substr(0, vrayPath.find_last_of("/\\"));
		Logger::log(Logger::Info, "Setting VRAY_PATH to", vrayPath);
		// putenv requires valid memory until exit or variable unset
		static char vrayPathBuf[1024] = {0, };
		strncpy(vrayPathBuf, ("VRAY_PATH=" + vrayPath).c_str(), 1024);
		putenv(vrayPathBuf);

		auto userPath = getenv("PATH");
		static char pathBuf[1024] = {0, };
		strncpy(pathBuf, ("PATH=" + vrayPath + os_pathsep + std::string(userPath ? userPath : "")).c_str(), 1024);
		putenv(pathBuf);
		Logger::log(Logger::Info, "New PATH", pathBuf);
	}

	printInfo();
	printf("Starting VRayZmqServer on all interfaces with port %s, showing VFB: %s, log level %d\n\nLoading appsdk: %s\n\n",
		settings.port.c_str(), (settings.showVFB ? "true" : "false"), settings.logLevel, path);

	VRay::VRayInit init(path);

	int retCode = 0;
	try {
		int argc = 0;
		char *argv[1] = { nullptr };
		QApplication qapp(argc, argv);

		ZmqProxyServer server(settings.port, path, settings.showVFB, settings.checkHearbeat);
		std::thread serverRunner(&ZmqProxyServer::run, &server);

		retCode = qapp.exec();

		if (serverRunner.joinable()) {
			Logger::log(Logger::Debug, "Joining server thread.");
			serverRunner.join();
		}

	} catch (std::exception & e) {
		Logger::getInstance().log(Logger::Error, e.what());
	} catch (VRay::VRayException & e) {
		Logger::getInstance().log(Logger::Error, e.what());
	}

	Logger::log(Logger::Debug, "Main thread stopping.");
	return retCode;
}
