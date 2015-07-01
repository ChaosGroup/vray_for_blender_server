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
	ArgvSettings settings;
	if (!parseArgv(settings, argc, argv)) {
		std::cerr << "Arguments:\n"
			<< "-p <port-num>\tPort number to listen on\n"
			<< "-vfb\tSet show VFB option\n";
		return 0;
	}

	try {
		ZmqProxyServer server(settings.port);
		server.start();

		while (server.good()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	} catch (std::exception & e) {
		std::cerr << e.what() << std::endl;
	} catch (VRay::VRayException & e) {
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
