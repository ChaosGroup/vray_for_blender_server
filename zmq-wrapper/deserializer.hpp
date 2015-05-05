#ifndef _DESERIALIZER_HPP_
#define _DESERIALIZER_HPP_

#include "base_types.h"

class DeserializerStream {
public:
	DeserializerStream(const char * data, int size): first(data), last(data + size), current(data) {
	}

	void rewid() {
		current = first;
	}

	int getSize() const {
		return last - first;
	}

	bool read(char * where, int size) {
		if (forward(size)) {
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

template <typename T>
DeserializerStream & operator>>(DeserializerStream & stream, T* & value) {
	static_assert(false, "Dont deserialize to bare pointers");
}

template <>
inline DeserializerStream & operator>>(DeserializerStream & stream, const std::string & value) {
	int size;
	stream >> size;

	// either push back char by char, or do this
	value = std::string(stream.getCurrent(), size);
	stream.forward(size);

	return stream;
}

template <>
inline DeserializerStream & operator>> (DeserializerStream & stream, const VRayBaseTypes::AttrPlugin & plugin) {
	return stream >> plugin.plugin;
}

template <typename Q>
inline DeserializerStream & operator>>(DeserializerStream & stream, const VRayBaseTypes::AttrList<Q> & list) {
	list.init();
	int size;
	stream >> size;

	for (int c = 0; c < size; ++c) {
		Q item;
		stream >> item;
		list.append(item);
	}

	return stream;
}

template <>
inline DeserializerStream & operator>>(DeserializerStream & stream, const VRayBaseTypes::AttrMapChannels & map) {
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
inline DeserializerStream & operator<<(DeserializerStream & stream, const VRayBaseTypes::AttrInstancer::Item & instItem) {
	return stream >> instItem.index >> instItem.tm >> instItem.vel >> instItem.node;
}

template <>
inline DeserializerStream & operator>>(DeserializerStream & stream, const VRayBaseTypes::AttrInstancer & inst) {
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





#endif // _DESERIALIZER_HPP_
