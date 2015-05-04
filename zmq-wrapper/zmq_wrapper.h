#include <string>
#include <iostream>
#include <functional>
#include <thread>
#include "base_types.h"
#include <zmq.hpp>


namespace zmq {
	class message_t;
	class context_t;
	class socket_t;
}

class VRayMessage {
public:
	enum Type : int { PluginProperty, None };
	enum DataType : int { Transform };

	VRayMessage(zmq::message_t & message);
	VRayMessage(int size);

	zmq::message_t & getMessage();

	Type getType();

	std::string getPlugin();
	std::string getProperty();

	template <typename T>
	void getValue(T &);

	template <>
	void getValue(VRayBaseTypes::AttrTransform & tr) {
		memcpy(&tr, this->data() + this->getValueOffset(), sizeof(VRayBaseTypes::AttrTransform));
	}

	template <typename T>
	static VRayMessage createMessage(const char * plugin, const char * property, const T & value);


	template <>
	static VRayMessage createMessage(const char * plugin, const char * property, const VRayBaseTypes::AttrTransform & value) {
		int plugin_lenght = strlen(plugin),
			prop_length = strlen(property);

		int length = sizeof(VRayMessage::Type) + sizeof(int) + plugin_lenght + sizeof(int) + prop_length + sizeof(VRayMessage::DataType) + sizeof(value);
		zmq::message_t msg(length);
		char * data = reinterpret_cast<char*>(msg.data());

		Type t = PluginProperty;

		memcpy(data, &t, sizeof(t));
		data += sizeof(VRayMessage::PluginProperty);

		memcpy(data, &plugin_lenght, sizeof(int));
		data += sizeof(int);

		memcpy(data, plugin, plugin_lenght);
		data += plugin_lenght;

		memcpy(data, &prop_length, sizeof(int));
		data += sizeof(int);

		memcpy(data, property, prop_length);
		data += prop_length;

		DataType type = Transform;
		memcpy(data, &type, sizeof(type));
		data += sizeof(type);

		memcpy(data, &value, sizeof(value));

		return msg;
	}

private:
	char * data();
	int getValueOffset();

	zmq::message_t message;
	std::string plugin, property;
	Type type;
};


/**
 * Async wrapper for zmq::socket_t with callback on data received.
 */
class ZmqWrapper {
public:
	typedef std::function<void(VRayMessage &, ZmqWrapper *)> ZmqWrapperCallback_t;

	ZmqWrapper();

	void start();
	void stop();
	void send(const char * data, int size);
	void send(VRayMessage & message);

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
