#include "zmq_wrapper.h"
#include "zmq.hpp"
#include <chrono>
#include <condition_variable>
#include <random>

ZmqWrapper::ZmqWrapper() :
	context(new zmq::context_t(1)), frontend(nullptr),
	isWorking(true), worker(nullptr), isInit(false), flushOnExit(false) {

	bool socketInit = false;
	std::condition_variable threadReady;
	std::mutex threadMutex;


	this->worker = new std::thread([this, &threadReady, &socketInit, &threadMutex] {
		try {
			std::lock_guard<std::mutex> lock(threadMutex);

			this->frontend = std::unique_ptr<zmq::socket_t>(new zmq::socket_t(*(this->context), ZMQ_DEALER));

			int timeOut = 100;
			this->frontend->setsockopt(ZMQ_RCVTIMEO, &timeOut, sizeof(timeOut));
			timeOut *= 10;
			this->frontend->setsockopt(ZMQ_SNDTIMEO, &timeOut, sizeof(timeOut));
			socketInit = true;
		} catch (zmq::error_t & e) {
			this->isWorking = false;
			return;
		}
		threadReady.notify_all();

		while (!this->isInit) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		auto lastActiveTime = std::chrono::high_resolution_clock::now();
		try {
			while (this->isWorking) {
				zmq::message_t incoming;

				auto now = std::chrono::high_resolution_clock::now();

				// send keepalive
				if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastActiveTime).count() > DISCONNECT_TIMEOUT / 2) {
					zmq::message_t keepAlive(1);
					*reinterpret_cast<char *>(keepAlive.data()) = 1;
					if (!this->frontend->send(keepAlive)) {
						continue;
					}
				}

				if (this->messageQue.size() && this->frontend->connected()) {
					while (this->messageQue.size()) {
						std::lock_guard<std::mutex> lock(this->messageMutex);
						if (!this->frontend->send(this->messageQue.front().getMessage())) {
							break;
						}
						lastActiveTime = now;
						this->messageQue.pop();
					}
				}

				if (this->frontend->recv(&incoming)) {
					// if it's not user message - dont propagate
					const char * data = reinterpret_cast<const char *>(incoming.data());
					if (data[0] == 0) {
						VRayMessage msg(incoming);
						this->callback(msg, this);
					}
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		} catch (zmq::error_t & e) {
			auto x = e.what();
			assert(false);
			return;
		}


		if (this->flushOnExit && this->messageQue.size()) {
			std::lock_guard<std::mutex> lock(this->messageMutex);
			while (this->messageQue.size()) {
				if (!this->frontend->send(this->messageQue.front().getMessage())) {
					break;
				}
				this->messageQue.pop();
			}
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

void ZmqWrapper::setFlushOnExit(bool flag) {
	flushOnExit = flag;
}

bool ZmqWrapper::getFlushOnexit() const {
	return flushOnExit;
}

void ZmqWrapper::setCallback(ZmqWrapperCallback_t cb) {
	this->callback = cb;
}

void ZmqWrapper::send(VRayMessage && message) {
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
	std::random_device device;
	std::mt19937_64 generator(device());
	uint64_t id = generator();

	this->frontend->setsockopt(ZMQ_IDENTITY, &id, sizeof(id));

	this->frontend->connect(addr);
	this->isInit = true;
}
