#ifndef LOGGER_H
#define LOGGER_H

#include <functional>
#include <strstream>
#include <string>

class Logger {
public:
	enum Level {
		Info, Debug, Warning, Error
	};

	typedef std::function<void(Level, const std::string &)> StringCb;
	void setCallback(StringCb cb);

	template <typename T, typename ... R>
	static void log(Level lvl, const std::string & msg, const T & val, const R ... rest) {
		std::stringstream stream(msg, std::ios_base::ate | std::ios_base::in | std::ios_base::out);
		stream << ' ' << val;
		log(lvl, stream.str(), rest...);
	}

	static void log(Level lvl, const std::string & msg);

	/// default Level::Debug
	static void log(const std::string & msg);

	static Logger & getInstance();
private:
	StringCb scb;
};



#endif // LOGGER_H