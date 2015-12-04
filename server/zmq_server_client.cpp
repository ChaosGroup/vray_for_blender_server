#define VRAY_RUNTIME_LOAD_SECONDARY
#include "zmq_server_client.h"
#include <random>

using namespace std;

ZmqServerClient::ZmqServerClient(): identity(0) {

}

void ZmqServerClient::connect(const char * addr) {
	uint64_t id;

	if (this->identity != 0) {
		id = this->identity;
	} else {
		std::random_device device;
		std::mt19937_64 generator(device());
		id = generator();
	}

	this->frontend->setsockopt(ZMQ_IDENTITY, &id, sizeof(id));

	this->frontend->connect(addr);
	this->isInit = true;
}

void ZmqServerClient::setIdentity(uint64_t id) {
	this->identity = id;
}

