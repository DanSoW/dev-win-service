#include "system-info.h"

/* Получение имени компьютера */
std::string getMyComputerName()
{
    char buffer[256];
    unsigned long size = 256;

    GetComputerName(buffer, &size);

    return std::string(buffer);
}