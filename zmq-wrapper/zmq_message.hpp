#ifndef _ZMQ_MESSAGE_H_
#define _ZMQ_MESSAGE_H_

#include "zmq.hpp"
#include "base_types.h"
#include "serializer.hpp"
#include "deserializer.hpp"

class VRayMessage {
public:
	enum Type : int { PluginProperty, None };


	VRayMessage(zmq::message_t & message): message(0), plugin(""), property("") {
		this->message.move(&message);
		this->parse();
	}

	VRayMessage(VRayMessage && other): message(0), plugin(std::move(other.plugin)), property(std::move(other.property)) {
		this->message.move(&other.message);
	}

	VRayMessage(int size): message(size) {
	}

	zmq::message_t & getMessage() {
		return this->message;
	}

	const std::string & getProperty() const {
		return this->property;
	}

	const std::string & getPlugin() const {
		return this->plugin;
	}

	VRayBaseTypes::AttrTransform getTransform() {
		return this->tr;
	}

	template <typename T>
	static VRayMessage createMessage(const std::string & plugin, const std::string & property, const T & value) {
		using namespace std;
		SerializerStream strm;
		strm << VRayMessage::PluginProperty << plugin << property << value.getType() << value;
		VRayMessage msg(strm.getSize());
		memcpy(msg.getMessage().data(), strm.getData(), strm.getSize());
		return msg;
	}
private:

	void parse() {
		using namespace VRayBaseTypes;

		DeserializerStream stream(reinterpret_cast<char*>(message.data()), message.size());
		stream >> type;

		if (type == PluginProperty) {
			ValueType valueType;
			stream >> plugin >> property >> valueType;
			switch (valueType) {
			case ValueType::ValueTypeTransform:
				stream >> this->tr;
				break;
			default:
				break;
			}
		}

	}

	char * data() {
		return reinterpret_cast<char*>(this->message.data());
	}

	VRayBaseTypes::AttrTransform tr;

	zmq::message_t message;
	std::string plugin, property;
	Type type;
};

#endif // _ZMQ_MESSAGE_H_
