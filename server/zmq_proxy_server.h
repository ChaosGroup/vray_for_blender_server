#ifndef _ZMQ_PROXY_SERVER_H_
#define _ZMQ_PROXY_SERVER_H_

#include <string>
#include <thread>
#include <unordered_map>

#include "renderer_controller.h"

static const uint64_t DISCONNECT_TIMEOUT = 5000;

class ZmqProxyServer {
public:
	ZmqProxyServer(const std::string & port);
	~ZmqProxyServer();
	void start();
	void stop();

private:
	uint64_t getFreeWorkerId();
	
	bool isWorker(uint64_t id) const;
	bool isClient(uint64_t id) const;

	void mainLoop();
private:
	bool isRunning;
	std::thread workerThread;
	std::string port;

	uint64_t nextWorkerId;

	std::unordered_map<uint64_t, uint64_t> clientToWorker, workerToClient;
	struct WorkerWrapper {
		std::shared_ptr<RendererController> worker;
		std::chrono::time_point<std::chrono::high_resolution_clock> lastKeepAlive;
	};
	std::unordered_map<uint64_t, WorkerWrapper> workers;

	std::unique_ptr<zmq::context_t> context;
	std::unique_ptr<zmq::socket_t> routerSocket;
};




#endif // _ZMQ_PROXY_SERVER_H_
