#include "zmq_wrapper.h"
#include "zmq.hpp"

VRayMessage::VRayMessage(zmq::message_t & message): message(0), plugin(""), property("") {
	this->message.move(&message);
}

VRayMessage::VRayMessage(int size): message(size) {
}

VRayMessage::Type VRayMessage::getType() {
	return *reinterpret_cast<Type*>(this->data());
}

std::string VRayMessage::getPlugin() {
	if (this->plugin == "" && this->message.size()) {
		char * start = this->data() + sizeof(VRayMessage::Type) + sizeof(int);
		this->plugin = std::string(start, *reinterpret_cast<int*>(this->data() + sizeof(VRayMessage::Type)));
	}
	return this->plugin;
}


std::string VRayMessage::getProperty() {
	if (this->property == "" && this->message.size()) {
		int offset = sizeof(VRayMessage::Type) + sizeof(int) + this->getPlugin().size();
		char * start = this->data() + offset + sizeof(int);
		this->property = std::string(start, *reinterpret_cast<int*>(this->data() + offset));
	}
	return this->property;
}

char * VRayMessage::data() {
	return reinterpret_cast<char*>(this->message.data());
}

int VRayMessage::getValueOffset() {
	return sizeof(Type) + sizeof(int) + this->getPlugin().size() + sizeof(int) + this->getProperty().size() + sizeof(DataType);
}

zmq::message_t & VRayMessage::getMessage() {
	return this->message;
}


ZmqWrapper::ZmqWrapper() : context(new zmq::context_t(1)), socket(new zmq::socket_t(*context, ZMQ_DEALER)), isWorking(false), worker(nullptr) {
	int timeOut = 100;
	this->socket->setsockopt(ZMQ_RCVTIMEO, &timeOut, sizeof(timeOut));
}

void ZmqWrapper::setCallback(ZmqWrapperCallback_t cb) {
	this->callback = cb;
}

void ZmqWrapper::send(VRayMessage &message) {
	this->socket->send(message.getMessage());
}

void ZmqWrapper::send(const char * data, int size) {
	this->socket->send(data, size);
}

void ZmqWrapper::start() {
	this->isWorking = true;
	this->worker = new std::thread([this] {
		while (this->isWorking) {
			zmq::message_t message;
			if(this->socket->recv(&message)) {
				this->callback(VRayMessage(message), this);
			}
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
