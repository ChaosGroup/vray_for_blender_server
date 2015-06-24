#ifndef _DESERIALIZER_HPP_
#define _DESERIALIZER_HPP_

#include "base_types.h"

class DeserializerStream {
public:
	DeserializerStream() = delete;

	DeserializerStream(const char * data, int size): first(data), last(data + size), current(data) {
		forward(1);
	}

	bool hasMore() const {
		return current < last;
	}

	void rewind() {
		current = first;
	}

	int getSize() const {
		return last - first;
	}

	int getOffset() const {
		return last - current;
	}

	bool read(char * where, int size) {
		if (!forward(size)) {
			return false;
		}
		memcpy(where, current - size, size);
		return true;
	}

	const char * getCurrent() const {
		return current;
	}

	bool forward(int size) {
		if (current + size > last) {
			return false;
		}
		current += size;
		return true;
	}

private:
	const char * first, * current, * last;
};

template <typename T>
DeserializerStream & operator>>(DeserializerStream & stream, T & value) {
	stream.read(reinterpret_cast<char*>(&value), sizeof(value));
	return stream;
}

template <>
inline DeserializerStream & operator>>(DeserializerStream & stream, std::string & value) {
	int size;
	stream >> size;

	// either push back char by char, or do this
	value = std::string(stream.getCurrent(), size);
	stream.forward(size);

	return stream;
}

template <>
inline DeserializerStream & operator>>(DeserializerStream & stream, VRayBaseTypes::AttrSimpleType<std::string> & value) {
	stream >> value.m_Value;
	return stream;
}

template <>
inline DeserializerStream & operator>> (DeserializerStream & stream, VRayBaseTypes::AttrPlugin & plugin) {
	return stream >> plugin.plugin >> plugin.output;
}

template <typename Q>
inline DeserializerStream & operator>>(DeserializerStream & stream, VRayBaseTypes::AttrList<Q> & list) {
	list.init();
	int size;
	stream >> size;

	list.getData()->resize(size);
	memcpy(list.getData()->data(), stream.getCurrent(), size * sizeof(Q));
	stream.forward(size * sizeof(Q));

	return stream;
}

template <>
inline DeserializerStream & operator>>(DeserializerStream & stream, VRayBaseTypes::AttrList<VRayBaseTypes::AttrPlugin> & list) {
	list.init();
	int size;
	stream >> size;
	for (int c = 0; c < size; ++c) {
		VRayBaseTypes::AttrPlugin item;
		stream >> item;
		list.append(item);
	}
	return stream;
}

template <>
inline DeserializerStream & operator>>(DeserializerStream & stream, VRayBaseTypes::AttrList<std::string> & list) {
	list.init();
	int size;
	stream >> size;
	for (int c = 0; c < size; ++c) {
		std::string item;
		stream >> item;
		list.append(item);
	}
	return stream;
}

template <>
inline DeserializerStream & operator>>(DeserializerStream & stream, VRayBaseTypes::AttrMapChannels & map) {
	map.data.clear();
	int size;
	stream >> size;
	for (int c = 0; c < size; ++c) {
		std::string key;
		VRayBaseTypes::AttrMapChannels::AttrMapChannel channel;
		stream >> key >> channel.vertices >> channel.faces >> channel.name;
		map.data.emplace(key, channel);
	}
	return stream;
}


template <>
inline DeserializerStream & operator>>(DeserializerStream & stream, VRayBaseTypes::AttrInstancer::Item & instItem) {
	return stream >> instItem.index >> instItem.tm >> instItem.vel >> instItem.node;
}

template <>
inline DeserializerStream & operator>>(DeserializerStream & stream, VRayBaseTypes::AttrInstancer & inst) {
	int size;
	stream >> size;
	inst.data.init();
	for (int c = 0; c < size; ++c) {
		VRayBaseTypes::AttrInstancer::Item item;
		stream >> item;
		inst.data.append(item);
	}
	return stream;
}

template <>
inline DeserializerStream & operator>>(DeserializerStream & stream, VRayBaseTypes::AttrImage & image) {
	int size, width, height;
	VRayBaseTypes::AttrImage::ImageType type;
	stream >> type >> size >> width >> height;
	image.set(stream.getCurrent(), size, type, width, height);
	stream.forward(size);
	return stream;
}




#endif // _DESERIALIZER_HPP_
