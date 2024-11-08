#include "IPAddress.h"

/* Получение локального IP-адреса */
bool getMyIP(IPv4 &myIP)
{
    char szBuffer[1024];

    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 0);
    if (::WSAStartup(wVersionRequested, &wsaData) != 0)
    {
        return false;
    }

    if (gethostname(szBuffer, sizeof(szBuffer)) == SOCKET_ERROR)
    {
        WSACleanup();
        return false;
    }

    struct hostent *host = gethostbyname(szBuffer);
    if (host == NULL)
    {
        WSACleanup();
        return false;
    }

    myIP.b1 = ((struct in_addr *)(host->h_addr))->S_un.S_un_b.s_b1;
    myIP.b2 = ((struct in_addr *)(host->h_addr))->S_un.S_un_b.s_b2;
    myIP.b3 = ((struct in_addr *)(host->h_addr))->S_un.S_un_b.s_b3;
    myIP.b4 = ((struct in_addr *)(host->h_addr))->S_un.S_un_b.s_b4;

    WSACleanup();
    return true;
}

/* Конвертация IP-адреса в строку */
std::string convertIPtoString(IPv4 &ip)
{
    std::stringstream stream;
    stream << (short)ip.b1 << "." << (short)ip.b2 << "." << (short)ip.b3 << "." << (short)ip.b4;

    return stream.str();
}