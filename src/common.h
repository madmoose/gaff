#ifndef COMMON_H
#define COMMON_H

#define WINVER 0x0501
#include <windows.h>

#define PSAPI_VERSION 1
#include "psapi.h"

#include <cassert>
#include <cstdint>

class debugger_t;

typedef void(*db_callback_t)(debugger_t *, HANDLE hProcess, HANDLE hThread, DWORD addr, const void *);

#endif
