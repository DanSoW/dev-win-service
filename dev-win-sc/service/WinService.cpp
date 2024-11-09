#include "WinService.h"

SERVICE_STATUS gSvcStatus;
SERVICE_STATUS_HANDLE gSvcStatusHandle;
HANDLE ghSvcStopEvent = NULL;

/* Установка службы и её загрузка в базу данных служб */
VOID SvcInstall()
{
    // Дескриптор на менеджер управления службами
    SC_HANDLE schSCManager;
    // Дескриптор на конкретную службу (которую мы установим)
    SC_HANDLE schService;
    // Путь к исполняемому файлу службы (dev-win-sc.exe, который мы сначала запускаем как консольное приложение)
    TCHAR szUnquotedPath[MAX_PATH];

    // Структура полного описания службы
    SERVICE_DESCRIPTION sd;

    // Преобразование полного описания службы в LPTSTR
    LPTSTR szDesc = WITHOUT_CONST(SVCDESCRIPTION);

    // Определение пути до текущего исполняемого файла (dev-win-sc.exe)
    if (!GetModuleFileName(NULL, szUnquotedPath, MAX_PATH))
    {
        logger << (LogMsg() << "Не удалось установить службу (" << GetLastError() << ")");
        return;
    }

    // В случае, если путь содержит пробел, его необходимо заключить в кавычки, чтобы
    // он был правильно интерпретирован. Например,
    // "d:\my share\myservice.exe" следует указывать как
    // ""d:\my share\myservice.exe""
    TCHAR szPath[MAX_PATH];
    StringCbPrintf(szPath, MAX_PATH, TEXT("\"%s\""), szUnquotedPath);

    logger << (LogMsg() << "Путь к исполняемому файлу службы: " << szPath);

    // Получение дескриптора менеджера управления службами
    schSCManager = OpenSCManager(
        NULL,                   // Имя компьютера
        NULL,                   // Имя конкретной базы данных служб
        SC_MANAGER_ALL_ACCESS); // Права доступа (указываем все права)

    if (schSCManager == NULL)
    {
        logger << (LogMsg() << "OpenSCManager вернул NULL (" << GetLastError() << ")");
        return;
    }

    // Создание службы и получение её дескриптора
    schService = CreateServiceA(
        schSCManager,              // Дескриптор SCM
        SVCNAME,                   // Имя службы (отображается в диспетчере устройств)
        SVCDISPLAY,                // Краткое описание службы (отображается в диспетчере устройств и окне "Службы")
        SERVICE_ALL_ACCESS,        // Определение прав для службы (полный доступ)
        SERVICE_WIN32_OWN_PROCESS, // Тип службы
        SERVICE_DEMAND_START,      // Тип запуска
        SERVICE_ERROR_NORMAL,      // Тип контроля ошибки
        szPath,                    // Путь до исполняемого файла службы
        NULL,                      // Группа
        NULL,                      // Тег идентификатора
        NULL,                      // Зависимости
        NULL,                      // Стартовое имя (LocalSystem)
        NULL);                     // Пароль

    if (schService == NULL)
    {
        logger << (LogMsg() << "CreateService вернул NULL (" << GetLastError() << ")");

        // Закрытие дескриптора SCM
        CloseServiceHandle(schSCManager);
        return;
    }
    else
    {
        logger << "Служба успешно установлена";

        // Добавляем полное описание службы
        sd.lpDescription = szDesc;

        // Инициируем операцию изменения полного описания службы, которое сейчас есть в базе данных служб
        if (!ChangeServiceConfig2(
                schService,
                SERVICE_CONFIG_DESCRIPTION,
                &sd))
        {
            logger << "ChancheServiceConfig2 вернул NULL";
        }
        else
        {
            logger << "Полное описание службы успешно установлено";
        }
    }

    // Закрытие дескриптора службы и SCM
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
}

