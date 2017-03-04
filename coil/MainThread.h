#pragma once

#include "Route.h"

#define THREAD_COUNT 8
#define FULL_STATISTICS1

#define URL "http://www.hacker.org/coil/index.php"
//password here is incorrect
#define LOGIN_FIELDS "name=deniss.kocergins&password=00000000"


void initMainThread();
HANDLE startMainThread();
DWORD WINAPI main_thread(LPVOID lpParam);
