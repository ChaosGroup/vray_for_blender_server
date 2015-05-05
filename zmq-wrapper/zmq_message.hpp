#ifndef _ZMQ_MESSAGE_H_
#define _ZMQ_MESSAGE_H_

#include "zmq.hpp"
#include "base_types.h"
#include "serializer.hpp"

class VRayMessage {
public:
	enum Type : int { PluginProperty, None };


	VRayMessage(zmq::message_t & message): message(0), plugin(""), property("") {
		this->message.move(&message);
	}

	VRayMessage(VRayMessage && other): message(0), plugin(std::move(other.plugin)), property(std::move(other.property)) {
		this->message.move(&other.message);
	}

	VRayMessage(int size): message(size) {
	}

	Type VRayMessage::getType() {
		return *reinterpret_cast<Type*>(this->data());
	}

	std::string getPlugin() {
		if (this->plugin == "" && this->message.size()) {
			char * start = this->data() + sizeof(VRayMessage::Type) + sizeof(int);
			this->plugin = std::string(start, *reinterpret_cast<int*>(this->data() + sizeof(VRayMessage::Type)));
		}
		return this->plugin;
	}


	std::string getProperty() {
		if (this->property == "" && this->message.size()) {
			int offset = sizeof(VRayMessage::Type) + sizeof(int) + this->getPlugin().size();
			char * start = this->data() + offset + sizeof(int);
			this->property = std::string(start, *reinterpret_cast<int*>(this->data() + offset));
		}
		return this->property;
	}

	zmq::message_t & getMessage() {
		return this->message;
	}

	template <typename T>
	void getValue(T &);

	template <>
	void getValue(VRayBaseTypes::AttrTransform & tr) {
		memcpy(&tr, this->data() + this->getValueOffset(), sizeof(VRayBaseTypes::AttrTransform));
	}


	template <typename T>
	static VRayMessage createMessage(const std::string & plugin, const std::string & property, const T & value) {
		SerializerStream strm;
		strm << VRayMessage::PluginProperty << plugin << property << value;
		VRayMessage msg(strm.getSize());
		memcpy(msg.getMessage().data(), strm.getData(), strm.getSize());
		return msg;
	}


private:

	char * data() {
		return reinterpret_cast<char*>(this->message.data());
	}

	int getValueOffset() {
		return sizeof(Type) + sizeof(int) + this->getPlugin().size() + sizeof(int) + this->getProperty().size() + sizeof(VRayBaseTypes::ValueType);
	}

	zmq::message_t message;
	std::string plugin, property;
	Type type;
};

#endif // _ZMQ_MESSAGE_H_
