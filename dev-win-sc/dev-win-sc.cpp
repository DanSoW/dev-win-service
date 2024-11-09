#include "dev-win-sc.h"
#include "service/WinService.h"

VOID SvcInit(DWORD, LPTSTR *);
VOID WINAPI SvcMain(DWORD, LPTSTR *);

// Точка входа в приложение
int __cdecl _tmain(int argc, TCHAR *argv[])
{
    // Добавление поддержки русского языка в консоли
    setlocale(LC_ALL, "ru");

    if (lstrcmpi(argv[1], TEXT("/install")) == 0)
    {
        logger << "Запуск установки службы ...";

        SvcInstall();
        return 0;
    }
    else if (lstrcmpi(argv[1], TEXT("/start")) == 0)
    {
        logger << "Запуск службы ...";

        DoStartSvc();
        return 0;
    }
    else if (lstrcmpi(argv[1], TEXT("/stop")) == 0)
    {
        logger << "Запуск остановки службы ...";

        DoStopSvc();
        return 0;
    }
    else if (lstrcmpi(argv[1], TEXT("/uninstall")) == 0)
    {
        logger << "Запуск деинсталляции службы ...";

        DoDeleteSvc();
        return 0;
    }
    else
    {
        // Описание точки входа для SCM
        SERVICE_TABLE_ENTRY DispatchTable[] =
            {
                {WITHOUT_CONST(SVCNAME), (LPSERVICE_MAIN_FUNCTION)SvcMain},
                {NULL, NULL}};

        logger << "Запуск StartServiceCtrlDispatcher...";
        if (!StartServiceCtrlDispatcher(DispatchTable))
        {
            DWORD lastError = GetLastError();

            switch (lastError)
            {
            case ERROR_FAILED_SERVICE_CONTROLLER_CONNECT:
            {
                logger << "ERROR_FAILED_SERVICE_CONTROLLER_CONNECT";
                break;
            }
            case ERROR_INVALID_DATA:
            {
                logger << "ERROR_INVALID_DATA";
                break;
            }
            case ERROR_SERVICE_ALREADY_RUNNING:
            {
                logger << "ERROR_SERVICE_ALREADY_RUNNING";
                break;
            }
            }
        }
    }

    return 0;
}

/* Точка входа в службу */
VOID WINAPI SvcMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
    // Регистрация обработчика управляющих сообщений для службы
    gSvcStatusHandle = RegisterServiceCtrlHandler(
        SVCNAME,         // Имя службы
        SvcCtrlHandler); // Обработчик

    if (!gSvcStatusHandle)
    {
        logger << (LogMsg() << "Не получилось зарегистрировать обработчик для службы (" << GetLastError() << ")");
        return;
    }

    // Определяем значение в структуре статуса службы
    gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS; // Тип службы
    gSvcStatus.dwServiceSpecificExitCode = 0;             // ExitCode

    logger << "Служба будет находится в состоянии SERVICE_START_PENDING в течении 3 секунд";
    ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

    logger << "Запуск функции SvcInit";
    SvcInit(dwArgc, lpszArgv);
}

/* Функция, в которой описана основная работа службы */
VOID SvcInit(DWORD dwArgc, LPTSTR *lpszArgv)
{
    logger << (LogMsg() << "(SvcInit) Thread ID: " << std::this_thread::get_id() << " " << GetCurrentProcessId());

    // Создание события, которое будет сигнализировать об остановке службы
    ghSvcStopEvent = CreateEvent(
        NULL,  // Аттрибуты события
        TRUE,  // Ручной сброс события
        FALSE, // Не сигнализировать сразу после создания события
        NULL); // Имя события

    if (ghSvcStopEvent == NULL)
    {
        DWORD lastError = GetLastError();

        logger << (LogMsg() << "CreateEvent вернул NULL (" << lastError << ")");

        // Отправка в SCM состояния об остановке службы
        ReportSvcStatus(SERVICE_STOPPED, GetLastError(), 0);
        return;
    }

    // Отправка статуса SCM о работе службы
    ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);
    logger << "Работа службы";

    int count = 1;

    // Работа бесконечного цикла до тех пор, пока не будет подан сигнал о завершении работы службы
    while (WaitForSingleObject(ghSvcStopEvent, 0) != WAIT_OBJECT_0)
    {
        logger << (LogMsg() << "[WORKING] Счётчик: " << count++);
        Sleep(2000);
    }

    logger << "Завершение работы службы";
    // Отправка статуса SCM о завершении службы
    ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
}
