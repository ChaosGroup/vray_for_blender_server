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

class Logger {
public:
	enum Level {
		Info, Debug, Warning, Error, None
	};
	typedef std::function<void(Level, const std::string &)> StringCb;

	void setCallback(StringCb cb);

	void setCurrentlevel(Level lvl) { currentLevel = lvl; }
	Level getCurrentLevel() const { return currentLevel; }

	void setForceInfoLog(bool flag) { forceInfoLog = flag; }
	bool getForceInfoLog() const { return forceInfoLog;  }

	template <typename ... R>
	static void log(Level lvl, const std::string & msg, const R & ... rest) {
		if (lvl == Info && getInstance().currentLevel != Info && !getInstance().forceInfoLog) {
			// skip Info if not required
			return;
		}
		std::stringstream stream(msg, std::ios_base::ate | std::ios_base::in | std::ios_base::out);
		logImpl(lvl, stream, rest...);
	}

	static void log(Level lvl, const std::string & msg) {
		if (lvl == Info && getInstance().currentLevel != Info && !getInstance().forceInfoLog) {
			// skip Info if not required
			return;
		}
		std::stringstream stream(msg, std::ios_base::ate | std::ios_base::in | std::ios_base::out);
		logImpl(lvl, stream, msg);
	}

	/// default Level::Debug
	static void log(const std::string & msg) {
		log(Debug, msg);
	}

	static Logger & getInstance();
private:
	Logger();

	template <typename T, typename ... R>
	static void logImpl(Level lvl, std::stringstream & stream, const T & val, const R & ... rest) {
		if (lvl == Info && getInstance().currentLevel != Info && !getInstance().forceInfoLog) {
			// skip Info if not required
			return;
		}
		if (lvl != Info) {
			stream << ' ';
		}
		hookFormat(stream, val);
		logImpl(lvl, stream, rest...);
	}

	static void logImpl(Level lvl, std::stringstream & msg);


	// Hook formatting so we log strings that are ready to be compiled

	template <typename T>
	static void hookFormat(std::ostream & out, const T & val) {
		out << val;
	}

	template <>
	static void hookFormat(std::ostream & out, const float & val) {
		if (std::fabs(std::roundf(val) - val) < 0.001f) {
			out << static_cast<int>(std::roundf(val)) << '.';
		} else {
			out << val;
		}
		out << 'f';
	}

	template <>
	static void hookFormat(std::ostream & out, const VRay::Color & val) {
		out << "Color(";
		hookFormat(out, val.r);
		out << ',';
		hookFormat(out, val.g);
		out << ',';
		hookFormat(out, val.b);
		out << ")";
	}

	template <>
	static void hookFormat(std::ostream & out, const VRay::Vector & val) {
		out << "Vector(";
		hookFormat(out, val.x);
		out << ',';
		hookFormat(out, val.y);
		out << ',';
		hookFormat(out, val.z);
		out << ")";
	}

	template <>
	static void hookFormat(std::ostream & out, const VRayBaseTypes::AttrVector & val) {
		hookFormat(out, *reinterpret_cast<const VRay::Vector*>(&val));
	}

	template <>
	static void hookFormat(std::ostream & out, const VRay::Matrix & val) {
		out << "Matrix(";
		hookFormat(out, val.v0);
		out << ',';
		hookFormat(out, val.v1);
		out << ',';
		hookFormat(out, val.v2);
		out << ")";
	}

	template <>
	static void hookFormat(std::ostream & out, const VRay::Transform & val) {
		out << "Transform(";
		hookFormat(out, val.matrix);
		out << ',';
		hookFormat(out, val.offset);
		out << ")";
	}

	template <typename Q>
	static void hookFormat(std::ostream & out, const std::vector<Q> & arr) {
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
	static void hookFormat(std::ostream & out, const std::vector<VRayBaseTypes::AttrPlugin> & arr) {
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

	bool     forceInfoLog;
	Level    currentLevel;
	StringCb scb;
};


#endif // LOGGER_H
