#include "zmq_proxy_server.h"
#include <chrono>
#include <random>
#include <exception>
#include <iostream>

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
void ZmqProxyServer::run() {
	using namespace zmq;
	using namespace std::chrono;


	queue<pair<uint64_t, VRayMessage>> sendQ;
	mutex sendQMutex;

	try {
		context = unique_ptr<context_t>(new context_t(1));
		routerSocket = unique_ptr<socket_t>(new socket_t(*context, ZMQ_ROUTER));

		routerSocket->bind((string("tcp://*:") + port).c_str());
	} catch (zmq::error_t & e) {
		std::cerr << e.what() << std::endl;
		return;
	}
	
	auto lastTimeoutCheck = high_resolution_clock::now();

	try {
		while (true) {
			auto now = high_resolution_clock::now();

#ifdef VRAY_ZMQ_PING
			if (duration_cast<milliseconds>(now - lastTimeoutCheck).count() > DISCONNECT_TIMEOUT && workers.size()) {
				lastTimeoutCheck = now;
				cout << "Clients before check: " << workers.size() << endl;

				for (auto workerIter = workers.begin(), end = workers.end(); workerIter != end; /*nop*/) {
					auto inactiveTime = duration_cast<milliseconds>(now - workerIter->second.lastKeepAlive).count();
					if (inactiveTime > DISCONNECT_TIMEOUT) {
						workerIter = workers.erase(workerIter);
					} else {
						++workerIter;
					}
				}
				cout << "Clients after check: " << workers.size() << endl;
			}
#endif // VRAY_ZMQ_PING

			if (sendQ.size()) {
				unique_lock<mutex> l(sendQMutex);
				int maxSend = 10;
				while (sendQ.size() && --maxSend) {
					auto & p = sendQ.front();

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

			assert(!e.size() && "No empty frame!");
			assert(payload.size() && "Unexpected empty frame");

			const uint64_t messageIdentity = *reinterpret_cast<uint64_t*>(identity.data());

			uint64_t receiverId = 0;

			auto worker = workers.find(messageIdentity);

			// add worker for new client
			if (worker == workers.end()) {
				auto sendFn = [&sendQ, &sendQMutex, messageIdentity](VRayMessage && msg) {
					unique_lock<mutex> l(sendQMutex);
					sendQ.emplace(make_pair(messageIdentity, move(msg)));
				};

				WorkerWrapper wrapper = {
					shared_ptr<RendererController>(new RendererController(sendFn, showVFB)),
					now, messageIdentity
				};

				auto res = workers.emplace(make_pair(messageIdentity, wrapper));
				assert(res.second && "Failed to add worker!");
				worker = res.first;
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}

			if (payload.size() > 1) {
				VRayMessage pl(payload);
				worker->second.worker->handle(pl);
			}
			worker->second.lastKeepAlive = high_resolution_clock::now();
		}
	} catch (zmq::error_t & e) {
		std::cerr << e.what() << std::endl;
	}

	workers.clear();
	routerSocket.release();
	context.release();
}
#endif // VRAY_ZMQ_SINGLE_MODE
