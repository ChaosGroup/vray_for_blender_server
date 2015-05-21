/*
 * Copyright (c) 2015, Chaos Software Ltd
 *
 * V-Ray For Blender
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef VRAY_FOR_BLENDER_BASE_TYPES_H
#define VRAY_FOR_BLENDER_BASE_TYPES_H


#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>


namespace VRayBaseTypes {

const int VectorBytesCount  = 3 * sizeof(float);
const int Vector2BytesCount = 2 * sizeof(float);


enum ValueType {
	ValueTypeUnknown = 0,

	ValueTypeInt,
	ValueTypeFloat,
	ValueTypeColor,
	ValueTypeAColor,
	ValueTypeVector,
	ValueTypeVector2,
	ValueTypeMatrix,
	ValueTypeTransform,
	ValueTypeString,
	ValueTypePlugin,

	ValueTypeImage,

	ValueTypeList,

	ValueTypeListInt,
	ValueTypeListFloat,
	ValueTypeListColor,
	ValueTypeListVector,
	ValueTypeListVector2,
	ValueTypeListMatrix,
	ValueTypeListTransform,
	ValueTypeListString,
	ValueTypeListPlugin,

	ValueTypeListValue,

	ValueTypeInstancer,
	ValueTypeMapChannels,
};

struct AttrBase {
	ValueType getType() const {
		assert(false);
		return ValueType::ValueTypeUnknown;
	}
};

template <typename T>
struct AttrSimpleType: public AttrBase {
	ValueType getType() const;
	AttrSimpleType(): m_Value() {}
	AttrSimpleType(const T & val): m_Value(val) {}

	T m_Value;
};

inline ValueType AttrSimpleType<int>::getType() const {
	return ValueType::ValueTypeInt;
}

inline ValueType AttrSimpleType<float>::getType() const {
	return ValueType::ValueTypeFloat;
}

inline ValueType AttrSimpleType<std::string>::getType() const {
	return ValueType::ValueTypeString;
}

struct AttrImage: public AttrBase {
	ValueType getType() const {
		return ValueType::ValueTypeImage;
	}

	enum ImageType {
		NONE, RGBA_REAL, JPG
	};

	AttrImage(): width(0), height(0), imageType(NONE), data(nullptr), size(0) {

	}

	AttrImage(const void * data, int size, AttrImage::ImageType type, int width, int height):
		data(nullptr)
	{
		set(data, size, type, width, height);
	}

	void set(const void * data, int size, AttrImage::ImageType type, int width, int height) {
		this->width = width;
		this->height = height;
		this->imageType = type;
		this->data.reset(new char[size]);
		this->size = size;
		memcpy(this->data.get(), data, size);
	}

	std::unique_ptr<char[]> data;
	int size, width, height;
	ImageType imageType;
};


struct AttrColorBase: public AttrBase {

	ValueType getType() const {
		return ValueType::ValueTypeColor;
	}

	AttrColorBase():
	    r(0.0f),
	    g(0.0f),
	    b(0.0f)
	{}

	AttrColorBase(const float &r, const float &g, const float &b):
	    r(r),
	    g(g),
	    b(b)
	{}

	AttrColorBase(float c):
		r(c),
		g(c),
		b(c)
	{}

	AttrColorBase(float color[4]):
		r(color[0]),
		g(color[1]),
		b(color[2])
	{}

	float r;
	float g;
	float b;
};


struct AttrAColorBase: public AttrBase {

	ValueType getType() const {
		return ValueType::ValueTypeAColor;
	}

	AttrAColorBase():
	    alpha(1.0f)
	{}

	AttrAColorBase(const AttrColorBase &c, const float &a=1.0f):
	    color(c),
	    alpha(a)
	{}

	AttrColorBase  color;
	float      alpha;
};


struct AttrVectorBase: public AttrBase {

	ValueType getType() const {
		return ValueType::ValueTypeVector;
	}

	AttrVectorBase():
	    x(0.0f),
	    y(0.0f),
	    z(0.0f)
	{}

	AttrVectorBase(float vector[3]):
		x(vector[0]),
		y(vector[1]),
		z(vector[2])
	{}

	AttrVectorBase(const float &_x, const float &_y, const float &_z):
		x(_x),
		y(_y),
		z(_z)
	{}

	float operator * (const AttrVectorBase other) {
		return x * other.x + y * other.y + z * other.z;
	}

	AttrVectorBase operator - (const AttrVectorBase other) {
		return AttrVectorBase(x - other.x, y - other.y, z - other.z);
	}

	bool operator == (const AttrVectorBase other) {
		return (x == other.x) && (y == other.y) && (z == other.z);
	}

	float len() const {
		return sqrtf(x * x + y * y + z * z);
	}

	void set(const float &_x, const float &_y, const float &_z) {
		x = _x;
		y = _y;
		z = _z;
	}

	void set(float vector[3]) {
		x = vector[0];
		y = vector[1];
		z = vector[2];
	}

	float x;
	float y;
	float z;
};


struct AttrVector2Base : public AttrBase {

	ValueType getType() const {
		return ValueType::ValueTypeVector2;
	}

	AttrVector2Base():
	    x(0.0f),
	    y(0.0f)
	{}

	AttrVector2Base(float vector[2]):
		x(vector[0]),
		y(vector[1])
	{}

	float x;
	float y;
};


struct AttrMatrixBase : public AttrBase {

	ValueType getType() const {
		return ValueType::ValueTypeMatrix;
	}

	AttrMatrixBase() {}

	AttrMatrixBase(float tm[3][3]):
	    v0(tm[0]),
	    v1(tm[1]),
	    v2(tm[2])
	{}

	AttrMatrixBase(float tm[4][4]):
	    v0(tm[0]),
	    v1(tm[1]),
	    v2(tm[2])
	{}

	AttrVectorBase v0;
	AttrVectorBase v1;
	AttrVectorBase v2;
};


struct AttrTransformBase : public AttrBase {

	ValueType getType() const {
		return ValueType::ValueTypeTransform;
	}

	AttrTransformBase() {}

	AttrTransformBase(float tm[4][4]):
	    m(tm),
	    offs(tm[3])
	{}

	AttrMatrixBase m;
	AttrVectorBase offs;
};


struct AttrPluginBase : public AttrBase {

	ValueType getType() const {
		return ValueType::ValueTypePlugin;
	}

	AttrPluginBase() {}
	AttrPluginBase(const std::string &name):
	    plugin(name)
	{}

	operator bool () const {
		return !plugin.empty();
	}
	std::string output;
	std::string plugin;
};


template <typename T>
struct AttrListBase : public AttrBase {
	typedef std::vector<T>              DataType;
	typedef boost::shared_ptr<DataType> DataArray;

	ValueType getType() const ;

	AttrListBase() {
		init();
	}

	AttrListBase(const int &size) {
		init();
		resize(size);
	}

	void init() {
		m_Ptr = DataArray(new DataType);
	}

	void resize(const int &cnt) {
		m_Ptr.get()->resize(cnt);
	}

	void append(const T &value) {
		m_Ptr.get()->push_back(value);
	}

	void prepend(const T &value) {
		m_Ptr.get()->insert(0, value);
	}

	int getCount() const {
		return m_Ptr.get()->size();
	}

// wont work for AttrList<std::string>
	int getBytesCount() const {
		return getCount() * sizeof(T);
	}

	T* operator * () {
		return &m_Ptr.get()->at(0);
	}

	const T* operator * () const {
		return &m_Ptr.get()->at(0);
	}

	operator bool () const {
		return m_Ptr && m_Ptr.get()->size();
	}

	const bool empty() const {
		return !m_Ptr || (m_Ptr.get()->size() == 0);
	}

	const DataArray getData() const {
		return m_Ptr;
	}

	DataArray ptr() {
		return m_Ptr;
	}

private:
	DataArray m_Ptr;
};

typedef AttrListBase<int>         AttrListInt;
typedef AttrListBase<float>       AttrListFloat;
typedef AttrListBase<AttrColorBase>   AttrListColor;
typedef AttrListBase<AttrVectorBase>  AttrListVector;
typedef AttrListBase<AttrVector2Base> AttrListVector2;
typedef AttrListBase<AttrPluginBase>  AttrListPlugin;
typedef AttrListBase<std::string> AttrListString;


inline ValueType AttrListInt::getType() const {
	return ValueType::ValueTypeListInt;
}

inline ValueType AttrListFloat::getType() const {
	return ValueType::ValueTypeListFloat;
}

inline ValueType AttrListColor::getType() const {
	return ValueType::ValueTypeListColor;
}

inline ValueType AttrListVector::getType() const {
	return ValueType::ValueTypeListVector;
}

inline ValueType AttrListVector2::getType() const {
	return ValueType::ValueTypeListVector2;
}

inline ValueType AttrListPlugin::getType() const {
	return ValueType::ValueTypeListPlugin;
}

inline ValueType AttrListString::getType() const {
	return ValueType::ValueTypeListString;
}


struct AttrMapChannelsBase : public AttrBase {

	ValueType getType() const {
		return ValueType::ValueTypeMapChannels;
	}

	struct AttrMapChannel {
		AttrListVector vertices;
		AttrListInt    faces;
		std::string    name;
	};
	typedef boost::unordered_map<std::string, AttrMapChannel> MapChannelsMap;

	MapChannelsMap data;
};


struct AttrInstancerBase : public AttrBase {

	ValueType getType() const {
		return ValueType::ValueTypeInstancer;
	}

	struct Item {
		int            index;
		AttrTransformBase  tm;
		AttrTransformBase  vel;
		AttrPluginBase     node;
	};
	typedef AttrListBase<Item> Items;

	Items data;
};

struct AttrValue {
	typedef AttrListBase<AttrValue> AttrListValue;

	AttrValue():
		type(ValueTypeUnknown)
	{}

	AttrValue(const std::string &attrValue) {
		type = ValueTypeString;
		valString = attrValue;
	}

	AttrValue(const char *attrValue) {
		type = ValueTypeString;
		valString = attrValue;
	}

	AttrValue(const AttrPluginBase attrValue) {
		type = ValueTypePlugin;
		valPlugin = attrValue;
	}

	AttrValue(const AttrPluginBase attrValue, const std::string &output) {
		type = ValueTypePlugin;
		valPlugin = attrValue;
		valPluginOutput = output;
	}

	AttrValue(const AttrColorBase &c) {
		type = ValueTypeColor;
		valColor = c;
	}

	AttrValue(const AttrAColorBase &ac) {
		type = ValueTypeAColor;
		valAColor = ac;
	}

	AttrValue(const AttrVectorBase &v) {
		type = ValueTypeVector;
		valVector = v;
	}

	AttrValue(const AttrMatrixBase &m) {
		type = ValueTypeMatrix;
		valMatrix = m;
	}

	AttrValue(const AttrTransformBase &attrValue) {
		type = ValueTypeTransform;
		valTransform = attrValue;
	}

	AttrValue(const int &attrValue) {
		type = ValueTypeInt;
		valInt = attrValue;
	}

	AttrValue(const bool &attrValue) {
		type = ValueTypeInt;
		valInt = attrValue;
	}

	AttrValue(const float &attrValue) {
		type = ValueTypeFloat;
		valFloat = attrValue;
	}

	AttrValue(const AttrListInt &attrValue) {
		type = ValueTypeListInt;
		valListInt = attrValue;
	}

	AttrValue(const AttrListFloat &attrValue) {
		type = ValueTypeListFloat;
		valListFloat = attrValue;
	}

	AttrValue(const AttrListVector &attrValue) {
		type = ValueTypeListVector;
		valListVector = attrValue;
	}

	AttrValue(const AttrListColor &attrValue) {
		type = ValueTypeListColor;
		valListColor = attrValue;
	}

	AttrValue(const AttrListPlugin &attrValue) {
		type = ValueTypeListPlugin;
		valListPlugin = attrValue;
	}

	AttrValue(const AttrListValue &attrValue) {
		type = ValueTypeListValue;
		valListValue = attrValue;
	}

	AttrValue(const AttrListString &attrValue) {
		type = ValueTypeListString;
		valListString = attrValue;
	}

	AttrValue(const AttrMapChannelsBase &attrValue) {
		type = ValueTypeMapChannels;
		valMapChannels = attrValue;
	}
	AttrValue(const AttrInstancerBase &attrValue) {
		type = ValueTypeInstancer;
		valInstancer = attrValue;
	}

	// TODO: Replace with single storage with reinterpret_cast<>
	int                 valInt;
	float               valFloat;
	AttrVectorBase          valVector;
	AttrColorBase           valColor;
	AttrAColorBase          valAColor;

	std::string         valString;

	AttrMatrixBase          valMatrix;
	AttrTransformBase       valTransform;

	AttrPluginBase          valPlugin;
	std::string         valPluginOutput;

	AttrListInt         valListInt;
	AttrListFloat       valListFloat;
	AttrListVector      valListVector;
	AttrListColor       valListColor;
	AttrListPlugin      valListPlugin;
	AttrListValue       valListValue;
	AttrListString      valListString;

	AttrMapChannelsBase     valMapChannels;
	AttrInstancerBase       valInstancer;

	ValueType           type;

	const char *getTypeAsString() const {
		switch (type) {
			case ValueTypeInt:           return "Int";
			case ValueTypeFloat:         return "Float";
			case ValueTypeColor:         return "Color";
			case ValueTypeAColor:        return "AColor";
			case ValueTypeVector:        return "Vector";
			case ValueTypeTransform:     return "Transform";
			case ValueTypeString:        return "String";
			case ValueTypePlugin:        return "Plugin";
			case ValueTypeListInt:       return "ListInt";
			case ValueTypeListFloat:     return "ListFloat";
			case ValueTypeListColor:     return "ListColor";
			case ValueTypeListVector:    return "ListVector";
			case ValueTypeListMatrix:    return "ListMatrix";
			case ValueTypeListTransform: return "ListTransform";
			case ValueTypeListString:    return "ListString";
			case ValueTypeListPlugin:    return "ListPlugin";
			case ValueTypeListValue:     return "ListValue";
			case ValueTypeInstancer:     return "Instancer";
			case ValueTypeMapChannels:   return "Map Channels";
			default:
				break;
		}
		return "Unknown";
	}

	operator bool () const {
		bool valid = true;
		if (type == ValueTypeUnknown) {
			valid = false;
		}
		else if (type == ValueTypePlugin) {
			valid = !!(valPlugin);
		}
		return valid;
	}
};


struct PluginAttr {
	PluginAttr() {}
	PluginAttr(const std::string &attrName, const AttrValue &attrValue):
	    attrName(attrName),
	    attrValue(attrValue)
	{}

	std::string  attrName;
	AttrValue    attrValue;

};
typedef boost::unordered_map<std::string, PluginAttr> PluginAttrs;


struct PluginDesc {
	typedef boost::unordered_map<std::string, PluginDesc> PluginAttrsCache;
	static PluginAttrsCache cache;

	std::string  pluginName;
	std::string  pluginID;
	PluginAttrs  pluginAttrs;

	//PluginDesc() {}
	PluginDesc(const std::string &plugin_name, const std::string &plugin_id, const std::string &prefix=""):
		pluginName(plugin_name),
		pluginID(plugin_id)
	{
		if (!prefix.empty()) {
			pluginName.insert(0, prefix);
		}
	}

	bool contains(const std::string &paramName) const {
		if (get(paramName)) {
			return true;
		}
		return false;
	}

	const PluginAttr *get(const std::string &paramName) const {
		if (pluginAttrs.count(paramName)) {
			const auto pIt = pluginAttrs.find(paramName);
			return &pIt->second;
		}
		return nullptr;
	}

	PluginAttr *get(const std::string &paramName) {
		if (pluginAttrs.count(paramName)) {
			return &pluginAttrs[paramName];
		}
		return nullptr;
	}

	void add(const PluginAttr &attr, const float &time=0.0f) {
		pluginAttrs[attr.attrName] = attr;
	}

	void add(const std::string &attrName, const AttrValue &attrValue, const float &time=0.0f) {
		add(PluginAttr(attrName, attrValue), time);
	}

	void del(const std::string &attrName) {
		auto delIt = pluginAttrs.find(attrName);
		pluginAttrs.erase(delIt);
	}

	void showAttributes() const {
//		PRINT_INFO_EX("Plugin \"%s.%s\" parameters:",
//		              pluginID.c_str(), pluginName.c_str());

		for (const auto &pIt : pluginAttrs) {
			const PluginAttr &p = pIt.second;
//			PRINT_INFO_EX("  %s [%s]",
//			              p.attrName.c_str(), p.attrValue.getTypeAsString());
		}
	}
};

} // namespace VRayForBlender

#endif // VRAY_FOR_BLENDER_PLUGIN_EXPORTER_H
