#ifndef _ZMQ_PROXY_SERVER_H_
#define _ZMQ_PROXY_SERVER_H_

#include <string>
#include <thread>
#include <unordered_map>
#include <ostream>
#include <iomanip>

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <windows.h>
#else
	#include <dlfcn.h>
#endif
#include <vraysdk.hpp>
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
	return strm << std::setfill('0') << std::setw(3) << (uint64_t)id % 1000;
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
		ClientType clientType;
		uint64_t appsdkWorkTimeMs;
		uint64_t appsdkMaxTimeMs;

		WorkerWrapper(std::shared_ptr<RendererController> worker, time_point lastKeepAlive, client_id_t id, ClientType clType);
	};

public:
	ZmqProxyServer(const std::string &port, const char *appsdkPath, bool showVFB = false, bool checkHeartbeat = true);

	void run();

private:

	void dispatcherThread();

	void addWorker(client_id_t clientId, time_point now);
	uint64_t sendOutMessages();
	bool initZmq();
	bool reportStats(time_point now);

	// this will return true if there is no client for more than SHUTDOWN_TIMEOUT
	bool checkForHeartbeat(time_point now);
	bool checkForTimeout(time_point now);

	void clearMessagesForClient(const client_id_t & client);

private:
	const bool checkHeartbeat;
	bool showVFB;
	bool dispatcherRunning;
	std::string port;

	// active workers
	std::unordered_map<client_id_t, WorkerWrapper> workers;
	std::mutex workersMutex;

	std::unique_ptr<zmq::context_t> context;
	std::unique_ptr<zmq::socket_t> routerSocket;

	std::unique_ptr<VRay::VRayInit> vray;

	// message queue for sending
	std::deque<std::pair<client_id_t, VRayMessage>> sendQ;
	// message queue for receieveing messages
	std::queue<std::pair<client_id_t, zmq::message_t>> dispatcherQ;
	std::mutex sendQMutex, dispatchQMutex;


	time_point lastDataCheck;
	time_point lastTimeoutCheck;
	time_point lastHeartbeat;
	uint64_t dataTransfered;
};

#endif // _ZMQ_PROXY_SERVER_H_
