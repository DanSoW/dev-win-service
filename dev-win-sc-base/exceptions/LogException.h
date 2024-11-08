#pragma once

#include <exception>
#include <iostream>
#include <string>

/* Исключение для логгера */
class LogException : public std::exception {
private:
	std::string message;

public:
	LogException(const char*);
	LogException(const std::string&, const std::string&);

public:
	const char* what() const throw();
};