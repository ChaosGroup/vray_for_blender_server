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

/// Wrapper class over uint64_t to enable custom printing in Logger
/// Contains minimal implemetation required
class client_id_t {
public:
	client_id_t(uint64_t id): m_id(id) {}

	client_id_t() = default;

	client_id_t(const client_id_t &) = default;
	client_id_t & operator=(const client_id_t &) = default;

	client_id_t(client_id_t && o) {
		std::swap(m_id, o.m_id);
	}

	client_id_t & operator=(client_id_t && o) {
		std::swap(m_id, o.m_id);
		return *this;
	}

	operator const uint64_t() const {
		return m_id;
	}

private:
	uint64_t m_id;
};

/// Print only 3 digits of the client ID
inline std::ostream & operator<<(std::ostream & strm, const client_id_t & id) {
	return strm << std::setfill('0') << std::setw(3) << (uint64_t)id % 1000;
}

namespace std {
	/// Hash-er for the client ID wrapper
	template <>
	struct hash<client_id_t> {
		size_t operator()(const client_id_t & el) const {
			return el;
		}
	};
}

/// Server that manages clients and renderers
/// Instantiated a RendererController for each 'Renderer' and maintains heartbeat connection with
/// each instance of Blender that is running
class ZmqProxyServer {
	typedef std::chrono::high_resolution_clock::time_point time_point;

	/// Context for a singe Renderer
	struct WorkerWrapper {
		struct MessageInfo {
			VRayMessage::Type type;
			VRayMessage::PluginAction pAction;
			VRayMessage::RendererAction rAction;
		};

		std::unique_ptr<RendererController> worker; ///< Pointer to the RendererController
		time_point                          lastKeepAlive; ///< Last time we got message from this client
		client_id_t                         id; ///< The associated client ID, used for message routing
		ClientType                          clientType; ///< Either heartbeat or exporter
		uint64_t                            appsdkWorkTimeMs; ///< Total time this client spent waiting for appsdk calls
		uint64_t                            appsdkMaxTimeMs; ///< The slowest appsdk call
		MessageInfo                         msg; ///< Info for the slowest appsdk call

		WorkerWrapper(const WorkerWrapper &) = delete;
		WorkerWrapper & operator=(const WorkerWrapper &) = delete;
		WorkerWrapper(WorkerWrapper && o): worker(std::move(o.worker)) {
			lastKeepAlive = o.lastKeepAlive;
			id = o.id;
			clientType = o.clientType;
			appsdkWorkTimeMs = o.appsdkWorkTimeMs;
			appsdkMaxTimeMs = o.appsdkMaxTimeMs;
		}

		WorkerWrapper(std::unique_ptr<RendererController> worker, time_point lastKeepAlive, client_id_t id, ClientType clType);
	};

public:
	/// Create new server
	/// @port - the listening port
	/// @appsdkPath - full path to the appsdk that will be passet to VRay::Init
	/// @showVFB - flag passed to VRay::Init to enable/disable UI
	/// @checkHeartbeat - if true server will remain active until there are heartbeat clients and shutdown if all disconnect
	ZmqProxyServer(const std::string &port, const char *appsdkPath, bool showVFB = false, bool checkHeartbeat = true);

	/// Starts serving requests until there are active clients (heartbeat or exporter)
	void run();
private:

	/// Base function for thread that relays received messages to appropriate RendererController
	void dispatcherThread();

	/// Create a Renderer for a given client
	/// @clientId - the ID of the client
	/// @now - current time
	void addWorker(client_id_t clientId, time_point now);

	/// Send out queued messaged, but no more that maxSend
	/// @maxSend - upper limit of messages to send
	/// @return - bytes sent
	uint64_t sendOutMessages(int maxSend = 10);

	/// Initialize ZMQ
	/// @return - false on fail
	bool initZmq();

	/// Use Logger::log to print stats, does nothing if called more often that once a second
	/// @now - current time
	/// @return - true if actually printed
	bool reportStats(time_point now);

	/// Check if all heartbeat clients are inactive
	/// Used to stop server if @checkHeartbeat is true and this returns true
	/// @now - current time
	/// @return - true if there are no heartbeat clients
	bool checkForHeartbeat(time_point now);

	/// Check and disconnects any inactive clients
	/// @now - current time
	/// @return	- true if some work was done, false if called more frequent than 100ms or no clients in server
	bool checkForTimeout(time_point now);

	/// Clear all pending messages for a client
	/// @client - the ID of the client
	void clearMessagesForClient(const client_id_t & client);

private:
	const bool  checkHeartbeat; ///< If true server stops itself if there are no active clients
	bool        showVFB; ///< Flag for appsdk UI
	std::string port; ///< Listening port

	std::unordered_map<client_id_t, WorkerWrapper> workers; ///< Map of all active clients
	std::mutex workersMutex; ///< Lock for @workers

	std::unique_ptr<zmq::context_t> context; ///< The ZMQ context
	std::unique_ptr<zmq::socket_t> routerSocket; ///< The ZMQ socket

	std::deque<std::pair<client_id_t, VRayMessage>> sendQ; ///< Message queue for sending
	std::mutex sendQMutex; ///< Lock for @sendQ
	std::queue<std::pair<client_id_t, zmq::message_t>> dispatcherQ; ///< Message queue for receieved messages
	std::mutex dispatchQMutex; ///< Lock for @dispatcherQ
	bool dispatcherRunning; ///< Flag for dispatcher thread

	time_point lastDataCheck; ///< Last time @reportStats did work
	time_point lastTimeoutCheck; ///< Last time @checkForTimeout did work
	time_point lastHeartbeat; ///< Last time a heartbeat client sent data
	uint64_t dataTransfered; ///< Total bytes send and receieved
};

#endif // _ZMQ_PROXY_SERVER_H_
