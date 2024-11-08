#include "../net/IPAddress.h"
#include "logger.h"

Logger::Logger(std::string title, std::string path) : title{title}, path{path}
{
}

Logger::~Logger()
{
	if (this->fileout.is_open())
	{
		this->fileout.close();
	}
}

/* Set new logger path */
void Logger::setPath(std::string path)
{
	if (this->fileout.is_open())
	{
		this->fileout.close();
	}

	this->filename = "";
	this->path = path;
}

/* Установка максимального размера файла логгера */
void Logger::setMaxSize(std::int64_t size)
{
	this->maxSize = size * 1024 * 1024;
}

/* Запись данных в лог-файл */
bool Logger::writeData(std::string text, std::string comment)
{
	// Блокировка мьютекса
	std::lock_guard<std::mutex> lock(this->mtx);

#if (DEBUG == 1)
	std::cout << text << std::endl;
#endif

	if (this->prepareFile())
	{
		time_t date = time(nullptr);
		struct tm dateTm = *localtime(&date);

		this->fileout << timePointToString<double, 3>(std::chrono::system_clock::now())
					  << " " << text << comment << std::endl;
		return true;
	}
	else
	{
		throw LogException("Возникла ошибка при подготовке файла", this->filename);
	}

	return false;
}

void Logger::write(std::string line)
{
	this->writeData(line);
}

Logger &Logger::operator<<(const std::string &line)
{
	this->writeData(line);

	return *this;
}

Logger &Logger::operator<<(LogMsg &msg)
{
	this->writeData(msg());

	return *this;
}

/* Подготовка файла для записи логов */
bool Logger::prepareFile()
{
	bool b = false;

	time_t current = time(nullptr);
	struct tm dateTm = *localtime(&this->date);
	struct tm dateTmCurrent = *localtime(&current);

	if (!(dateEqual(dateTm, dateTmCurrent)))
	{
		this->date = current;
		this->part = 0;
		b = true;

		dateTm = dateTmCurrent;
	}

	if ((!b) && !this->filename.empty() && std::filesystem::exists(this->filename) && (fs::file_size(std::filesystem::path(this->filename)) > this->maxSize))
	{
		b = true;

		if (++this->part == USHRT_MAX)
		{
			return false;
		}
	}

	b = b || !this->fileout.is_open();

	if (b)
	{
		if (this->fileout.is_open())
		{
			this->fileout.close();
		}

		std::string partStr = (this->part > 0) ? "~" + std::to_string(this->part) : "";
		std::string dateStr = dateToString(dateTm);

		this->filename = this->path + DIR_SEPARATOR + dateStr + DIR_SEPARATOR + APP_NAME + "-" + APP_VERSION + DIR_SEPARATOR + partStr + this->title + "_" + dateStr + ".log";

		std::vector<std::filesystem::path> dirs = {
			this->path,
			this->path + DIR_SEPARATOR + dateStr,
			this->path + DIR_SEPARATOR + dateStr + DIR_SEPARATOR + APP_NAME + "-" + APP_VERSION};

		for (const auto &i : dirs)
		{
			if (!std::filesystem::is_directory(i))
			{
				std::filesystem::create_directory(i);
			}
		}

		this->fileout.open(this->filename, std::ios::app | std::ios::ate);

		/* Запись заголовка в файл лога */
		if (fs::file_size(std::filesystem::path(this->filename)) == 0)
		{
			IPv4 ip;
			getMyIP(ip);

			std::string appInfo = "Пользователь@" + getMyComputerName() + "(" + convertIPtoString(ip) + ")";

			this->fileout << "/*" << std::endl
						  << " * " << appInfo << std::endl
						  << " * " << currentDateTime() << std::endl
						  << " */" << std::endl
						  << std::endl;
		}

		if (this->fileout.is_open())
		{
			return true;
		}

		return false;
	}

	return true;
}

extern Logger &logger = LoggerSingleton::GetInstance(FILE_NAME_LOG, LOG_PATH);