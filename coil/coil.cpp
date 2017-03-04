// coil.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "curl/curl.h"
#include "Route.h"
#include "MainThread.h"



int _tmain(int argc, _TCHAR* argv[])
{
	initMainThread();
	main_thread(0);
}
