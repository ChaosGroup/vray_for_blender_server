#include "zmq_proxy_server.h"
#include <chrono>
#include <random>
#include <exception>

using namespace std;


ZmqProxyServer::ZmqProxyServer(const string & port)
	: port(port), isRunning(false), context(nullptr), routerSocket(nullptr),
	nextWorkerId(1), vray(new VRay::VRayInit(true)), isOk(true) {
	if (!vray) {
		throw logic_error("Failed to instantiate vray!");
	}
}

ZmqProxyServer::~ZmqProxyServer() {
	stop();
}

void ZmqProxyServer::start() {
	isRunning = true;
	workerThread = thread(&ZmqProxyServer::mainLoop, this);
}

void ZmqProxyServer::stop() {
	isRunning = false;
	if (workerThread.joinable()) {
		workerThread.join();
	}
}

void ZmqProxyServer::mainLoop() {
	using namespace zmq;
	using namespace std::chrono;

	try {
		context = unique_ptr<context_t>(new context_t(1));
		routerSocket = unique_ptr<socket_t>(new socket_t(*context, ZMQ_ROUTER));

		int opt = 1, sopt = sizeof(int);
		routerSocket->setsockopt(ZMQ_ROUTER_MANDATORY, &opt, sopt);
		routerSocket->bind((string("tcp://*:") + port).c_str());
	} catch (error_t & e) {
		std::cerr << e.what() << std::endl;
		isOk = isRunning = false;
		return;
	}
	
	auto lastTimeoutCheck = high_resolution_clock::now();

	try {
		while (isRunning) {
			message_t identity;

			auto now = high_resolution_clock::now();
			if (duration_cast<milliseconds>(now - lastTimeoutCheck).count() > DISCONNECT_TIMEOUT) {
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

			if (!routerSocket->recv(&identity, ZMQ_NOBLOCK)) {
				continue;
			}

			const uint64_t & messageIdentity = *reinterpret_cast<uint64_t*>(identity.data());

			uint64_t receiverId = 0;
			bool isClientMessage = isClient(messageIdentity);

			if (isClientMessage) {
				receiverId = clientToWorker[messageIdentity];
				workers[receiverId].lastKeepAlive = now;
			} else if (isWorker(messageIdentity)) {
				receiverId = workerToClient[messageIdentity];
			} else {
				uint64_t workerId = getFreeWorkerId();
				const uint64_t & clientId = messageIdentity;

				clientToWorker[clientId] = workerId;
				workerToClient[workerId] = clientId;

				WorkerWrapper wrapper = {
					shared_ptr<RendererController>(new RendererController("tcp://localhost:" + port, workerId)),
					high_resolution_clock::now()
				};

				workers.emplace(make_pair(workerId, wrapper));
				this_thread::sleep_for(milliseconds(100));

				receiverId = workerId;
			}

			message_t receiverIdentity(sizeof(receiverId));
			*reinterpret_cast<uint64_t*>(receiverIdentity.data()) = receiverId;

			message_t payload;
			routerSocket->recv(&payload);


			try {
				routerSocket->send(receiverIdentity, ZMQ_SNDMORE);
				routerSocket->send(payload);
			} catch (zmq::error_t & e) {
				if (isClientMessage) {
					workers.erase(workers.find(receiverId));
				} else {
					throw;
				}
				continue;
			}
		}
	} catch (error_t & e) {
		std::cerr << e.what() << std::endl;
		isOk = isRunning = false;
	}
}


uint64_t ZmqProxyServer::getFreeWorkerId() {
	uint64_t id;
	do {
		id = ++nextWorkerId;
	} while (isWorker(id) || isClient(id));
	return id;
}

bool ZmqProxyServer::isWorker(uint64_t id) const {
	return workerToClient.find(id) != workerToClient.end();
}

bool ZmqProxyServer::isClient(uint64_t id) const {
	return clientToWorker.find(id) != clientToWorker.end();
}
