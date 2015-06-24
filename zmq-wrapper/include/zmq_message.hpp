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
const int MAX_MESSAGE_SIZE = max_type_sizeof<VRayBaseTypes::AttrColor,
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
											VRayBaseTypes::AttrImage,
											VRayBaseTypes::AttrSimpleType<int>,
											VRayBaseTypes::AttrSimpleType<float>,
											VRayBaseTypes::AttrSimpleType<double>,
											VRayBaseTypes::AttrSimpleType<std::string>>::value;



class VRayMessage {
public:
	enum class Type : int { None, SingleValue, ChangePlugin, ChangeRenderer };
	enum class PluginAction { None, Create, Remove, Update };
	enum class RendererAction { None, Init, Free, Start, Stop, Pause, Resume, Resize,
		_ArgumentRenderAction,
		AddHosts, RemoveHosts, LoadScene, AppendScene, ExportScene, SetRenderMode, SetAnimationProperties,
        SetCurrentTime, ClearFrameValues };

	enum class ValueSetter { None, Default, AsString };


	VRayMessage(zmq::message_t & message):
		message(0),	type(Type::None),
		valueType(VRayBaseTypes::ValueType::ValueTypeUnknown),
		pluginAction(PluginAction::None), rendererAction(RendererAction::None), valueSetter(ValueSetter::None) {
		this->message.move(&message);
		this->parse();
	}

	VRayMessage(VRayMessage && other):
		message(0), type(other.type), valueType(other.valueType),
		pluginAction(other.pluginAction), pluginName(std::move(other.pluginName)),
		pluginProperty(std::move(other.pluginProperty)), rendererAction(other.rendererAction),
		valueSetter(ValueSetter::None) {
		this->message.move(&other.message);
	}

	VRayMessage(int size):
		message(size), type(Type::None),
		valueType(VRayBaseTypes::ValueType::ValueTypeUnknown),
		pluginAction(PluginAction::None), rendererAction(RendererAction::None),
		valueSetter(ValueSetter::None) {
	}

	zmq::message_t & getMessage() {
		return this->message;
	}

	const std::string & getProperty() const {
		return this->pluginProperty;
	}

	const std::string & getPlugin() const {
		return this->pluginName;
	}

	const std::string & getPluginType() const {
		return this->pluginType;
	}

	Type getType() const {
		return type;
	}

	PluginAction getPluginAction() const {
		return pluginAction;
	}

	RendererAction getRendererAction() const {
		return rendererAction;
	}

	ValueSetter getValueSetter() const {
		return valueSetter;
	}

