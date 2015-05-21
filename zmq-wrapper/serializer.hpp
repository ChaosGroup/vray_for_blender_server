#ifndef _SERIALIZER_HPP_
#define _SERIALIZER_HPP_

#include <vector>
#include <string>
#include "base_types.h"

class SerializerStream {
public:

	void write(const char * data, int size) {
		for (int c = 0; c < size; ++c) {
			stream.push_back(data[c]);
		}
	}

	int getSize() const {
		return stream.size();
	}

	char * getData() {
		return stream.data();
	}

private:
	std::vector<char> stream;
};


template <typename T>
inline SerializerStream & operator<<(SerializerStream & stream, const T & value) {
	stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
	return stream;
}

template <typename T>
inline SerializerStream & operator<<(SerializerStream & stream, const T* & value) {
	static_assert(false, "Dont serialize bare pointers");
}

template <>
inline SerializerStream & operator<<(SerializerStream & stream, const std::string & value) {
	int size = value.size();
	stream << size;
	stream.write(value.c_str(), size);
	return stream;
}

template <>
inline SerializerStream & operator<<(SerializerStream & stream, const VRayBaseTypes::AttrSimpleType<std::string> & value) {
	stream << value.m_Value;
	return stream;
}


template <>
inline SerializerStream & operator<<(SerializerStream & stream, const VRayBaseTypes::AttrPluginBase & plugin) {
	return stream << plugin.plugin;
}

template <typename Q>
inline SerializerStream & operator<<(SerializerStream & stream, const VRayBaseTypes::AttrListBase<Q> & list) {
	stream << list.getCount();
	if (!list.empty()) {
		for (auto & item : *(list.getData())) {
			stream << item;
		}
	}
	return stream;
}

template <>
inline SerializerStream & operator<<(SerializerStream & stream, const VRayBaseTypes::AttrMapChannelsBase & map) {
	stream << static_cast<int>(map.data.size());
	for (auto & pair : map.data) {
		stream << pair.first << pair.second.vertices << pair.second.faces << pair.second.name;
	}
	return stream;
}


template <>
inline SerializerStream & operator<<(SerializerStream & stream, const VRayBaseTypes::AttrInstancerBase::Item & instItem) {
	return stream << instItem.index << instItem.tm << instItem.vel << instItem.node;
}

template <>
inline SerializerStream & operator<<(SerializerStream & stream, const VRayBaseTypes::AttrInstancerBase & inst) {
	stream << inst.data.getCount();
	if (!inst.data.empty()) {
		for (auto & item : *(inst.data.getData())) {
			stream << item;
		}
	}
	return stream;
}


template <>
inline SerializerStream & operator<<(SerializerStream & stream, const VRayBaseTypes::AttrImage & image) {
	stream << image.imageType << image.size << image.width << image.height;
	stream.write(image.data.get(), image.size);
	return stream;
}


#endif // _SERIALIZER_HPP_
