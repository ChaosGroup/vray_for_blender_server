#define VRAY_RUNTIME_LOAD_SECONDARY
#include "zmq_proxy_server.h"
#include "utils/logger.h"
#include <chrono>
#include <random>
#include <exception>

using namespace std;
using namespace std::chrono;
using namespace zmq;

ZmqProxyServer::ZmqProxyServer(const string & port, const char *appsdkPath, bool showVFB, bool checkHeartbeat)
    : port(port)
    , context(nullptr)
    , routerSocket(nullptr)
    , vray(new VRay::VRayInit(appsdkPath))
    , dataTransfered(0)
    , showVFB(showVFB)
	, checkHeartbeat(checkHeartbeat)
{
	if (!vray) {
		throw logic_error("Failed to instantiate vray!");
	}
}

void ZmqProxyServer::dispatcherThread() {
	while (true) {
		if (!dispatcherQ.empty()) {
			unique_lock<mutex> lock(dispatchQMutex);
			if (dispatcherQ.empty()) {
				continue;
			}
			auto item = move(dispatcherQ.front());
			dispatcherQ.pop();

			// explicitly unlock at this point to allow main thread to use the Q sooner
			lock.unlock();

			lock_guard<mutex> workerLock(workersMutex);
			auto worker = this->workers.find(item.first);
			if (worker != this->workers.end()) {
				if (worker->second.clientType != ClientType::Exporter) {
					Logger::log(Logger::Error, "Message from non exporter client");
				}

				auto beforeCall = chrono::high_resolution_clock::now();
				worker->second.worker->handle(VRayMessage(item.second));
				worker->second.appsdkWorkTimeMs += chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - beforeCall).count();
			}
		} else {
			this_thread::sleep_for(chrono::milliseconds(1));
		}
	}
}

void ZmqProxyServer::addWorker(client_id_t clientId, time_point now) {
	auto sendFn = [this, clientId](VRayMessage && msg) {
		lock_guard<mutex> l(this->sendQMutex);
		this->sendQ.emplace(make_pair(clientId, move(msg)));
	};

	WorkerWrapper wrapper = {
		shared_ptr<RendererController>(new RendererController(sendFn, showVFB)),
		now, clientId, 0, ClientType::Exporter
	};

	auto res = workers.emplace(make_pair(clientId, wrapper));
	assert(res.second && "Failed to add worker!");
	Logger::log(Logger::Debug, "New client (", clientId, ") connected - spawning renderer.");
}

uint64_t ZmqProxyServer::sendOutMessages() {
	if (sendQ.empty()) {
		return 0;
	}

	uint64_t transferred = 0;

	lock_guard<mutex> l(sendQMutex);
	int maxSend = 10;
	while (sendQ.size() && --maxSend) {
		auto & p = sendQ.front();
		transferred += p.second.getMessage().size();

		message_t id(sizeof(client_id_t));
		memcpy(id.data(), &p.first, sizeof(client_id_t));
		routerSocket->send(id, ZMQ_SNDMORE);
		routerSocket->send("", 0, ZMQ_SNDMORE);
		routerSocket->send(p.second.getMessage());

		sendQ.pop();
	}

	return transferred;
}

void ZmqProxyServer::clearMessagesForClient(const client_id_t & cl) {
	lock_guard<mutex> qLock(dispatchQMutex);

	auto size = dispatcherQ.size();
	for (auto c = size; c < size; c++) {
		auto item = move(dispatcherQ.front());
		dispatcherQ.pop();

		if (dispatcherQ.front().first != cl) {
			dispatcherQ.emplace(move(item));
		} else {
			--size;
		}
	}
}

bool ZmqProxyServer::checkForTimeout(time_point now) {
	if (duration_cast<milliseconds>(now - lastTimeoutCheck).count() <= 100 || workers.empty()) {
		return false;
	}

	lastTimeoutCheck = now;
	for (auto workerIter = workers.begin(), end = workers.end(); workerIter != end; /*nop*/) {
		auto inactiveTime = duration_cast<milliseconds>(now - workerIter->second.lastKeepAlive).count();
		uint64_t max_timeout = 0;

		switch (workerIter->second.clientType) {
		case ClientType::Exporter:
			max_timeout = EXPORTER_TIMEOUT;
			break;
		case ClientType::Heartbeat:
			max_timeout = HEARBEAT_TIMEOUT;
			break;
		default:
			max_timeout = 0;
		}

		if (inactiveTime > max_timeout) {
			Logger::log(Logger::Debug, "Client (", workerIter->first, ") timed out - stopping it's renderer.");

			// filter messages intended for the selected client
			clearMessagesForClient(workerIter->first);

			{
				lock_guard<mutex> workerLock(workersMutex);
				workerIter = workers.erase(workerIter);
			}
		} else {
			++workerIter;
		}
	}

	Logger::log(Logger::Debug, "Active renderers:", workers.size());
	return true;
}

