#include "zmq_proxy_server.h"
#include "utils/logger.h"
#include "utils/version.h"
#include <string>

struct ArgvSettings {
	std::string port;
	bool showVFB;
};

bool parseArgv(ArgvSettings & settings, int argc, char * argv[]) {
	for (int c = 1; c < argc; ++c) {
		if (!strcmp(argv[c], "-p") && c + 1 < argc) {
			settings.port = argv[++c];
		} else if (!strcmp(argv[c], "-vfb")) {
			settings.showVFB = true;
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
	puts("-vfb\tSet show VFB option");
}


int main(int argc, char *argv[]) {
	Logger::getInstance().setCallback([](Logger::Level lvl, const std::string & msg) {
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
			//printf("INFO: %s\n", msg.c_str());
			break;
		}
	});

	ArgvSettings settings = {"", false};
	if (!parseArgv(settings, argc, argv)) {
		printHelp();
		return 0;
	}

	printInfo();
	printf("Starting VRayZmqServer on all interfaces with port %s, showing VFB: %s\n\n", settings.port, (settings.showVFB ? "true" : "false"));

	try {
		ZmqProxyServer server(settings.port, settings.showVFB);
		server.run();
	} catch (std::exception & e) {
		Logger::getInstance().log(Logger::Error, e.what());
	} catch (VRay::VRayException & e) {
		Logger::getInstance().log(Logger::Error, e.what());
	}

	return 0;
}
