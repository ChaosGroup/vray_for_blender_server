#define VRAY_RUNTIME_LOAD_SECONDARY
#include "zmq_proxy_server.h"
#include "utils/logger.h"
#include "utils/version.h"
#include <string>

struct ArgvSettings {
	std::string port;
	bool showVFB;
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
				settings.logLevel = static_cast<Logger::Level>( (lvl > Logger::None ? Logger::None : lvl < Logger::Info ? Logger::Info : lvl) );
			}
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
	ArgvSettings settings = {"", false, Logger::Warning};
	if (!parseArgv(settings, argc, argv)) {
		printHelp();
		return 0;
	}

	Logger::getInstance().setCallback([&settings](Logger::Level lvl, const std::string & msg) {
		if (lvl >= settings.logLevel - 1) {
			switch (lvl) {
			case Logger::Debug:
				printf("DEBUG: %s\n", msg.c_str());
				break;
			case Logger::Warning:
				printf("WARNING: %s\n", msg.c_str());
				break;
			case Logger::Error:
				printf("ERROR: %s\n", msg.c_str());
				break;
			case Logger::Info:
				printf("INFO: %s\n", msg.c_str());
				break;
			}
		}
	});

	const char * sdkPathName = "VRAY_ZMQSERVER_APPSDK_PATH";

	const char * path = std::getenv(sdkPathName);
	if (!path) {
		const char * ldPath = std::getenv("LD_LIBRARY_PATH");
		printf("Undefined %s, will try to load appsdk from $LD_LIBRARY_PATH = \"%s\"\n", sdkPathName, ldPath ? ldPath : "");
		printHelp();
		return 0;
	}


	printInfo();
	printf("Starting VRayZmqServer on all interfaces with port %s, showing VFB: %s, log level %d\n\nLoading appsdk: %s\n\n",
		settings.port.c_str(), (settings.showVFB ? "true" : "false"), settings.logLevel, path);

	try {
		ZmqProxyServer server(settings.port, path, settings.showVFB);
		server.run();
	} catch (std::exception & e) {
		Logger::getInstance().log(Logger::Error, e.what());
	} catch (VRay::VRayException & e) {
		Logger::getInstance().log(Logger::Error, e.what());
	}

	return 0;
}