/* Деинсталляция службы */
VOID __stdcall DoDeleteSvc()
{
    // Дескриптор SCM и службы
    SC_HANDLE schSCManager;
    SC_HANDLE schService;

    // Получение дескриптора SCM
    schSCManager = OpenSCManager(
        NULL,                  
        NULL,                   
        SC_MANAGER_ALL_ACCESS);

    if (NULL == schSCManager)
    {
        logger << (LogMsg() << "OpenSCManager вернул NULL (" << GetLastError() << ")");
        return;
    }

    // Получение дескриптора службы
    schService = OpenService(
        schSCManager, 
        SVCNAME,      
        DELETE);      // Права на удаление

    if (schService == NULL)
    {
        logger << (LogMsg() << "OpenService вернул NULL (" << GetLastError() << ")");

        CloseServiceHandle(schSCManager);
        return;
    }

    // Удаление службы
    if (!DeleteService(schService))
    {
        logger << (LogMsg() << "DeleteService вернула NULL (" << GetLastError() << ")");
    }
    else
    {
        logger << "Служба успешно удалена";
    }

    // Освобождение занятых дескрипторов
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
}

/* Функция запуска службы */
VOID __stdcall DoStartSvc()
{
    // Информация о статусе службы
    SERVICE_STATUS_PROCESS ssStatus;
    // Переменные для хранения времени в миллисекундах
    ULONGLONG dwOldCheckPoint;
    ULONGLONG dwStartTickCount;

    // Вычисляемое время ожидания
    DWORD dwWaitTime;
    // Число необходимых байт
    DWORD dwBytesNeeded;

    // Дескриптор SCM
    SC_HANDLE schSCManager;

    // Дескриптор службы
    SC_HANDLE schService;

    // Получение дескриптора SCM
    schSCManager = OpenSCManager(
        NULL,                   // Имя компьютера
        NULL,                   // Название базы данных
        SC_MANAGER_ALL_ACCESS); // Полный доступ

    if (NULL == schSCManager)
    {
        logger << (LogMsg() << "OpenSCManager вернул NULL (" << GetLastError() << ")");
        return;
    }

    // Получение дескриптора службы
    schService = OpenService(
        schSCManager,        // Дескриптор SCM
        SVCNAME,             // Имя службы
        SERVICE_ALL_ACCESS); // Полный доступ

    if (schService == NULL)
    {
        logger << (LogMsg() << "OpenService вернул NULL (" << GetLastError() << ")");

        // Закрытие дескриптора SCM
        CloseServiceHandle(schSCManager);
        return;
    }

    // Получение статуса службы
    if (!QueryServiceStatusEx(
            schService,                     // Дескриптор службы
            SC_STATUS_PROCESS_INFO,         // Уровень требуемой информации
            (LPBYTE)&ssStatus,              // Адрес структуры
            sizeof(SERVICE_STATUS_PROCESS), // Размер структуры
            &dwBytesNeeded))                // Необходимый размер, если буфер слишком мал
    {
        logger << (LogMsg() << "QueryServiceStatusEx вернул NULL (" << GetLastError() << ")");

        // Закрытие дескрипторов
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return;
    }

    // Проверяем, запущена ли служба уже. Если она запущена, то нет смысла её запускать ещё раз.
    if (ssStatus.dwCurrentState != SERVICE_STOPPED && ssStatus.dwCurrentState != SERVICE_STOP_PENDING)
    {
        logger << "Нельзя запустить службу, поскольку она уже запущена";

        // Закрытие дескрипторов
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return;
    }

    // Сохраняем количество тиков в начальную точку
    dwStartTickCount = GetTickCount64();

    // Старое значение контрольной точки (ориентируемся на предыдущий запуск службы)
    dwOldCheckPoint = ssStatus.dwCheckPoint;

    // Дожидаемся остановки службы прежде чем запустить её
    while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
    {
        // Не ждём дольше, чем указано в подсказке "Ждать" (dwWaitHint). Оптимальный интервал составляет
        // одну десятую от указанного в подсказке, но не менее 1 секунды и не более 10 секунд
        dwWaitTime = ssStatus.dwWaitHint / 10;

        if (dwWaitTime < 1000)
        {
            dwWaitTime = 1000;
        }
        else if (dwWaitTime > 10000)
        {
            dwWaitTime = 10000;
        }

        Sleep(dwWaitTime);

        // Проверяем статус до тех пор, пока служба больше не перестанет находится в режиме ожидания
        if (!QueryServiceStatusEx(
                schService,
                SC_STATUS_PROCESS_INFO,
                (LPBYTE)&ssStatus,
                sizeof(SERVICE_STATUS_PROCESS),
                &dwBytesNeeded))
        {
            logger << (LogMsg() << "QueryServiceStatusEx вернул NULL (" << GetLastError() << ")");

            // Закрытие дескрипторов
            CloseServiceHandle(schService);
            CloseServiceHandle(schSCManager);
            return;
        }

        if (ssStatus.dwCheckPoint > dwOldCheckPoint)
        {
            // Продолжаем ждать и проверять
            dwStartTickCount = GetTickCount64();
            dwOldCheckPoint = ssStatus.dwCheckPoint;
        }
        else
        {
            if ((GetTickCount64() - dwStartTickCount) > (ULONGLONG)ssStatus.dwWaitHint)
            {
                logger << "Таймаут ожидания остановки службы";

                CloseServiceHandle(schService);
                CloseServiceHandle(schSCManager);
                return;
            }
        }
    }

    // Отправляем запрос на запуск службы
    if (!StartService(
            schService, // Дескриптор службы
            0,          // Число аргументов
            NULL))      // Отсутствуют аргументы
    {
        logger << (LogMsg() << "StartService вернул NULL (" << GetLastError() << ")");

        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return;
    }
    else
    {
        logger << "Служба в процессе запуска ...";
    }

    // Получаем статус службы
    if (!QueryServiceStatusEx(
            schService,
            SC_STATUS_PROCESS_INFO,
            (LPBYTE)&ssStatus,
            sizeof(SERVICE_STATUS_PROCESS),
            &dwBytesNeeded))
    {
        logger << (LogMsg() << "QueryServiceStatusEx вернул (" << GetLastError() << ")");

        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return;
    }

    // Сохраняем количество тиков в начальную точку
    dwStartTickCount = GetTickCount64();
    dwOldCheckPoint = ssStatus.dwCheckPoint;

    while (ssStatus.dwCurrentState == SERVICE_START_PENDING)
    {
        // Не ждём дольше, чем указано в подсказке "Ждать" (dwWaitHint). Оптимальный интервал составляет
        // одну десятую от указанного в подсказке, но не менее 1 секунды и не более 10 секунд
        dwWaitTime = ssStatus.dwWaitHint / 10;

        if (dwWaitTime < 1000)
        {
            dwWaitTime = 1000;
        }
        else if (dwWaitTime > 10000)
        {
            dwWaitTime = 10000;
        }

        Sleep(dwWaitTime);

        // Дополнительная проверка статуса
        if (!QueryServiceStatusEx(
                schService,
                SC_STATUS_PROCESS_INFO,
                (LPBYTE)&ssStatus,
                sizeof(SERVICE_STATUS_PROCESS),
                &dwBytesNeeded))
        {
            logger << (LogMsg() << "QueryServiceStatusEx вернул NULL (" << GetLastError() << ")");
            break;
        }

        if (ssStatus.dwCheckPoint > dwOldCheckPoint)
        {
            // Продолжаем проверку
            dwStartTickCount = GetTickCount64();
            dwOldCheckPoint = ssStatus.dwCheckPoint;
        }
        else
        {
            if ((GetTickCount64() - dwStartTickCount) > (ULONGLONG)ssStatus.dwWaitHint)
            {
                // Не было достигнуто никакого прогресса (подсказка "Подождать" стала не актуальной)
                break;
            }
        }
    }

    // Определяем запущена ли служба
    if (ssStatus.dwCurrentState == SERVICE_RUNNING)
    {
        logger << "Служба успешно запущена";
    }
    else
    {
        logger << "Служба не запущена";
        logger << (LogMsg() << "Current state: " << ssStatus.dwCurrentState);
        logger << (LogMsg() << "Exit Code: " << ssStatus.dwWin32ExitCode);
        logger << (LogMsg() << "Check Point: " << ssStatus.dwCheckPoint);
        logger << (LogMsg() << "Wait Hint: " << ssStatus.dwWaitHint);
    }

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
}

