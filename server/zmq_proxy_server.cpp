#include "zmq_proxy_server.h"
#include "utils/logger.h"
#include <chrono>
#include <random>
#include <exception>

using namespace std;


ZmqProxyServer::ZmqProxyServer(const string & port, bool showVFB)
	: port(port), context(nullptr), routerSocket(nullptr),
	vray(new VRay::VRayInit(true)), showVFB(showVFB) {
	if (!vray) {
		throw logic_error("Failed to instantiate vray!");
	}
}


#ifdef VRAY_ZMQ_SINGLE_MODE
#if 0
void ZmqProxyServer::run() {
	try {
		uint64_t dummyId = 1;

		WorkerWrapper wrapper = {
			shared_ptr<RendererController>(new RendererController("tcp://*:" + port, dummyId, showVFB)),
			std::chrono::high_resolution_clock::now()
		};

		workers.emplace(make_pair(dummyId, wrapper));
	} catch (zmq::error_t & e) {
		std::cerr << e.what() << std::endl;
		isOk = isRunning = false;
		return;
	}

	isOk = isRunning = true;
}
#endif
#else // VRAY_ZMQ_SINGLE_MODE
void ZmqProxyServer::dispatcherThread(queue<pair<uint64_t, zmq::message_t>> &que, mutex &mtx) {
	while (true) {
		if (!que.empty()) {
			unique_lock<mutex> lock(mtx);
			auto item = move(que.front());
			que.pop();

			// explicitly unlock at this point to allow main thread to use the Q sooner
			lock.unlock();

			auto worker = this->workers.find(item.first);
			if (worker != this->workers.end()) {
				worker->second.worker->handle(VRayMessage(item.second));
			}
		}
	}
}

void ZmqProxyServer::run() {
	using namespace zmq;
	using namespace std::chrono;


	queue<pair<uint64_t, VRayMessage>> sendQ;
	queue<pair<uint64_t, message_t>> dispatcherQ;
	mutex sendQMutex, dispatchQMutex;

	auto dispacther = thread(&ZmqProxyServer::dispatcherThread, this, ref(dispatcherQ), ref(dispatchQMutex));

	try {
		context = unique_ptr<context_t>(new context_t(1));
		routerSocket = unique_ptr<socket_t>(new socket_t(*context, ZMQ_ROUTER));

		Logger::log(Logger::Debug, "Binding to tcp://*:", port);

		routerSocket->bind((string("tcp://*:") + port).c_str());
	} catch (zmq::error_t & e) {
		Logger::log(Logger::Error, "ZMQ exception during init:", e.what());
		return;
	}
	
	auto lastTimeoutCheck = high_resolution_clock::now();
	auto lastDataCheck = high_resolution_clock::now();

	try {
		uint64_t dataTransfered = 0;
		while (true) {
			auto now = high_resolution_clock::now();

			auto dataReportDiff = duration_cast<milliseconds>(now - lastDataCheck).count();
			if (dataReportDiff > 1000) {
				lastDataCheck = now;
				if (dataTransfered > 0) {
					Logger::log(Logger::Info, "Data transfered", dataTransfered / 1024., "KB for", dataReportDiff, "ms");
					dataTransfered = 0;
				}
			}

#ifdef VRAY_ZMQ_PING
			if (duration_cast<milliseconds>(now - lastTimeoutCheck).count() > DISCONNECT_TIMEOUT && workers.size()) {
				lastTimeoutCheck = now;

				for (auto workerIter = workers.begin(), end = workers.end(); workerIter != end; /*nop*/) {
					auto inactiveTime = duration_cast<milliseconds>(now - workerIter->second.lastKeepAlive).count();
					if (inactiveTime > DISCONNECT_TIMEOUT) {
						Logger::log(Logger::Debug, "Client (", workerIter->first, ") timed out - stopping it's renderer.");
						workerIter = workers.erase(workerIter);
					} else {
						++workerIter;
					}
				}

				Logger::log(Logger::Debug, "Active renderers:", workers.size());
			}
#endif // VRAY_ZMQ_PING

			if (sendQ.size()) {
				lock_guard<mutex> l(sendQMutex);
				int maxSend = 10;
				while (sendQ.size() && --maxSend) {
					auto & p = sendQ.front();
					dataTransfered += p.second.getMessage().size();

					message_t id(sizeof(uint64_t));
					memcpy(id.data(), &p.first, sizeof(uint64_t));
					routerSocket->send(id, ZMQ_SNDMORE);
					routerSocket->send("", 0, ZMQ_SNDMORE);
					routerSocket->send(p.second.getMessage());

					sendQ.pop();
				}
			}

			message_t identity;
			if (!routerSocket->recv(&identity, ZMQ_NOBLOCK)) {
				continue;
			}

			message_t e, payload;
			routerSocket->recv(&e);
			routerSocket->recv(&payload);
			dataTransfered += payload.size();

			assert(!e.size() && "No empty frame!");
			assert(payload.size() && "Unexpected empty frame");

			const uint64_t messageIdentity = *reinterpret_cast<uint64_t*>(identity.data());

			uint64_t receiverId = 0;

			auto worker = workers.find(messageIdentity);

			// add worker for new client
			if (worker == workers.end()) {
				auto sendFn = [&sendQ, &sendQMutex, messageIdentity](VRayMessage && msg) {
					lock_guard<mutex> l(sendQMutex);
					sendQ.emplace(make_pair(messageIdentity, move(msg)));
				};

				WorkerWrapper wrapper = {
					shared_ptr<RendererController>(new RendererController(sendFn, showVFB)),
					now, messageIdentity
				};

				auto res = workers.emplace(make_pair(messageIdentity, wrapper));
				assert(res.second && "Failed to add worker!");
				worker = res.first;
				Logger::log(Logger::Debug, "New client (", messageIdentity, ") connected - spawning renderer.");
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}

			if (payload.size() > 1) {
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
#endif // VRAY_ZMQ_SINGLE_MODE
