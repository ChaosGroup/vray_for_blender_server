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
	ValueTypeDouble,
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


template <typename T>
struct AttrSimpleType {
	ValueType getType() const;
	AttrSimpleType(): m_Value() {}
	AttrSimpleType(const T & val): m_Value(val) {}

	T m_Value;
};

template <>
inline ValueType AttrSimpleType<int>::getType() const {
	return ValueType::ValueTypeInt;
}

template <>
inline ValueType AttrSimpleType<float>::getType() const {
	return ValueType::ValueTypeFloat;
}

template <>
inline ValueType AttrSimpleType<double>::getType() const {
	return ValueType::ValueTypeDouble;
}

template <>
inline ValueType AttrSimpleType<std::string>::getType() const {
	return ValueType::ValueTypeString;
}

struct AttrImage {
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


struct AttrColor {

	ValueType getType() const {
		return ValueType::ValueTypeColor;
	}

	AttrColor():
	    r(0.0f),
	    g(0.0f),
	    b(0.0f)
	{}

	AttrColor(const float &r, const float &g, const float &b):
	    r(r),
	    g(g),
	    b(b)
	{}

	AttrColor(float c):
		r(c),
		g(c),
		b(c)
	{}

	AttrColor(float color[4]):
		r(color[0]),
		g(color[1]),
		b(color[2])
	{}

	float r;
	float g;
	float b;
};


struct AttrAColor {

	ValueType getType() const {
		return ValueType::ValueTypeAColor;
	}

	AttrAColor():
	    alpha(1.0f)
	{}

	AttrAColor(const AttrColor &c, const float &a=1.0f):
	    color(c),
	    alpha(a)
	{}

	AttrColor  color;
	float      alpha;
};


struct AttrVector {

	ValueType getType() const {
		return ValueType::ValueTypeVector;
	}

	AttrVector():
	    x(0.0f),
	    y(0.0f),
	    z(0.0f)
	{}

	AttrVector(float vector[3]):
		x(vector[0]),
		y(vector[1]),
		z(vector[2])
	{}

	AttrVector(const float &_x, const float &_y, const float &_z):
		x(_x),
		y(_y),
		z(_z)
	{}

	float operator * (const AttrVector other) {
		return x * other.x + y * other.y + z * other.z;
	}

	AttrVector operator - (const AttrVector other) {
		return AttrVector(x - other.x, y - other.y, z - other.z);
	}

