#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <string>
#include <fstream>
#include <ctime>
#include <mutex>
#include <filesystem>
#include <algorithm>
#include "./log-msg.h"
#include "../config.h"
#include "../utils/system-info/system-info.h"
#include "../utils/utils.h"
#include "../exceptions/LogException.h"

namespace fs = std::filesystem;

/* Класс логгера */
class Logger
{
private:
	const std::string title;
	std::string path;

	// Максимальный размер файла лога
	std::int64_t maxSize = MAX_LOG_SIZE * 1024 * 1024;

	std::string filename = "";
	std::fstream fileout;
	std::time_t date = time(nullptr);

	// Часть записываемого лога
	std::uint16_t part = 0;

	// Мьютекс для потокобезопасной записи логов в файл
	std::mutex mtx;

public:
	Logger(std::string title, std::string path);
	virtual ~Logger();

public:
	bool writeData(std::string text, std::string comment = "");
	void write(std::string line);
	void setPath(std::string path);
	void setMaxSize(std::int64_t);

public:
	Logger &operator<<(const std::string &);
	Logger &operator<<(LogMsg &);

private:
	bool prepareFile();
};

/* Определение Singleton для логгера (обёртка) */
class LoggerSingleton
{
public:
	static Logger &GetInstance(const std::string &title, const std::string &path)
	{
		static Logger instance = Logger(title, path);
		return instance;
	}

	LoggerSingleton() = delete;
	LoggerSingleton(const LoggerSingleton &) = delete;
	const LoggerSingleton &operator=(const LoggerSingleton &) = delete;
};

// Экспортирование логгера
extern Logger &logger;