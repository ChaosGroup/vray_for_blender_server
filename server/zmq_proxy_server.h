#ifndef _ZMQ_PROXY_SERVER_H_
#define _ZMQ_PROXY_SERVER_H_

#include <string>
#include <thread>
#include <unordered_map>
#include <queue>
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
#include <set>

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
		std::unique_ptr<RendererController> worker; ///< Pointer to the RendererController
		time_point                          lastKeepAlive; ///< Last time we got message from this client
		client_id_t                         id; ///< The associated client ID, used for message routing
		ClientType                          clientType; ///< Either heartbeat or exporter

		WorkerWrapper(const WorkerWrapper &) = delete;
		WorkerWrapper & operator=(const WorkerWrapper &) = delete;
		WorkerWrapper(WorkerWrapper && o): worker(std::move(o.worker)) {
			lastKeepAlive = o.lastKeepAlive;
			id = o.id;
			clientType = o.clientType;
		}

		WorkerWrapper(std::unique_ptr<RendererController> worker, time_point lastKeepAlive, client_id_t id, ClientType clType);
	};

public:
	/// Create new server
	/// @port - the listening port
	/// @appsdkPath - full path to the appsdk that will be passet to VRay::Init
	/// @showVFB - flag passed to VRay::Init to enable/disable UI
	/// @checkHeartbeat - if true server will remain active until there are heartbeat clients and shutdown if all disconnect
	ZmqProxyServer(const std::string &port, bool showVFB = false, bool checkHeartbeat = true);

	/// Starts serving requests until there are active clients (heartbeat or exporter)
	void run();
private:
	/// Get an option from a socket
	/// @socket - the socket to read option from
	/// @option - the option value/name (eg. ZMQ_RCVMORE)
	/// @return - pair of int and bool
	///           int - the returned option value
	///           bool - flag set to true if exception occured
	std::pair<int, bool> checkSocketOpt(zmq::socket_t & socket, int option) const;

	/// Create a Renderer for a given client
	/// @clientId - the ID of the client
	/// @now - current time
	void addWorker(client_id_t clientId, time_point now, ClientType type);

	/// Use Logger::log to print stats, does nothing if called more often that once a second
	/// @now - current time
	/// @return - true if actually printed
	bool reportStats(time_point now);

	/// Check if all heartbeat clients are inactive
	/// Used to stop server if @checkHeartbeat is true and this returns true
	/// @now - current time
	/// @return - true if there are no heartbeat clients
	bool checkForTimeouts(time_point now);

	/// Thread base for reaper thread
	void reaperThreadBase();
private:
	const bool  checkHeartbeat; ///< If true server stops itself if there are no active clients
	bool        showVFB; ///< Flag for appsdk UI
	std::string port; ///< Listening port

	std::unordered_map<client_id_t, WorkerWrapper> workers; ///< Map of all active clients
	zmq::context_t context; ///< The ZMQ context
	std::set<client_id_t> stoppedClients; ///< List of all clients, for which a RendererController has been deleted

	time_point lastDataCheck; ///< Last time @reportStats did work
	time_point lastTimeoutCheck; ///< Last time @checkForTimeout did work
	time_point lastHeartbeat; ///< Last time a heartbeat client sent data
	uint64_t dataTransfered; ///< Total bytes send and receieved

	std::vector<WorkerWrapper> deadRenderers; ///< Free-ing renderer object is slow, so we dont do it on main thread and instead push here
	std::thread reaperThread; ///< Thread that will free any WorkerWrappers put inside @deadRenderers
	std::condition_variable reaperCond; ///< Cond var to signal thread that will reap renderers
	std::mutex reaperMtx; ///< Mutex protecting @deadRenderers
	volatile bool reaperRunning; ///< Flag to stop repaer thread
};

#endif // _ZMQ_PROXY_SERVER_H_