	bool operator == (const AttrVector other) {
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


struct AttrVector2 {

	ValueType getType() const {
		return ValueType::ValueTypeVector2;
	}

	AttrVector2():
	    x(0.0f),
	    y(0.0f)
	{}

	AttrVector2(float vector[2]):
		x(vector[0]),
		y(vector[1])
	{}

	float x;
	float y;
};


struct AttrMatrix {

	ValueType getType() const {
		return ValueType::ValueTypeMatrix;
	}

	AttrMatrix() {}

	AttrMatrix(float tm[3][3]):
	    v0(tm[0]),
	    v1(tm[1]),
	    v2(tm[2])
	{}

	AttrMatrix(float tm[4][4]):
	    v0(tm[0]),
	    v1(tm[1]),
	    v2(tm[2])
	{}

	AttrVector v0;
	AttrVector v1;
	AttrVector v2;
};


struct AttrTransform {

	ValueType getType() const {
		return ValueType::ValueTypeTransform;
	}

	AttrTransform() {}

	AttrTransform(float tm[4][4]):
	    m(tm),
	    offs(tm[3])
	{}

	AttrMatrix m;
	AttrVector offs;
};

struct AttrValue;
struct AttrPlugin {

	ValueType getType() const {
		return ValueType::ValueTypePlugin;
	}

	AttrPlugin() {}
	AttrPlugin(const std::string &name):
	    plugin(name)
	{}

	operator bool () const {
		return !plugin.empty();
	}

	AttrPlugin& operator=(const std::string &name) {
		plugin = name;
		return *this;
	}

	AttrPlugin & operator=(const AttrValue &);

	std::string output;
	std::string plugin;
};


template <typename T>
struct AttrList {
	typedef std::vector<T>              DataType;
	typedef boost::shared_ptr<DataType> DataArray;

	ValueType getType() const ;

	AttrList() {
		init();
	}

	AttrList(const int &size) {
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

typedef AttrList<int>         AttrListInt;
typedef AttrList<float>       AttrListFloat;
typedef AttrList<AttrColor>   AttrListColor;
typedef AttrList<AttrVector>  AttrListVector;
typedef AttrList<AttrVector2> AttrListVector2;
typedef AttrList<AttrPlugin>  AttrListPlugin;
typedef AttrList<std::string> AttrListString;


template <>
inline ValueType AttrListInt::getType() const {
	return ValueType::ValueTypeListInt;
}

template <>
inline ValueType AttrListFloat::getType() const {
	return ValueType::ValueTypeListFloat;
}

template <>
inline ValueType AttrListColor::getType() const {
	return ValueType::ValueTypeListColor;
}

template <>
inline ValueType AttrListVector::getType() const {
	return ValueType::ValueTypeListVector;
}

template <>
inline ValueType AttrListVector2::getType() const {
	return ValueType::ValueTypeListVector2;
}

template <>
inline ValueType AttrListPlugin::getType() const {
	return ValueType::ValueTypeListPlugin;
}

template <>
inline ValueType AttrListString::getType() const {
	return ValueType::ValueTypeListString;
}


struct AttrMapChannels {

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


struct AttrInstancer {

	ValueType getType() const {
		return ValueType::ValueTypeInstancer;
	}

	struct Item {
		int            index;
		AttrTransform  tm;
		AttrTransform  vel;
		AttrPlugin     node;
	};
	typedef AttrList<Item> Items;

	Items data;
};

struct AttrValue {
	typedef AttrList<AttrValue> AttrListValue;

	AttrValue():
		type(ValueTypeUnknown) {}

	AttrValue(const AttrValue &other) {
		*this = other;
	}

	AttrValue(const std::string &attrValue) {
		type = ValueTypeString;
		valString = attrValue;
	}

	AttrValue(const char *attrValue) {
		type = ValueTypeString;
		valString = attrValue;
	}

	AttrValue(const AttrPlugin attrValue) {
		type = ValueTypePlugin;
		valPlugin = attrValue;
	}

	AttrValue(const AttrPlugin attrValue, const std::string &output) {
		type = ValueTypePlugin;
		valPlugin = attrValue;
		valPlugin.output = output;
	}

	AttrValue(const AttrColor &c) {
		type = ValueTypeColor;
		valColor = c;
	}

	AttrValue(const AttrAColor &ac) {
		type = ValueTypeAColor;
		valAColor = ac;
	}

	AttrValue(const AttrVector &v) {
		type = ValueTypeVector;
		valVector = v;
	}

	AttrValue(const AttrMatrix &m) {
		type = ValueTypeMatrix;
		valMatrix = m;
	}

	AttrValue(const AttrTransform &attrValue) {
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

	AttrValue(const AttrMapChannels &attrValue) {
		type = ValueTypeMapChannels;
		valMapChannels = attrValue;
	}
	AttrValue(const AttrInstancer &attrValue) {
		type = ValueTypeInstancer;
		valInstancer = attrValue;
	}

	// TODO: Replace with single storage with reinterpret_cast<>
	int                 valInt;
	float               valFloat;
	AttrVector          valVector;
	AttrColor           valColor;
	AttrAColor          valAColor;

	std::string         valString;

	AttrMatrix          valMatrix;
	AttrTransform       valTransform;

	AttrPlugin          valPlugin;

	AttrListInt         valListInt;
	AttrListFloat       valListFloat;
	AttrListVector      valListVector;
	AttrListColor       valListColor;
	AttrListPlugin      valListPlugin;
	AttrListValue       valListValue;
	AttrListString      valListString;

	AttrMapChannels     valMapChannels;
	AttrInstancer       valInstancer;

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

	operator bool() const {
		bool valid = true;
		if (type == ValueTypeUnknown) {
			valid = false;
		} else if (type == ValueTypePlugin) {
			valid = !!(valPlugin);
		}
		return valid;
	}
};

inline AttrPlugin & AttrPlugin::operator = (const AttrValue & val) {
	if (val.type == ValueTypePlugin) {
		*this = val.valPlugin;
	}
	return *this;
}
} // namespace VRayBaseTypes

#endif // VRAY_FOR_BLENDER_BASE_TYPES_H
