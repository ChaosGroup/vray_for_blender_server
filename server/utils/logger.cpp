#include "logger.h"


void Logger::setCallback(Logger::StringCb cb) {
	scb = cb;
}

void Logger::log(const std::string & msg) {
	Logger::log(Level::Debug, msg);
}

void Logger::log(Logger::Level lvl, const std::string & msg) {
	Logger::getInstance().scb(lvl, msg);
}

Logger & Logger::getInstance() {
	static Logger l;
	return l;
}
