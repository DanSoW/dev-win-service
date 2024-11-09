#pragma once

#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include "../logger/logger.h"

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "kernel32.lib")

// Имя службы
#define SVCNAME TEXT("DevWinSc")
// Краткое описание службы
#define SVCDISPLAY TEXT("Разрабатываемая служба DevWinSc")
// Полное описание службы
#define SVCDESCRIPTION TEXT("Данная служба разрабатыватся для туториала на Habr.ru")

// Преобразование const char* в char*
#define WITHOUT_CONST(x) const_cast<char*>(x)

// Состояние службы
extern SERVICE_STATUS          gSvcStatus;
// Дескриптор состояния службы
extern SERVICE_STATUS_HANDLE   gSvcStatusHandle;
// Дескриптор флага остановки работы службы
extern HANDLE                  ghSvcStopEvent;

/* Функция установки службы */
VOID SvcInstall(void);

VOID __stdcall DoStartSvc(void);
VOID __stdcall DoStopSvc(void);
VOID __stdcall DoDeleteSvc(void);
BOOL __stdcall StopDependentServices(SC_HANDLE&, SC_HANDLE&);

/* Обработчик служебных сообщений от SCM */
VOID WINAPI SvcCtrlHandler(DWORD);

/* Сообщение текущего статуса службы SCM */
VOID ReportSvcStatus(DWORD, DWORD, DWORD);