/* Остановка службы */
VOID __stdcall DoStopSvc()
{
    // Статус процесса службы
    SERVICE_STATUS_PROCESS ssp;
    ULONGLONG dwStartTime = GetTickCount64();
    DWORD dwBytesNeeded;
    ULONGLONG dwTimeout = 30000; 
    DWORD dwWaitTime;

    // Дескрипторы SCM и службы
    SC_HANDLE schSCManager;
    SC_HANDLE schService;

    // Получение дескриптора SCM
    schSCManager = OpenSCManager(
        NULL,                   
        NULL,                   
        SC_MANAGER_ALL_ACCESS);

    if (NULL == schSCManager)
    {
        logger << (LogMsg() << "OpenSCManager вернул NULL (" << GetLastError() << ")");
        return;
    }

    // Получение дескриптора службы
    schService = OpenService(
        schSCManager, 
        SVCNAME,      
        SERVICE_STOP |
            SERVICE_QUERY_STATUS |
            SERVICE_ENUMERATE_DEPENDENTS);

    if (schService == NULL)
    {
        logger << (LogMsg() << "OpenService вернул NULL (" << GetLastError() << ")");
        CloseServiceHandle(schSCManager);
        return;
    }

    // Убеждаемся, что служба ещё не остановлена
    if (!QueryServiceStatusEx(
            schService,
            SC_STATUS_PROCESS_INFO,
            (LPBYTE)&ssp,
            sizeof(SERVICE_STATUS_PROCESS),
            &dwBytesNeeded))
    {
        logger << (LogMsg() << "QueryServiceStatusEx вернул NULL (" << GetLastError() << ")");

        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return;
    }

    // Проверяем текущий статус службы (остановлена она или нет)
    if (ssp.dwCurrentState == SERVICE_STOPPED)
    {
        logger << "Служба уже остановлена";

        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);

        return;
    }

    // Ожидаем остановки службы, если она находится в состоянии остановки
    while (ssp.dwCurrentState == SERVICE_STOP_PENDING)
    {
        logger << "Служба останавливается ...";

        dwWaitTime = ssp.dwWaitHint / 10;
        if (dwWaitTime < 1000)
        {
            dwWaitTime = 1000;
        }
        else if (dwWaitTime > 10000)
        {
            dwWaitTime = 10000;
        }

        Sleep(dwWaitTime);

        // Получение текущего статуса службы
        if (!QueryServiceStatusEx(
                schService,
                SC_STATUS_PROCESS_INFO,
                (LPBYTE)&ssp,
                sizeof(SERVICE_STATUS_PROCESS),
                &dwBytesNeeded))
        {
            logger << (LogMsg() << "QueryServiceStatusEx вернул NULL (" << GetLastError() << ")");

            CloseServiceHandle(schService);
            CloseServiceHandle(schSCManager);
            return;
        }

        if (ssp.dwCurrentState == SERVICE_STOPPED)
        {
            logger << "Служба успешно остановлена";

            CloseServiceHandle(schService);
            CloseServiceHandle(schSCManager);
            return;
        }

        if ((GetTickCount64() - dwStartTime) > dwTimeout)
        {
            logger << "Служба останавливалась слишком долго";

            CloseServiceHandle(schService);
            CloseServiceHandle(schSCManager);
            return;
        }
    }

    // Останавливаем все зависимые службы
    StopDependentServices(schSCManager, schService);

    // Отправляем текущей службе запрос на остановку
    if (!ControlService(
            schService,
            SERVICE_CONTROL_STOP,
            (LPSERVICE_STATUS)&ssp))
    {
        logger << (LogMsg() << "ControlService вернул NULL (" << GetLastError() << ")");

        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return;
    }

    // Ожидание завершения службы
    while (ssp.dwCurrentState != SERVICE_STOPPED)
    {
        Sleep(ssp.dwWaitHint);

        // Получение статуса службы
        if (!QueryServiceStatusEx(
                schService,
                SC_STATUS_PROCESS_INFO,
                (LPBYTE)&ssp,
                sizeof(SERVICE_STATUS_PROCESS),
                &dwBytesNeeded))
        {
            logger << (LogMsg() << "QueryServiceStatusEx вернул NULL (" << GetLastError() << ")");

            CloseServiceHandle(schService);
            CloseServiceHandle(schSCManager);
            return;
        }

        if (ssp.dwCurrentState == SERVICE_STOPPED) {
            break;
        }

        if ((GetTickCount64() - dwStartTime) > dwTimeout)
        {
            logger << "Служба слишком долго завершалась";

            CloseServiceHandle(schService);
            CloseServiceHandle(schSCManager);
            return;
        }
    }

    logger << "Служба успешно остановлена";

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
}

