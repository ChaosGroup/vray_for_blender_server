#ifndef LOGGER_H
#define LOGGER_H

#include <functional>
#include <sstream>
#include <string>

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <windows.h>
#else
	#include <dlfcn.h>
#endif
#include <vraysdk.hpp>
#include <base_types.h>

namespace LoggerFormat {


// All hookFormat functions are used to print various values in form that could be used for compilation
template <typename T>
void hookFormat(std::ostream & out, const T & val) {
	out << val;
}


template <>
inline void hookFormat(std::ostream & out, const float & val) {
	if (std::fabs(std::roundf(val) - val) < 0.001f) {
		out << static_cast<int>(std::roundf(val)) << '.';
	} else {
		out << val;
	}
	out << 'f';
}

template <>
inline void hookFormat(std::ostream & out, const VRay::Color & val) {
	out << "Color(";
	hookFormat(out, val.r);
	out << ',';
	hookFormat(out, val.g);
	out << ',';
	hookFormat(out, val.b);
	out << ")";
}

template <>
inline void hookFormat(std::ostream & out, const VRay::Vector & val) {
	out << "Vector(";
	hookFormat(out, val.x);
	out << ',';
	hookFormat(out, val.y);
	out << ',';
	hookFormat(out, val.z);
	out << ")";
}

template <>
inline void hookFormat(std::ostream & out, const VRayBaseTypes::AttrVector & val) {
	hookFormat(out, *reinterpret_cast<const VRay::Vector*>(&val));
}

template <>
inline void hookFormat(std::ostream & out, const VRay::Matrix & val) {
	out << "Matrix(";
	hookFormat(out, val.v0);
	out << ',';
	hookFormat(out, val.v1);
	out << ',';
	hookFormat(out, val.v2);
	out << ")";
}

template <>
inline void hookFormat(std::ostream & out, const VRay::Transform & val) {
	out << "Transform(";
	hookFormat(out, val.matrix);
	out << ',';
	hookFormat(out, val.offset);
	out << ")";
}

template <typename Q>
inline void hookFormat(std::ostream & out, const std::vector<Q> & arr) {
	out << "ValueList{";
	if (!arr.empty()) {
		out << "Value(";
		hookFormat(out, arr[0]);
		out << ')';
	}

	for (int c = 1; c < arr.size(); ++c) {
		out << ",Value(";
		hookFormat(out, arr[c]);
		out << ')';
	}
	out << "}";
}

template <>
inline void hookFormat(std::ostream & out, const std::vector<VRayBaseTypes::AttrPlugin> & arr) {
	out << "ValueList{";
	if (!arr.empty()) {
		out << "Value(\"";
		hookFormat(out, arr[0].plugin);
		out << "\")";
	}

	for (int c = 1; c < arr.size(); ++c) {
		out << ",Value(\"";
		hookFormat(out, arr[c].plugin);
		out << "\")";
	}
	out << "}";
}
} // namespace LoggerFormat

/// Global singleton class used for logging
/// All logged messages have a Logger::Level asociated with them that is printed as text in the output
/// One exceptions is Level::Info which is used to dump compilable code of what the exported did with appsdk
class Logger {
public:
	enum Level {
		Info, Debug, Warning, Error, None
	};

	/// Callback for each constructed message by the logger
	typedef std::function<void(Level, const std::string &)> StringCb;

	/// Set the current callback
	void setCallback(StringCb cb);

	/// Set current level - used to filter Level::Info if not required
	/// @lvl - the desiered Level
	void setCurrentlevel(Level lvl) { currentLevel = lvl; }

	/// Return the current log level
	Level getCurrentLevel() const { return currentLevel; }

	/// Multiple argument function used to conviniently join different type of arguments in one message
	/// @lvl - the level for this particular log message
	/// @rest ... - parts of the message
	template <typename ... R>
	static void log(Level lvl, const R & ... rest) {
		if (lvl == Info && getInstance().currentLevel != Info) {
			// skip Info if not required
			return;
		}
		std::stringstream stream("", std::ios_base::ate | std::ios_base::in | std::ios_base::out);
		logImpl(lvl, stream, rest...);
	}

	/// Log message with default Debug level
	static void log(const std::string & msg) {
		log(Debug, msg);
	}

	Logger(Logger && other)
		: isBuffered(other.isBuffered)
		, bufferedMessages(std::move(other.bufferedMessages))
		, currentLevel(other.currentLevel)
		, msgCallback(other.msgCallback) {}

	void log(const Logger & buffered);
	Logger makeBuffered();

	/// Get the Logger instace
	static Logger & getInstance();
private:
	Logger();

	/// Implementation of Logger::log without any copies of it's arguments
	template <typename T, typename ... R>
	static void logImpl(Level lvl, std::stringstream & stream, const T & val, const R & ... rest) {
		if (lvl == Info && getInstance().currentLevel != Info) {
			// skip Info if not required
			return;
		}
		if (lvl != Info) {
			stream << ' ';
		}
		LoggerFormat::hookFormat(stream, val);
		logImpl(lvl, stream, rest...);
	}

	/// Base case for logImpl
	static void logImpl(Level lvl, std::stringstream & msg);

	bool isBuffered; ///< If set to true all messages will be buffered in instance
	std::vector<std::pair<Level, std::string>> bufferedMessages; ///< Serves as alternative destination for messages (instead of callback)
	Level currentLevel; ///< Current log Level
	StringCb msgCallback; ///< Current callback
};


#endif // LOGGER_H
