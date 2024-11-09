#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCKAPI_ 

#include <string>
#include <stdio.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <sstream>

#pragma comment(lib, "Ws2_32.lib")

struct IPv4
{
    UCHAR b1, b2, b3, b4;
};

bool getMyIP(IPv4& myIP);
std::string convertIPtoString(IPv4&);
