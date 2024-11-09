#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <algorithm>
#include <vector>
#include <string>
#include <cstring>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <cmath>

std::vector<std::string> split(const std::string& str, const std::string& delimiters);
bool dateEqual(struct tm date1, struct tm date2);
std::string dateToString(struct tm date, std::string sep = "");
std::string currentDateTime();

/* Конвертация структуры TimePoint в строку специфического формата (дата и время)*/
template<
	typename Double = double,
	std::size_t Precision = std::numeric_limits<Double>::digits10,
	typename TimePoint
>
	requires std::is_floating_point_v<Double> && (Precision <= std::numeric_limits<Double>::digits10)
inline std::string timePointToString(const TimePoint& timePoint) throw()
{
	auto seconds = Double(timePoint.time_since_epoch().count())
		* TimePoint::period::num / TimePoint::period::den;
	auto const zeconds = std::modf(seconds, &seconds);

	std::time_t tt(seconds);
	std::ostringstream oss;

	auto const tm = std::localtime(&tt);
	if (!tm) {
		throw std::runtime_error(std::strerror(errno));
	}

	oss << std::put_time(tm, "%H:%M:")
		<< std::setw(Precision + 3) << std::setfill('0')
		<< std::fixed << std::setprecision(Precision)
		<< tm->tm_sec + zeconds;

	if (!oss) {
		throw std::runtime_error("Ошибка конвертации TimePoint в String");
	};

	return oss.str();
}

/* Конвертация строки в структуру TimePoint */
template<typename TimePoint>
TimePoint fromString(const std::string& str) throw()
{
	std::istringstream iss{ str };

	std::tm tm{};

	if (!(iss >> std::get_time(&tm, "%Y-%b-%d %H:%M:%S"))) {
		throw std::invalid_argument("get_time");
	}

	TimePoint timePoint{ std::chrono::seconds(std::mktime(&tm)) };

	if (iss.eof()) {
		return timePoint;
	}

	double zz;
	if (iss.peek() != '.' || !(iss >> zz)) {
		throw std::invalid_argument("decimal");
	}

	using hr_clock = std::chrono::high_resolution_clock;
	std::size_t zeconds = zz * hr_clock::period::den / hr_clock::period::num;
	return timePoint += hr_clock::duration(zeconds);
}

