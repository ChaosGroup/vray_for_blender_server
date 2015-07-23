#ifndef _ZMQ_PROXY_SERVER_H_
#define _ZMQ_PROXY_SERVER_H_

#include <string>
#include <thread>
#include <unordered_map>

#include "renderer_controller.h"

class ZmqProxyServer {
public:
	ZmqProxyServer(const std::string & port, bool showVFB = false);

	void run();

private:
	bool showVFB;
	std::string port;

	struct WorkerWrapper {
		std::shared_ptr<RendererController> worker;
		std::chrono::time_point<std::chrono::high_resolution_clock> lastKeepAlive;
		uint64_t id;
	};
	std::unordered_map<uint64_t, WorkerWrapper> workers;

	std::unique_ptr<zmq::context_t> context;
	std::unique_ptr<zmq::socket_t> routerSocket;

	std::unique_ptr<VRay::VRayInit> vray;
};




#endif // _ZMQ_PROXY_SERVER_H_
