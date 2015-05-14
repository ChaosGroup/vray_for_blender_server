#ifndef _ZMQ_WRAPPER_H_
#define _ZMQ_WRAPPER_H_

#include <string>
#include <iostream>
#include <functional>
#include <thread>
#include <zmq.hpp>
#include <memory>
#include <queue>
#include <mutex>

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

	/// send will copy size bytes from data internally, callee can free memory immediately
	void send(void *data, int size);

	/// send steals the message contents and is ZmqWrapper's resposibility
	void send(VRayMessage & message);

	void setCallback(ZmqWrapperCallback_t cb);

private:
	ZmqWrapperCallback_t callback;
	std::thread * worker;
	bool isWorking;

	std::unique_ptr<zmq::context_t> context;
	std::queue<VRayMessage> messageQue;
	std::mutex messageMutex;

protected:
	bool isInit;
	std::unique_ptr<zmq::socket_t> frontend;
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
