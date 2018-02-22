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
	    , showProfileLog(false)
	    , logLevel(Logger::Warning)
	{}
	std::string port;
	bool showVFB;
	bool checkHearbeat;
	bool dumpInfoLog;
	bool showProfileLog;
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
				switch (lvl) {
				case 1:
					settings.logLevel = Logger::Info;
					break;
				case 2:
					settings.logLevel = Logger::Debug;
					break;
				case 3:
					settings.logLevel = Logger::Warning;
					break;
				case 4:
					/* fall-trough */
				default:
					settings.logLevel = Logger::Error;
					break;
				}
			}
		} else if (!strcmp(argv[c], "-noHeartbeat")) {
			settings.checkHearbeat = false;
		} else if (!strcmp(argv[c], "-dumpInfoLog")) {
			settings.dumpInfoLog = true;
		} else if (!strcmp(argv[c], "-showProfile")) {
			settings.showProfileLog = true;
		} else {
			return false;
		}
	}
	return settings.port != "";
}

void printInfo() {
	puts("VRayZmqServer");
	printf("Version %d.%d:%d\n", VERSION_MAJOR, VERSION_MINOR, ZMQ_PROTOCOL_VERSION);
}

void printHelp() {
	printInfo();
	puts("Arguments:");
	puts("-p <port-num>\tPort number to listen on");
	puts("-vfb\t\tSet show VFB option");
	puts("-log <level>\t1-4, 1 = Info, 2 = Debug, 3 = Warning, 4 = Error");
}

/// Parse command line arguments, initialize logger, initialize server and start it
int main(int argc, char *argv[]) {
	ArgvSettings settings;
	if (!parseArgv(settings, argc, argv)) {
		printHelp();
		return 0;
	}
	std::ofstream infoDump;

	bool firstLog = true;
	auto filterLevel = settings.logLevel;
	if (settings.dumpInfoLog) {
		settings.logLevel = Logger::APIDump;
		infoDump.open("dumpInfoLog.txt", std::ios::trunc | std::ios::ate);
	}
	Logger::getInstance().setCurrentlevel(settings.logLevel);

	const auto begin = std::chrono::high_resolution_clock::now();
	auto lastProfileTime = begin;
	auto lastLogTime = begin;

	Logger::getInstance().setCallback([filterLevel, &settings, &infoDump, &lastLogTime, &firstLog, &lastProfileTime] (Logger::Level lvl, const std::string & msg) {
		using namespace std;
		using namespace std::chrono;
		if (filterLevel <= Logger::Info && lvl == Logger::Info) {
			printf("ZMQ_INFO: %s\n", msg.c_str());
		} else if (filterLevel <= Logger::Debug && lvl == Logger::Debug) {
			printf("ZMQ_DEBUG: %s\n", msg.c_str());
		} else if (filterLevel <= Logger::Warning && lvl == Logger::Warning) {
			printf("ZMQ_WARNING: %s\n", msg.c_str());
		} else if (filterLevel <= Logger::Error && lvl == Logger::Error) {
			printf("ZMQ_ERROR: %s\n", msg.c_str());
		} else if (lvl == Logger::APIDump && settings.dumpInfoLog) {
			if (settings.dumpInfoLog && infoDump) {
				if (!firstLog) {
					const auto now = high_resolution_clock::now();
					const auto sleepTime = duration_cast<milliseconds>(now - lastLogTime).count();
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
		} else if (lvl == Logger::Profile && settings.showProfileLog) {
			const auto now = high_resolution_clock::now();
			const auto passed = now - lastProfileTime;
			lastProfileTime = now;
			const int minutes = duration_cast<chrono::minutes>(passed).count() % 60;
			const int seconds = duration_cast<chrono::seconds>(passed).count() % 60;
			const int milliseconds = duration_cast<chrono::milliseconds>(passed).count() % 1000;
			printf("[%2d:%2d:%4d]\t%s\n", minutes, seconds, milliseconds, msg.c_str());
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
#ifdef _WIN32
		_putenv(vrayPathBuf);
#else
		putenv(vrayPathBuf);
#endif
		auto userPath = getenv("PATH");
		const int pathSize = 4096;
		static char pathBuf[pathSize] = {0, };
		const auto pathValue = "PATH=" + vrayPath + os_pathsep + std::string(userPath ? userPath : "");
		strncpy(pathBuf, pathValue.c_str(), pathSize);
#ifdef _WIN32
		_putenv(pathBuf);
#else
		putenv(pathBuf);
#endif
		Logger::log(Logger::Debug, "New PATH", pathBuf);
	}

	printInfo();
	printf("Starting VRayZmqServer on all interfaces with port %s, showing VFB: %s, log level %d\nLoading appsdk: %s\n",
		settings.port.c_str(), (settings.showVFB ? "true" : "false"), settings.logLevel, path);

	int retCode = 0;
	try {
		VRay::VRayInit init(path, settings.showVFB);

		printf("AppSDK version: %s\n", VRay::getSDKVersion().toString().c_str());
		printf("V-Ray version: %s\n\n", VRay::getVRayVersion().toString().c_str());

		int argc = 0;
		char *argv[1] = { nullptr };
		QApplication qapp(argc, argv);

		ZmqProxyServer server(settings.port, settings.showVFB, settings.checkHearbeat);
		std::thread serverRunner(&ZmqProxyServer::run, &server);

		// blocks until qApp->quit() is called
		retCode = qapp.exec();

		if (serverRunner.joinable()) {
			Logger::log(Logger::Debug, "Joining server thread.");
			serverRunner.join();
		}

	} catch (std::exception & e) {
		Logger::log(Logger::Error, e.what());
	} catch (VRay::VRayException & e) {
		Logger::log(Logger::Error, e.what());
	}

	Logger::log(Logger::Debug, "Main thread stopping.");
	return retCode;
}