bool ZmqProxyServer::checkForHeartbeat(time_point now) {
	auto active_heartbeats = count_if(workers.begin(), workers.end(), [](const pair<client_id_t, WorkerWrapper> & worker) {
		return worker.second.clientType == ClientType::Heartbeat;
	});

	if (active_heartbeats != 0) {
		lastHeartbeat = now;
		return false;
	} else {
		return duration_cast<milliseconds>(now - lastHeartbeat).count() > HEARBEAT_TIMEOUT;
	}
}

bool ZmqProxyServer::initZmq() {
	try {
		context = unique_ptr<context_t>(new context_t(1));
		routerSocket = unique_ptr<socket_t>(new socket_t(*context, ZMQ_ROUTER));

		Logger::log(Logger::Debug, "Binding to tcp://*:", port);

		routerSocket->bind((string("tcp://*:") + port).c_str());
	} catch (zmq::error_t & e) {
		Logger::log(Logger::Error, "ZMQ exception during init:", e.what());
		return false;
	}
	return true;
}

bool ZmqProxyServer::reportStats(time_point now) {
	auto dataReportDiff = duration_cast<milliseconds>(now - lastDataCheck).count();
	if (dataReportDiff <= 1000) {
		return false;
	}
	lastDataCheck = now;
	if (dataTransfered / 1024 > 1) {
		Logger::log(Logger::Debug, "Data transfered", dataTransfered / 1024., "KB for", dataReportDiff, "ms");
		dataTransfered = 0;
	}

	for (const auto & worker : workers) {
		Logger::log(Logger::Debug, "Client (", worker.second.id, ") appsdk time:", worker.second.appsdkWorkTimeMs, "ms");
	}

	return true;
}

void ZmqProxyServer::run() {
	auto dispacther = thread(&ZmqProxyServer::dispatcherThread, this);
	auto stopDispatcher = [](thread *th) {
		th->detach();
	};

	unique_ptr<thread, decltype(stopDispatcher)> wrapper(&dispacther, stopDispatcher);

	if (!initZmq()) {
		return;
	}

	lastTimeoutCheck = high_resolution_clock::now();
	lastDataCheck = high_resolution_clock::now();

	try {
		while (true) {
			bool didWork = false;
			auto now = high_resolution_clock::now();

			didWork = didWork || reportStats(now);
			didWork = didWork || checkForTimeout(now);

			if (checkForHeartbeat(now)) {
				if (checkHeartbeat) {
					Logger::log(Logger::Error, "No active blender instaces for more than", HEARBEAT_TIMEOUT, "ms Shutting down");
					break;
				}
			}

			if (auto transfer = sendOutMessages()) {
				didWork = true;
				dataTransfered += transfer;
			}

			message_t identity;
			if (!routerSocket->recv(&identity, ZMQ_NOBLOCK)) {
				if (!didWork) {
					this_thread::sleep_for(chrono::milliseconds(1));
				}
				continue;
			}

			message_t e, payload;
			routerSocket->recv(&e);
			routerSocket->recv(&payload);
			dataTransfered += payload.size();

			assert(!e.size() && "No empty frame!");
			assert(payload.size() && "Unexpected empty frame");

			const client_id_t messageIdentity = *reinterpret_cast<client_id_t*>(identity.data());

			uint64_t receiverId = 0;

			auto worker = workers.find(messageIdentity);

			// add worker for new client
			if (worker == workers.end()) {
				addWorker(messageIdentity, now);
				worker = workers.find(messageIdentity);
				if (worker == workers.end()) {
					Logger::log(Logger::Error, "Failed to create worker for client (", messageIdentity, ")");
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}


			if (payload.size() == sizeof(ClientType)) {
				const ClientType * type = reinterpret_cast<ClientType *>(payload.data());
				worker->second.clientType = *type;
				if (*type != ClientType::Heartbeat && *type != ClientType::Exporter) {
					Logger::log(Logger::Error, "Unexpected client type", static_cast<int>(*type), "Disconnecting client", worker->first);
					clearMessagesForClient(worker->first);
					workers.erase(worker);
				} else {
					// respond to heartbeat
					lock_guard<mutex> l(this->sendQMutex);
					VRayMessage msg(sizeof(ClientType));
					memcpy(msg.getMessage().data(), type, sizeof(ClientType));
					this->sendQ.emplace(make_pair(messageIdentity, move(msg)));
				}
			} else {
				lock_guard<mutex> l(dispatchQMutex);
				dispatcherQ.push(make_pair(messageIdentity, move(payload)));
			}

			worker->second.lastKeepAlive = high_resolution_clock::now();
		}
	} catch (zmq::error_t & e) {
		Logger::log(Logger::Error, "Zmq exception in server: ", e.what());
	}

	Logger::log(Logger::Debug, "Server stopping all renderers.");

	workers.clear();
	routerSocket.release();
	context.release();
}
