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
											VRayBaseTypes::AttrInstancer,
											VRayBaseTypes::AttrImage>::value;



class VRayMessage {
public:
	enum Type : int { NoType, SingleValue, ChangePlugin };
	enum PluginAction { NoAction, Create, Remove, Update };


	VRayMessage(zmq::message_t & message):
		message(0), plugin(""), property(""),
		type(NoType), valueType(VRayBaseTypes::ValueType::ValueTypeUnknown),
		action(NoAction) {
		this->message.move(&message);
		this->parse();
	}

	VRayMessage(VRayMessage && other):
		message(0), type(other.type), valueType(other.valueType),
		action(other.action), plugin(std::move(other.plugin)),
		property(std::move(other.property)) {

		this->message.move(&other.message);
	}

	VRayMessage(int size):
		message(size), plugin(""), property(""),
		type(NoType), valueType(VRayBaseTypes::ValueType::ValueTypeUnknown),
		action(NoAction) {
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

	Type getType() const {
		return type;
	}

	PluginAction getPluginAction() const {
		return action;
	}

	template <typename T>
	const T * getValue() const {
		return reinterpret_cast<const T *>(this->value_data);
	}

	VRayBaseTypes::ValueType getValueType() const {
		return valueType;
	}

	VRayMessage(const VRayMessage &) = delete;
	VRayMessage & operator=(const VRayMessage &) = delete;



	static VRayMessage createMessage(const std::string & plugin, PluginAction action) {
		assert(action == Create || action == Remove);
		SerializerStream strm;
		strm << VRayMessage::ChangePlugin << plugin << action;
		return fromStream(strm);
	}

	/// Creates message to control a plugin property
	template <typename T>
	static VRayMessage createMessage(const std::string & plugin, const std::string & property, const T & value) {
		using namespace std;
		SerializerStream strm;
		strm << VRayMessage::ChangePlugin << plugin << PluginAction::Update << property << value.getType() << value;
		return fromStream(strm);
	}

	/// Create message containing only a value, eg AttrImage
	template <typename T>
	static VRayMessage createMessage(const T & value) {
		SerializerStream strm;
		strm << VRayMessage::SingleValue << value.getType() << value;
		return fromStream(strm);
	}



private:

	template <typename T>
	T * setValue() {
		T * ptr = reinterpret_cast<T *>(this->value_data);
		new(ptr)T();
		return ptr;
	}

	static VRayMessage fromStream(SerializerStream & strm) {
		VRayMessage msg(strm.getSize());
		memcpy(msg.message.data(), strm.getData(), strm.getSize());
		return msg;
	}

	void parse() {
		using namespace VRayBaseTypes;

		DeserializerStream stream(reinterpret_cast<char*>(message.data()), message.size());
		stream >> type;

		if (type == ChangePlugin) {

			stream >> plugin >> action;
			if (action == Update) {
				stream >> property >> valueType;
				switch (valueType) {
				case ValueType::ValueTypeColor:
					stream >> *setValue<AttrColor>();
					break;
				case ValueType::ValueTypeAColor:
					stream >> *setValue<AttrAColor>();
					break;
				case ValueType::ValueTypeVector:
					stream >> *setValue<AttrVector>();
					break;
				case ValueType::ValueTypeVector2:
					stream >> *setValue<AttrVector2>();
					break;
				case ValueType::ValueTypeMatrix:
					stream >> *setValue<AttrMatrix>();
					break;
				case ValueType::ValueTypeTransform:
					stream >> *setValue<AttrTransform>();
					break;
				case ValueType::ValueTypePlugin:
					stream >> *setValue<AttrPlugin>();
					break;
				case ValueType::ValueTypeListInt:
					stream >> *setValue<AttrListInt>();
					break;
				case ValueType::ValueTypeListFloat:
					stream >> *setValue<AttrListFloat>();
					break;
				case ValueType::ValueTypeListColor:
					stream >> *setValue<AttrListColor>();
					break;
				case ValueType::ValueTypeListVector:
					stream >> *setValue<AttrListVector>();
					break;
				case ValueType::ValueTypeListVector2:
					stream >> *setValue<AttrListVector2>();
					break;
				case ValueType::ValueTypeListPlugin:
					stream >> *setValue<AttrListPlugin>();
					break;
				case ValueType::ValueTypeListString:
					stream >> *setValue<AttrListString>();
					break;
				case ValueType::ValueTypeMapChannels:
					stream >> *setValue<AttrMapChannels>();
					break;
				case ValueType::ValueTypeInstancer:
					stream >> *setValue<AttrInstancer>();
					break;
				default:
					throw 42;
				}
			}


		} else if(type == SingleValue) {
			stream >> valueType;
			switch(valueType) {
			case ValueType::ValueTypeImage:
				stream >> *setValue<AttrImage>();
				break;
			default:
				throw 42;
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
	PluginAction action;
	VRayBaseTypes::ValueType valueType;
};

#endif // _ZMQ_MESSAGE_H_