	void getRendererSize(int & width, int & height) {
		width = this->rendererWidth;
		height = this->rendererHeight;
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



	/// Static methods for creating messages

	static VRayMessage createMessage(const std::string & pluginName, const std::string & pluginType) {
		SerializerStream strm;
		strm << VRayMessage::Type::ChangePlugin << pluginName << PluginAction::Create << pluginType;
		return fromStream(strm);
	}

	static VRayMessage createMessage(const std::string & plugin, PluginAction action) {
		assert(action == PluginAction::Create || action == PluginAction::Remove && "Wrong PluginAction");
		SerializerStream strm;
		strm << VRayMessage::Type::ChangePlugin << plugin << action;
		return fromStream(strm);
	}

	/// Creates message to control a plugin property
	template <typename T>
	static VRayMessage createMessage(const std::string & plugin, const std::string & property, const T & value, bool stringValue = false) {
		using namespace std;
		SerializerStream strm;
		ValueSetter setter = stringValue ? ValueSetter::AsString : ValueSetter::Default;
		strm << VRayMessage::Type::ChangePlugin << plugin << PluginAction::Update << property << setter << value.getType() << value;
		return fromStream(strm);
	}

	static VRayMessage createMessage(const std::string & plugin, const std::string & property, const std::string & value, bool stringValue) {
		using namespace std;
		SerializerStream strm;
		strm << VRayMessage::Type::ChangePlugin << plugin << PluginAction::Update << property
		     << ValueSetter::AsString << VRayBaseTypes::ValueType::ValueTypeString << value;
		return fromStream(strm);
	}

	/// Create message containing only a value, eg AttrImage
	template <typename T>
	static VRayMessage createMessage(const T & value) {
		SerializerStream strm;
		strm << VRayMessage::Type::SingleValue << value.getType() << value;
		return fromStream(strm);
	}

	/// create message to control renderer

	static VRayMessage createMessage(const RendererAction & action) {
		assert(action < RendererAction::_ArgumentRenderAction && "Renderer action provided requires argument!");
		SerializerStream strm;
		strm << Type::ChangeRenderer << action;
		return fromStream(strm);
	}

	template <typename T>
	static VRayMessage createMessage(const RendererAction & action, const T & value) {
		assert(action > RendererAction::_ArgumentRenderAction && "Renderer action provided requires NO argument!");
		SerializerStream strm;
		VRayBaseTypes::AttrSimpleType<T> valWrapper(value);
		strm << Type::ChangeRenderer << action << valWrapper.getType() << valWrapper;
		return fromStream(strm);
	}


	static VRayMessage createMessage(const RendererAction & action, int width, int height) {
		assert(action == RendererAction::Resize && "Resize renderer action required");
		SerializerStream strm;
		strm << Type::ChangeRenderer << RendererAction::Resize << width << height;
		return fromStream(strm);
	}


	~VRayMessage() {
		using namespace VRayBaseTypes;

		switch (valueType) {
		case ValueType::ValueTypeColor:
			getValue<AttrColor>()->~AttrColor();
			break;
		case ValueType::ValueTypeAColor:
			getValue<AttrAColor>()->~AttrAColor();
			break;
		case ValueType::ValueTypeVector:
			getValue<AttrVector>()->~AttrVector();
			break;
		case ValueType::ValueTypeVector2:
			getValue<AttrVector2>()->~AttrVector2();
			break;
		case ValueType::ValueTypeMatrix:
			getValue<AttrMatrix>()->~AttrMatrix();
			break;
		case ValueType::ValueTypeTransform:
			getValue<AttrTransform>()->~AttrTransform();
			break;
		case ValueType::ValueTypePlugin:
			getValue<AttrPlugin>()->~AttrPlugin();
			break;
		case ValueType::ValueTypeListInt:
			getValue<AttrListInt>()->~AttrListInt();
			break;
		case ValueType::ValueTypeListFloat:
			getValue<AttrListFloat>()->~AttrListFloat();
			break;
		case ValueType::ValueTypeListColor:
			getValue<AttrListColor>()->~AttrListColor();
			break;
		case ValueType::ValueTypeListVector:
			getValue<AttrListVector>()->~AttrListVector();
			break;
		case ValueType::ValueTypeListVector2:
			getValue<AttrListVector2>()->~AttrListVector2();
			break;
		case ValueType::ValueTypeListPlugin:
			getValue<AttrListPlugin>()->~AttrListPlugin();
			break;
		case ValueType::ValueTypeListString:
			getValue<AttrListString>()->~AttrListString();
			break;
		case ValueType::ValueTypeMapChannels:
			getValue<AttrMapChannels>()->~AttrMapChannels();
			break;
		case ValueType::ValueTypeInstancer:
			getValue<AttrInstancer>()->~AttrInstancer();
			break;
		case ValueType::ValueTypeImage:
			getValue<AttrImage>()->~AttrImage();
			break;
		case ValueType::ValueTypeString:
			getValue<AttrSimpleType<std::string>>()->~AttrSimpleType();
			break;
		}
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


	void readValue(DeserializerStream & stream) {
		using namespace VRayBaseTypes;

		stream >> valueType;
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
		case ValueType::ValueTypeImage:
			stream >> *setValue<AttrImage>();
			break;
		case ValueType::ValueTypeInt:
			stream >> *setValue<AttrSimpleType<int>>();
			break;
		case ValueType::ValueTypeFloat:
			stream >> *setValue<AttrSimpleType<float>>();
			break;
		case ValueType::ValueTypeDouble:
			stream >> *setValue<AttrSimpleType<double>>();
			break;
		case ValueType::ValueTypeString:
			stream >> *setValue<AttrSimpleType<std::string>>();
			break;
		default:
			assert(false && "Failed to parse value!\n");
		}
	}

	void parse() {
		using namespace VRayBaseTypes;

		DeserializerStream stream(reinterpret_cast<char*>(message.data()), message.size());
		stream >> type;

		if (type == Type::ChangePlugin) {
			stream >> pluginName >> pluginAction;
			if (pluginAction == PluginAction::Update) {
				stream >> pluginProperty >> valueSetter;
				readValue(stream);
			} else if (pluginAction == PluginAction::Create) {
				if (stream.hasMore()) {
					stream >> pluginType;
				}
			}
		} else if (type == Type::SingleValue) {
			readValue(stream);
		} else if (type == Type::ChangeRenderer) {
			stream >> rendererAction;
			if (rendererAction == RendererAction::Resize) {
				stream >> rendererWidth >> rendererHeight;
			} else if (rendererAction > RendererAction::_ArgumentRenderAction) {
				readValue(stream);
            }
		}
	}

	char * data() {
		return reinterpret_cast<char*>(this->message.data());
	}

	uint8_t value_data[MAX_MESSAGE_SIZE];

	zmq::message_t message;
	std::string pluginName, pluginType, pluginProperty;

	Type type;
	PluginAction pluginAction;
	RendererAction rendererAction;
	ValueSetter valueSetter;

	int rendererWidth, rendererHeight;
	VRayBaseTypes::ValueType valueType;
};

#endif // _ZMQ_MESSAGE_H_