/* Остановка всех зависимых служб */
BOOL __stdcall StopDependentServices(SC_HANDLE &schSCManager, SC_HANDLE &schService)
{
    // Характеризует какую зависимую службу мы сейчас обрабатываем
    DWORD i;
    // Число необходимых байт
    DWORD dwBytesNeeded;
    // Счётчик
    DWORD dwCount;

    // Массив структур ENUM_SERVICE_STATUS, содержащих имя службы в базе данных SCM и прочие сведения
    LPENUM_SERVICE_STATUS lpDependencies = NULL;
    ENUM_SERVICE_STATUS ess;

    // Дескриптор зависимой службы
    SC_HANDLE hDepService;

    // Состояние процесса службы
    SERVICE_STATUS_PROCESS ssp;

    ULONGLONG dwStartTime = GetTickCount64();

    // 30 секундный таймаут
    ULONGLONG dwTimeout = 30000;

    // Получаем список зависимых служб от переданной службы (только активных)
    // (передаём буфер нулевой длины, чтобы получить требуемый размер буфера, куда мы будем добавлять эти службы)
    if (EnumDependentServices(schService, SERVICE_ACTIVE,
                              lpDependencies, 0, &dwBytesNeeded, &dwCount))
    {
        // Зависимых служб нет, или есть, но они не активны на данный момент
        return TRUE;
    }
    else
    {
        if (GetLastError() != ERROR_MORE_DATA)
        {
            logger << "Неизвестная ошибка";
            return FALSE; // Unexpected error
        }

        // Выделяем отдельный буфер для зависимых служб с помощью функции HeapAlloc
        lpDependencies = (LPENUM_SERVICE_STATUS)HeapAlloc(
            GetProcessHeap(), HEAP_ZERO_MEMORY, dwBytesNeeded);

        if (!lpDependencies)
        {
            // Если буфер не выделился, то ничего не делаем
            return FALSE;
        }

        // Получаем список зависимых служб (в dwCount записывается их количество)
        if (!EnumDependentServices(schService, SERVICE_ACTIVE,
                                   lpDependencies, dwBytesNeeded, &dwBytesNeeded,
                                   &dwCount))
        {
            // Нет перечисляемых зависимых служб
            return FALSE;
        }

        // Обходим зависимые службы
        for (i = 0; i < dwCount; i++)
        {
            // Выбираем отдельную зависимую службу
            ess = *(lpDependencies + i);

            // Получаем дескриптор зависимой службы
            // с правами только для остановки и получения её статуса
            hDepService = OpenService(schSCManager,
                                      ess.lpServiceName,
                                      SERVICE_STOP | SERVICE_QUERY_STATUS);

            if (!hDepService)
            {
                // Дескриптор зависимой службы не получен

                logger << "!hDepService";
                return FALSE;
            }

            logger << "Отправка служебного кода для остановки службы ...";

            // Отправка кода SERVICE_CONTROL_STOP
            if (!ControlService(hDepService,
                                SERVICE_CONTROL_STOP,
                                (LPSERVICE_STATUS)&ssp))
            {
                // Освобождаем дескриптор зависимой службы
                CloseServiceHandle(hDepService);
                return FALSE;
            }

            logger << "Ожидаем остановку зависимой службы ...";

            // Цикл ожидания остановки службы
            while (ssp.dwCurrentState != SERVICE_STOPPED)
            {
                // Ожидаем столько времени, скольку указано в dwWaitHint зависимой службы
                Sleep(ssp.dwWaitHint);

                // Получение статуса зависимой службы
                if (!QueryServiceStatusEx(
                        hDepService,
                        SC_STATUS_PROCESS_INFO,
                        (LPBYTE)&ssp,
                        sizeof(SERVICE_STATUS_PROCESS),
                        &dwBytesNeeded))
                {
                    // Освобождаем дескриптор зависимой службы
                    CloseServiceHandle(hDepService);
                    return FALSE;
                }

                // Если служба уже остановлена - прекращаем ожидание
                if (ssp.dwCurrentState == SERVICE_STOPPED)
                {
                    break;
                }

                // В случае, если зависимая служба не изменила своего состояния в течении dwTimeout
                // выходим из данной функции
                if ((GetTickCount64() - dwStartTime) > dwTimeout)
                {
                    // Освобождаем дескриптор зависимой службы
                    CloseServiceHandle(hDepService);
                    return FALSE;
                }
            }

            // Освобождаем дескриптор зависимой службы
            CloseServiceHandle(hDepService);
        }

        // Освобождаем занятую память под буфер
        HeapFree(GetProcessHeap(), 0, lpDependencies);
    }

    return TRUE;
}

