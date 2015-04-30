#include "zmq_wrapper.h"
#include "zmq.hpp"

ZmqWrapperMessage::ZmqWrapperMessage(zmq::message_t * message) : message(message) {
}

void * ZmqWrapperMessage::data() {
	return this->message->data();
}

zmq::message_t * ZmqWrapperMessage::getMessage() {
	return this->message;
}


ZmqWrapper::ZmqWrapper() : context(new zmq::context_t(1)), socket(new zmq::socket_t(*context, ZMQ_DEALER)), isWorking(false), worker(nullptr) {
}

void ZmqWrapper::setCallback(ZmqWrapperCallback_t cb) {
	this->callback = cb;
}

void ZmqWrapper::send(ZmqWrapperMessage & message) {
	this->socket->send(*message.getMessage());
}

void ZmqWrapper::send(const char * data, int size) {
	this->socket->send(data, size);
}

void ZmqWrapper::start() {
	this->isWorking = true;
	this->worker = new std::thread([this] {
		while (this->isWorking) {
			zmq::message_t message;
			this->socket->recv(&message);
			ZmqWrapperMessage msg(&message);
			this->callback(msg, this);
		}
	});
}

void ZmqWrapper::stop() {
	if (this->worker) {
		this->isWorking = false;
		
		if (this->worker->joinable()) {
			this->worker->join();
		}

		delete this->worker;
		this->worker = nullptr;
	}
}

void ZmqClient::connect(const char * addr) {
	this->socket->connect(addr);
}

void ZmqServer::bind(const char * addr) {
	this->socket->bind(addr);
}