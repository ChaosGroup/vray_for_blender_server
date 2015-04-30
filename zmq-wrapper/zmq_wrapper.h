#include <string>
#include <iostream>
#include <functional>
#include <thread>

namespace zmq {
class message_t;
class context_t;
class socket_t;
}

class ZmqWrapperMessage {
public:
	ZmqWrapperMessage(zmq::message_t * message);
	void * data();
	zmq::message_t * getMessage();

private:
	zmq::message_t * message;
};

/**
 * Async wrapper for zmq::socket_t with callback on data received.
 */
class ZmqWrapper {
public:
	typedef std::function<void(ZmqWrapperMessage &, ZmqWrapper * client)> ZmqWrapperCallback_t;

	ZmqWrapper();

	void start();
	void stop();
	void send(const char * data, int size);
	void send(ZmqWrapperMessage & message);

	void setCallback(ZmqWrapperCallback_t cb);

private:
	ZmqWrapperCallback_t callback;
	std::thread * worker;
	bool isWorking;
	zmq::context_t * context;

protected:
	zmq::socket_t * socket;
};


class ZmqClient: public ZmqWrapper {
public:
	void connect(const char * addr);
};


class ZmqServer: public ZmqWrapper {
public:
	void bind(const char * addr);
};
