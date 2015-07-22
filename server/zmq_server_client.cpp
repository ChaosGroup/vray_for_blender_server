#include "zmq_server_client.h"
#include <random>

using namespace std;

ZmqServerClient::ZmqServerClient(): identity(0) {

}

#ifdef VRAY_ZMQ_SINGLE_MODE
void ZmqServerClient::connect(const char * addr) {
	this->frontend->bind(addr);
	this->isInit = true;
}

#else // VRAY_ZMQ_SINGLE_MODE
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
#endif // VRAY_ZMQ_SINGLE_MODE


void ZmqServerClient::setIdentity(uint64_t id) {
	this->identity = id;
}