/* Устанавливает текущий статус обслуживания и сообщает о нем в SCM */
VOID ReportSvcStatus(
    DWORD dwCurrentState,
    DWORD dwWin32ExitCode,
    DWORD dwWaitHint)
{
    // Определение значения контрольной точки
    static DWORD dwCheckPoint = 1;

    // Установка текущего состояния службы
    gSvcStatus.dwCurrentState = dwCurrentState;
    // Установка ExitCode службы
    gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
    // Установка расчётного времени ожидания операции в миллисекундах
    gSvcStatus.dwWaitHint = dwWaitHint;

    // Проверка на подготовку сервиса к запуску
    if (dwCurrentState == SERVICE_START_PENDING)
    {
        gSvcStatus.dwControlsAccepted = 0;
    }
    else
    {
        // Служба обрабатывает команду остановки
        gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    }

    // Если служба запущена или остановлена сбрасываем значение контрольной точки
    if ((dwCurrentState == SERVICE_RUNNING) ||
        (dwCurrentState == SERVICE_STOPPED))
    {
        gSvcStatus.dwCheckPoint = 0;
    }
    else
    {
        // Иначе добавляем значение контрольной точки
        gSvcStatus.dwCheckPoint = dwCheckPoint++;
    }

    // Отправка текущего статуса службы в SCM
    SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}

/* Вызывается SCM всякий раз, когда в службу отправляется управляющий код с помощью функции ControlService */
VOID WINAPI SvcCtrlHandler(DWORD dwCtrl)
{
    logger << (LogMsg() << "(SvcCtrlHandler) Thread ID: " << std::this_thread::get_id() << " " << GetCurrentProcessId());

    // Обработка полученного управляющего кода
    switch (dwCtrl)
    {
    case SERVICE_CONTROL_STOP:
        // Сигнализируем SCM о том, что текущая служба находится на этапе подготовки к остановке
        ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

        // Сигнализируем службу об остановке (просто меняем значение дескриптора ghSvcStopEvent)
        SetEvent(ghSvcStopEvent);

        // Сигнализируем SCM о завершении работы службы с определённым состоянием
        ReportSvcStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);

        logger << "SERVICE_CONTROL_STOP";
        return;

    case SERVICE_CONTROL_INTERROGATE:
        logger << "SERVICE_CONTROL_INTERROGATE";
        break;

    default:
        break;
    }
}