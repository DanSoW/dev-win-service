#include "log-msg.h"

LogMsg::LogMsg() : message{""} {}

LogMsg::LogMsg(const std::string& msg) : message{ msg } {}

std::string LogMsg::operator() () {
	return this->message;
}

std::string LogMsg::getMessage() {
	return this->message;
}