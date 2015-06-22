#ifndef _ZMQ_SERVER_H_
#define _ZMQ_SERVER_H_

#include "../zmq-wrapper/include/zmq_wrapper.h"

class ZmqServerClient: public ZmqWrapper {
public:
	ZmqServerClient();

	void setIdentity(uint64_t id);
	void connect(const char * addr);

private:
	uint64_t identity;
};

#endif // _ZMQ_SERVER_H_
