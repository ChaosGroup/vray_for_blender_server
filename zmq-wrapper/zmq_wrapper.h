#ifndef _ZMQ_WRAPPER_H_
#define _ZMQ_WRAPPER_H_

#include <string>
#include <iostream>
#include <functional>
#include <thread>
#include <zmq.hpp>
#include <memory>

#include "base_types.h"
#include "zmq_message.hpp"

/**
 * Async wrapper for zmq::socket_t with callback on data received.
 */
class ZmqWrapper {
public:
	typedef std::function<void(VRayMessage &, ZmqWrapper *)> ZmqWrapperCallback_t;

	ZmqWrapper();
	~ZmqWrapper();

	void send(const void * data, int size);
	void send(VRayMessage & message);

	void setCallback(ZmqWrapperCallback_t cb);

private:
	ZmqWrapperCallback_t callback;
	std::thread * worker;
	bool isWorking;

	std::unique_ptr<zmq::context_t> context;
protected:
	bool isInit;
	std::unique_ptr<zmq::socket_t> inproc, frontend;

	std::function<void()> setUp;
};


class ZmqClient: public ZmqWrapper {
public:
	void connect(const char * addr);
};


class ZmqServer: public ZmqWrapper {
public:
	void bind(const char * addr);
};


#endif // _ZMQ_WRAPPER_H_
