#include "logger.h"
#include <algorithm>

Logger::Logger()
	: currentLevel(Debug) {}

void Logger::setCallback(Logger::StringCb cb) {
	msgCallback = cb;
}

/// Base case for the logImpl - calls the callback with the generated string and Level
void Logger::logImpl(Logger::Level lvl, std::stringstream & stream) {
	if (lvl == APIDump && currentLevel != APIDump) {
		return;
	}
	auto msg = stream.str();
	// just hack replace all slashes to fix paths
	std::replace(msg.begin(), msg.end(), '\\', '/');
	if (isBuffered) {
		bufferedMessages.push_back(std::make_pair(lvl, msg));
	} else {
		msgCallback(lvl, msg);
	}
}

Logger & Logger::getInstance() {
	static Logger l;
	return l;
}

void Logger::log(const Logger & buffered) {
	for (const auto & msg : buffered.bufferedMessages) {
		msgCallback(msg.first, msg.second);
	}
}

Logger Logger::makeBuffered() {
	Logger buff;
	buff.currentLevel = currentLevel;
	buff.isBuffered = true;
	return buff;
}
