#include "logger.h"
#include <algorithm>

Logger::Logger()
	: forceInfoLog(false)
	, currentLevel(Debug) {}

void Logger::setCallback(Logger::StringCb cb) {
	scb = cb;
}

void Logger::logImpl(Logger::Level lvl, std::stringstream & stream) {
	auto & inst = getInstance();
	if (lvl == Info && inst.currentLevel != Info && !inst.forceInfoLog) {
		return;
	}
	auto msg = stream.str();
	// just hack replace all slashes to fix paths
	std::replace(msg.begin(), msg.end(), '\\', '/');
	inst.scb(lvl, msg);
}

Logger & Logger::getInstance() {
	static Logger l;
	return l;
}
