#include "zmq_wrapper.h"
#include "zmq.hpp"
#include <chrono>

ZmqWrapper::ZmqWrapper() :
	context(new zmq::context_t(1)),
	inproc(new zmq::socket_t(*context, ZMQ_PAIR)), frontend(nullptr),
	isWorking(true), worker(nullptr), isInit(false), setUp([]{}) {

	bool isBound = false;

	this->worker = new std::thread([this, &isBound] {
		zmq::socket_t proxy(*(this->context), ZMQ_PAIR);
		this->frontend = std::unique_ptr<zmq::socket_t>(new zmq::socket_t(*(this->context), ZMQ_DEALER));
		

		int timeOut = 100;
		proxy.setsockopt(ZMQ_RCVTIMEO, &timeOut, sizeof(timeOut));
		this->frontend->setsockopt(ZMQ_RCVTIMEO, &timeOut, sizeof(timeOut));
		
		proxy.bind("inproc://backend");
		isBound = true;

		while (!this->isInit) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		std::cout << "worker thread staring " << std::endl;

		while (this->isWorking) {
			zmq::message_t outgoing, incoming;

			// TODO: not copy all data
			if (proxy.recv(&outgoing)) {
				std::cout << "forwarding msg " << outgoing.size() << std::endl;
				this->frontend->send(outgoing);
				std::cout << " done" << std::endl;
			}

			if (this->frontend->recv(&incoming)) {
				std::cout << "got msg, calling callback " << incoming.size() << std::endl;
				this->callback(VRayMessage(incoming), this);
				std::cout << " .. done" << std::endl;
			}
		}

		this->frontend->close();
	});

	while(!isBound) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	this->inproc->connect("inproc://backend");
}

ZmqWrapper::~ZmqWrapper() {
	if (this->worker) {
		this->isWorking = false;
		this->inproc->close();

		if (this->worker->joinable()) {
			this->worker->join();
		}

		delete this->worker;
		this->worker = nullptr;
	}
}

void ZmqWrapper::setCallback(ZmqWrapperCallback_t cb) {
	this->callback = cb;
}

void ZmqWrapper::send(VRayMessage &message) {
	this->send(message.getMessage().data(), message.getMessage().size());
}

void ZmqWrapper::send(const void * data, int size) {
	std::cout << "Send to inproc " << size << std::endl;
	this->inproc->send(data, size);
}

void ZmqClient::connect(const char * addr) {
	this->frontend->connect(addr);
	std::cout << "connected " << addr << std::endl;

	this->isInit = true;
}

void ZmqServer::bind(const char * addr) {
	this->frontend->bind(addr);
	std::cout << "bound " << addr << std::endl;

	this->isInit = true;
}
