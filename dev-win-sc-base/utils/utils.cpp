#include "utils.h"

/* Реализация функции split */
std::vector<std::string> split(const std::string& str, const std::string& delimiters) {
	std::vector<std::string> tokens;
	std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
	std::string::size_type pos = str.find_first_of(delimiters, lastPos);

	while (std::string::npos != pos || std::string::npos != lastPos) {
		tokens.push_back(str.substr(lastPos, pos - lastPos));

		lastPos = str.find_first_not_of(delimiters, pos);
		pos = str.find_first_of(delimiters, lastPos);
	}

	return tokens;
}

/* Функция для сравнения двух дат */
bool dateEqual(struct tm date1, struct tm date2) {
	bool year = date1.tm_year == date2.tm_year;
	bool month = date1.tm_mon == date2.tm_mon;
	bool day = date1.tm_mday == date2.tm_mday;

	return year && month && day;
}

/* Конвертация даты и времени в строку */
std::string dateToString(struct tm date, std::string sep) {
	int year = date.tm_year + 1900;
	int month = date.tm_mon + 1;
	int day = date.tm_mday;

	std::string str = std::to_string(year) + sep;

	if (month < 10) {
		str += "0" + std::to_string(month) + sep;
	}
	else {
		str += std::to_string(month) + sep;
	}

	if (day < 10) {
		str += "0" + std::to_string(day);
	}
	else {
		str += std::to_string(day);
	}

	return str;
}

/* Получение текущей даты и времени */
std::string currentDateTime() {
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	tstruct = *localtime(&now);

	strftime(buf, sizeof(buf), "%d.%m.%Y %X", &tstruct);

	return buf;
}