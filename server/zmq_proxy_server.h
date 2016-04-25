#ifndef _ZMQ_PROXY_SERVER_H_
#define _ZMQ_PROXY_SERVER_H_

#include <string>
#include <thread>
#include <unordered_map>
#include <ostream>

#include "renderer_controller.h"

// minimal implemetation required for the project
class client_id_t {
public:
	client_id_t(uint64_t id): m_id(id) {}

	operator const uint64_t() const {
		return m_id;
	}

private:
	uint64_t m_id;
};

inline std::ostream & operator<<(std::ostream & strm, const client_id_t & id) {
	return strm << (uint64_t)id % 1000;
}

namespace std {
	template <>
	struct hash<client_id_t> {
		size_t operator()(const client_id_t & el) const {
			return el;
		}
	};
}

class ZmqProxyServer {
	typedef std::chrono::high_resolution_clock::time_point time_point;

	// Wrapper struct for the renderer object
	struct WorkerWrapper {
		std::shared_ptr<RendererController> worker;
		time_point lastKeepAlive;
		client_id_t id;
		uint64_t appsdkWorkTimeMs;
	};

public:
	ZmqProxyServer(const std::string &port, const char *appsdkPath, bool showVFB = false);

	void run();

private:

	void dispatcherThread();

	void addWorker(client_id_t clientId, time_point now);
	uint64_t sendOutMessages();
	bool initZmq();
	bool reportStats(time_point now);
#ifdef VRAY_ZMQ_PING
	bool checkForTimeout(time_point now);
#endif // VRAY_ZMQ_PING

private:
	bool showVFB;
	std::string port;

	// active workers
	std::unordered_map<client_id_t, WorkerWrapper> workers;
	std::mutex workersMutex;

	std::unique_ptr<zmq::context_t> context;
	std::unique_ptr<zmq::socket_t> routerSocket;

	std::unique_ptr<VRay::VRayInit> vray;

	// message queue for sending
	std::queue<std::pair<client_id_t, VRayMessage>> sendQ;
	// message queue for receieveing messages
	std::queue<std::pair<client_id_t, zmq::message_t>> dispatcherQ;
	std::mutex sendQMutex, dispatchQMutex;


	time_point lastDataCheck, lastTimeoutCheck;
	uint64_t dataTransfered;
};

#endif // _ZMQ_PROXY_SERVER_H_
