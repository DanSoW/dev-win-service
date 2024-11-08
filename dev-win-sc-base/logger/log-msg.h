#pragma once

#include <string>
#include <sstream>

/* Класс для сборки сообщений логгера */
class LogMsg {
private:
	std::string message;

public:
	LogMsg();
	LogMsg(const std::string&);

public:
	std::string operator() ();

	template<typename T>
	LogMsg& operator<<(T value) {
		std::stringstream stream;
		stream << value;

		this->message += stream.str();
		return *this;
	}

public:
	std::string getMessage();
};