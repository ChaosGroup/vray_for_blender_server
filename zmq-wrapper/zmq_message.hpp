#ifndef _ZMQ_MESSAGE_H_
#define _ZMQ_MESSAGE_H_

#include "zmq.hpp"
#include "base_types.h"
#include "serializer.hpp"
#include "deserializer.hpp"

// Compile time max(sizeof(A), sizeof(B))
template <size_t A, size_t B>
struct compile_time_max {
	enum { value = (A > B ? A : B) };
};


// recursive template for max of variable number of template arguments
// general case - max of sizeof of the first type and recursive call for the rest
template <typename T, typename ... Q>
struct max_type_sizeof {
	enum { value = compile_time_max<sizeof(T), max_type_sizeof<Q...>::value >::value };
};

// base case for just 2 types
template <typename T, typename Q>
struct max_type_sizeof<T, Q> {
	enum { value = compile_time_max<sizeof(T), sizeof(Q)>::value };
};


// maximum possible size of a value in the message
const int MAX_MESSAGE_SIZE = max_type_sizeof<VRayBaseTypes::AttrBase,
											VRayBaseTypes::AttrColor,
											VRayBaseTypes::AttrAColor,
											VRayBaseTypes::AttrVector,
											VRayBaseTypes::AttrVector2,
											VRayBaseTypes::AttrMatrix,
											VRayBaseTypes::AttrTransform,
											VRayBaseTypes::AttrPlugin,
											VRayBaseTypes::AttrListInt,
											VRayBaseTypes::AttrListFloat,
											VRayBaseTypes::AttrListColor,
											VRayBaseTypes::AttrListVector,
											VRayBaseTypes::AttrListVector2,
											VRayBaseTypes::AttrListPlugin,
											VRayBaseTypes::AttrListString,
											VRayBaseTypes::AttrMapChannels,
											VRayBaseTypes::AttrInstancer>::value;



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

	template <typename T>
	static VRayMessage createMessage(const std::string & plugin, const std::string & property, const T & value) {
		using namespace std;
		SerializerStream strm;
		strm << VRayMessage::PluginProperty << plugin << property << value.getType() << value;
		VRayMessage msg(strm.getSize());
		memcpy(msg.getMessage().data(), strm.getData(), strm.getSize());
		return msg;
	}

	template <typename T>
	const T * getValue() const {
		return reinterpet_cast<const T *>(this->value_data);
	}

	template <typename T>
	T * getValue() {
		return reinterpret_cast<T *>(this->value_data);
	}

	VRayBaseTypes::ValueType getValueType() const {
		return valueType;
	}

private:

	void parse() {
		using namespace VRayBaseTypes;

		DeserializerStream stream(reinterpret_cast<char*>(message.data()), message.size());
		stream >> type;

		if (type == PluginProperty) {

			stream >> plugin >> property >> valueType;

			switch (valueType) {
			case ValueType::ValueTypeColor:
				stream >> *getValue<AttrColor>();
				break;
			case ValueType::ValueTypeAColor:
				stream >> *getValue<AttrAColor>();
				break;
			case ValueType::ValueTypeVector:
				stream >> *getValue<AttrVector>();
				break;
			case ValueType::ValueTypeVector2:
				stream >> *getValue<AttrVector2>();
				break;
			case ValueType::ValueTypeMatrix:
				stream >> *getValue<AttrMatrix>();
				break;
			case ValueType::ValueTypeTransform:
				stream >> *getValue<AttrTransform>();
				break;
			case ValueType::ValueTypePlugin:
				stream >> *getValue<AttrPlugin>();
				break;
			case ValueType::ValueTypeListInt:
				stream >> *getValue<AttrListInt>();
				break;
			case ValueType::ValueTypeListFloat:
				stream >> *getValue<AttrListFloat>();
				break;
			case ValueType::ValueTypeListColor:
				stream >> *getValue<AttrListColor>();
				break;
			case ValueType::ValueTypeListVector:
				stream >> *getValue<AttrListVector>();
				break;
			case ValueType::ValueTypeListVector2:
				stream >> *getValue<AttrListVector2>();
				break;
			case ValueType::ValueTypeListPlugin:
				stream >> *getValue<AttrListPlugin>();
				break;
			case ValueType::ValueTypeListString:
				stream >> *getValue<AttrListString>();
				break;
			case ValueType::ValueTypeMapChannels:
				stream >> *getValue<AttrMapChannels>();
				break;
			case ValueType::ValueTypeInstancer:
				stream >> *getValue<AttrInstancer>();
				break;
			default:
				break;
			}
		}




	}

	char * data() {
		return reinterpret_cast<char*>(this->message.data());
	}

	uint8_t value_data[MAX_MESSAGE_SIZE];

	zmq::message_t message;
	std::string plugin, property;
	Type type;
	VRayBaseTypes::ValueType valueType;
};

#endif // _ZMQ_MESSAGE_H_
