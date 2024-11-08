#include "LogException.h"

LogException::LogException(const char* msg) :
	message{ msg } {}

LogException::LogException(const std::string& msg, const std::string& filepath) {
	this->message = msg + ": " + filepath;
}

const char* LogException::what() const throw() {
	return message.c_str();
}