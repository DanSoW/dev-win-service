cmake_minimum_required (VERSION 3.8)

# Включение горячей перезагрузки для компиляторов MSVC, если поддерживается.
if (POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("dev-win-sc")

# Установка флага DEBUG
set (DEBUG 1)

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++20 /Zc:__cplusplus")
endif()

set (CMAKE_C_STANDARD 17)
set (CMAKE_C_STANDARD_REQUIRED ON)

# Add configurable variables
set (APP_NAME "dev-win-sc")
set (APP_VERSION "debug")
set (APP_PACKAGE "win-sc")

set (FILE_NAME_LOG "dev_win_sc")

# Размер файла лога (не более 100 мб)
set (MAX_LOG_SIZE 100)

# Сепаратор пути для Windows
set (SEP_WIN "\\\\")

# Путь до лог файла
set (LOG_PATH ${CMAKE_SOURCE_DIR})

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set (DIR_SEPARATOR "'${SEP_WIN}'")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set (DIR_SEPARATOR '/')
else()
    set (DIR_SEPARATOR '/')
endif()

# Определение конфигурационного файла
configure_file (config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h @ONLY)

# Файлы проекта
add_executable (dev-win-sc "dev-win-sc.cpp" "dev-win-sc.h" "logger/logger.h" "logger/logger.cpp" "utils/utils.h" "config.h" "utils/utils.cpp" "exceptions/LogException.h" "exceptions/LogException.cpp" "logger/log-msg.h" "logger/log-msg.cpp" "service/WinService.h" "service/WinService.cpp" "net/IPAddress.h" "net/IPAddress.cpp" "utils/system-info/system-info.h" "utils/system-info/system-info.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET dev-win-sc PROPERTY CXX_STANDARD 20)
endif()
