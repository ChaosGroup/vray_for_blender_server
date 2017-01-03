#define VRAY_RUNTIME_LOAD_PRIMARY
#ifdef _WIN32
	#define _CRT_SECURE_NO_WARNINGS
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
#include <fstream>
#include <chrono>

#include <qapplication.h>

struct ArgvSettings {
	ArgvSettings()
	    : port("")
	    , showVFB(false)
	    , checkHearbeat(true)
	    , dumpInfoLog(false)
	    , logLevel(Logger::Warning)
	{}
	std::string port;
	bool showVFB;
	bool checkHearbeat;
	bool dumpInfoLog;
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
		} else if (!strcmp(argv[c], "-dumpInfoLog")) {
			settings.dumpInfoLog = true;
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

/// Parse command line arguments, initialize logger, initialize server and start it
int main(int argc, char *argv[]) {
	ArgvSettings settings;
	if (!parseArgv(settings, argc, argv)) {
		printHelp();
		return 0;
	}
	std::ofstream infoDump;
	auto lastLogTime = std::chrono::high_resolution_clock::now();
	bool firstLog = true;
	if (settings.dumpInfoLog) {
		settings.logLevel = Logger::Info;
		infoDump.open("dumpInfoLog.txt", std::ios::trunc | std::ios::ate);
	}
	Logger::getInstance().setCurrentlevel(settings.logLevel);


	Logger::getInstance().setCallback([&settings, &infoDump, &lastLogTime, &firstLog] (Logger::Level lvl, const std::string & msg) {
		using namespace std::chrono;
		if (lvl == Logger::Info && settings.dumpInfoLog && infoDump) {
			if (!firstLog) {
				auto now = high_resolution_clock::now();
				auto sleepTime = duration_cast<milliseconds>(now - lastLogTime).count();
				if (sleepTime > 10) {
					infoDump << "Sleep(" << sleepTime << ");\n";
				}
				lastLogTime = now;
			}
			firstLog = false;
			infoDump.write(msg.c_str(), msg.length());
			infoDump.write("\n", 1);
			infoDump.flush();
		}
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
				if (!settings.dumpInfoLog) {
					// only print info to console if it is not dumped in file
					printf("ZMQ_INFO: %s\n", msg.c_str());
				}
				break;
			case Logger::None:
			default:
				// nothing
				break;
			}
		}
	});

	// fix paths in order to load correct appsdk with matching vray and plugins
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
		Logger::log(Logger::Debug, "Setting VRAY_PATH to", vrayPath);
		// putenv requires valid memory until exit or variable unset
		static char vrayPathBuf[1024] = {0, };
		strncpy(vrayPathBuf, ("VRAY_PATH=" + vrayPath).c_str(), 1024);
		putenv(vrayPathBuf);

		auto userPath = getenv("PATH");
		const int pathSize = 4096;
		static char pathBuf[pathSize] = {0, };
		const auto pathValue = "PATH=" + vrayPath + os_pathsep + std::string(userPath ? userPath : "");
		strncpy(pathBuf, pathValue.c_str(), pathSize);
		putenv(pathBuf);
		Logger::log(Logger::Debug, "New PATH", pathBuf);
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

		// blocks until qApp->quit() is called
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
