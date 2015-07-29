#include "zmq_proxy_server.h"
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


int main(int argc, char *argv[]) {
	ArgvSettings settings = {"", false};
	if (!parseArgv(settings, argc, argv)) {
		puts("Arguments:");
		puts("-p <port-num>\tPort number to listen on");
		puts("-vfb\tSet show VFB option");
		return 0;
	}

	try {
		ZmqProxyServer server(settings.port, settings.showVFB);
		server.run();
	} catch (std::exception & e) {
		puts(e.what());
	} catch (VRay::VRayException & e) {
		puts(e.what());
	}

	return 0;
}
