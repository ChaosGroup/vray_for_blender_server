#include "zmq_wrapper.h"
#include "zmq.hpp"
#include <chrono>
#include <condition_variable>

ZmqWrapper::ZmqWrapper() :
	context(new zmq::context_t(1)), frontend(nullptr),
	isWorking(true), worker(nullptr), isInit(false) {

	bool socketInit = false;
	std::condition_variable threadReady;
	std::mutex threadMutex;


	this->worker = new std::thread([this, &threadReady, &socketInit, &threadMutex] {
		{
			std::lock_guard<std::mutex> lock(threadMutex);

			this->frontend = std::unique_ptr<zmq::socket_t>(new zmq::socket_t(*(this->context), ZMQ_DEALER));

			int timeOut = 100;
			this->frontend->setsockopt(ZMQ_RCVTIMEO, &timeOut, sizeof(timeOut));
			timeOut *= 10;
			this->frontend->setsockopt(ZMQ_SNDTIMEO, &timeOut, sizeof(timeOut));
			socketInit = true;
		}
		threadReady.notify_all();

		while (!this->isInit) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		while (this->isWorking) {
			zmq::message_t incoming;

			if (this->messageQue.size() && this->frontend->connected()) {
				std::lock_guard<std::mutex> lock(this->messageMutex);
				while (this->messageQue.size()) {
					if (!this->frontend->send(this->messageQue.front().getMessage())) {
						break;
					}
					this->messageQue.pop();
				}
			}

			if (this->frontend->recv(&incoming)) {
				this->callback(VRayMessage(incoming), this);
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		this->frontend->close();
	});



	{
		std::unique_lock<std::mutex> lock(threadMutex);
		// wait for the thread to finish initing the socket, else bind & connect might be called before init
		threadReady.wait(lock, [&socketInit] { return socketInit; });
	}
}

ZmqWrapper::~ZmqWrapper() {
	if (this->worker) {
		this->isWorking = false;

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
	std::lock_guard<std::mutex> lock(this->messageMutex);
	this->messageQue.push(std::move(message));
}

void ZmqWrapper::send(void * data, int size) {
	VRayMessage msg(size);
	memcpy(msg.getMessage().data(), data, size);

	std::lock_guard<std::mutex> lock(this->messageMutex);
	this->messageQue.push(std::move(msg));
}

void ZmqClient::connect(const char * addr) {
	this->frontend->connect(addr);

	this->isInit = true;
}

void ZmqServer::bind(const char * addr) {
	this->frontend->bind(addr);

	this->isInit = true;
}
