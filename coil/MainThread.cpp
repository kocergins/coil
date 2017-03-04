#include "StdAfx.h"
#include "MainThread.h"
#include "curl/curl.h"
#include "Route.h"
#include <time.h>

#define MAX_POST_SIZE 20480
#define MAX_INPUT_SIZE 204800

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream);
static DWORD WINAPI solver_func( LPVOID lpParam );
void handle_error(uint status);
uint sendSolutionAndGetTask();
void startSolvers();
void waitSolution();
bool storeSolution();
void initRequestWithNoSolution();
void cleanMemory();
void sendStatistics();
DWORD WINAPI main_thread( LPVOID lpParam );

Route* solution;
HWND m_main_window_handle;

uint labirint_size_x, labirint_size_y;
HANDLE thread_handle[THREAD_COUNT]; //solver thread related
bool thread_exit; //solver thread related
CRITICAL_SECTION solution_found;
static size_t data_writen;

char* post_fields;
char* input_data;

Route* route;

void initMainThread()
{
	post_fields = new char[MAX_POST_SIZE];
	input_data = new char[MAX_INPUT_SIZE];
	InitializeCriticalSection(&solution_found);
	initRequestWithNoSolution();
	route = NULL;
	solution = NULL;
	data_writen = 0;
}


size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	if (0 == memcpy_s((char*)stream + data_writen, MAX_INPUT_SIZE - data_writen, ptr, size * nmemb))
	{
		data_writen += size * nmemb;
		return size * nmemb;
	}
	else
		return 0;
}

DWORD WINAPI solver_func( LPVOID lpParam ) 
{
	int param = *(int*)lpParam;

	//TODO: use stack, not heap
	FailCondition*** fail_condition = new FailCondition**[labirint_size_y+2];
	for(uint i = 0; i < labirint_size_y+2; i++)
	{
		fail_condition[i] = new FailCondition*[labirint_size_x+2];
		for(uint j = 0; j < labirint_size_x+2; j++)
		{
			fail_condition[i][j] = new FailCondition[4];
		}
	}

	for (uint z = 0; z < 4; z++)
	{
		for (uint i = labirint_size_y*param/THREAD_COUNT; i < labirint_size_y*(param+1)/THREAD_COUNT; i++)
		{
			for (uint j = 0; j < labirint_size_x; j++)
			{
				if (thread_exit)
					return 0;
				Route* thread_route = new Route(i+1, j+1, route, fail_condition);
				
				if (thread_route->start_solve(z+1) && thread_route->solve())
				{
					if (thread_route->done && !solution && TryEnterCriticalSection(&solution_found))
					{
						thread_exit = true;
						solution = thread_route;
						LeaveCriticalSection(&solution_found);
						return 0;
					}
					else
						return 0;
				}
				else
				{

					delete thread_route;
				}
			}
		}
	}
	printf(".");
	return 0;
}

void handle_error(uint status)
{
	printf_s("Error - %d", status);
	Sleep(5000);
}

uint sendSolutionAndGetTask()
{
	//printf_s("posting request: %s\n", post_fields);

	CURL *curl = curl_easy_init();
	CURLcode status = CURLE_OK;
	if(!curl)
	{
		return 1;
	}

	if (status == CURLE_OK)
		status = curl_easy_setopt(curl, CURLOPT_URL, URL);
	if (status == CURLE_OK)
		status = curl_easy_setopt(curl, CURLOPT_POST, 1);
	if (status == CURLE_OK)
		status = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields);
	if (status == CURLE_OK)
		status = curl_easy_setopt(curl, CURLOPT_WRITEDATA, input_data);
	if (status == CURLE_OK)
		status = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_data);
	
	data_writen = 0;
	
	if (status == CURLE_OK)
		status = curl_easy_perform(curl);

	curl_easy_cleanup(curl);

	if (status != CURLE_OK || !data_writen)
		return 2;

	input_data[data_writen] = NULL;


	char *start;
	char *end;
	start = strstr(input_data, "FlashVars");
	if (!start)
		return 3;
	end = strstr(start, "board=");
	if (!end)
		return 4;
	*end = 0;
	
	labirint_size_x = 0;
	labirint_size_y = 0;
	sscanf_s(start, "FlashVars\" value=\"x=%d&y=%d&", &labirint_size_x, &labirint_size_y);

	if (!labirint_size_x || !labirint_size_y)
		return 5;

	start = end + 6; //start will be just after "board="
	end = strstr(start, ">");
	if (!end)
		return 6;

	*end = 0;

	//printf("size y=%d, size x=%d, values=%s\n", labirint_size_y, labirint_size_x, start);

	route = new Route(labirint_size_y+2, labirint_size_x+2, start);
	//route->print_matrix();

	return 0;
}

void startSolvers()
{
	thread_exit = false;

	for(int i = 0; i < THREAD_COUNT; i++)
	{
		thread_handle[i] = 0;
		int* param = new int;
		*param = i;
		
		thread_handle[i] = CreateThread( NULL, 0, &solver_func, param, 0, NULL);  
	}
}

void waitSolution()
{
	WaitForMultipleObjects(THREAD_COUNT, thread_handle, TRUE, INFINITE);
}

bool storeSolution()
{
	if (!solution)
		return false;

	//solution->simulation_mode = true;

	char* result = solution->get_result(MAX_POST_SIZE - 200);
	uint start_y, start_x;
	solution->get_start_coordinates(start_y, start_x);

	/*CellStatistics stats(labirint_size_y+2, labirint_size_x+2);
	Route tmp(start_y + 1, start_x + 1, route, &stats);
	tmp.simulation_mode = true;
	tmp.start_solve(*result);
	tmp.correct_path = result+1;
	tmp.solve();*/

	sprintf_s(post_fields, MAX_POST_SIZE, "%s&qpath=%s&y=%d&x=%d", LOGIN_FIELDS, result, start_y, start_x);
	return true;
}

void initRequestWithNoSolution()
{
	sprintf_s(post_fields, MAX_POST_SIZE, LOGIN_FIELDS);
}

void cleanMemory()
{
	delete route;
	route = NULL;

	for(int i=0; i<THREAD_COUNT; i++)
	{
		CloseHandle(thread_handle[i]);
	}

	delete solution;
	solution = NULL;
}

HANDLE startMainThread()
{
	return CreateThread( NULL, 0, &main_thread, 0, 0, NULL);  
}

void showStatistics()
{
	
}

DWORD WINAPI main_thread( LPVOID lpParam )
{
	while (1)
	{
		struct tm *current;
		time_t now;
		
		time(&now);
		current = localtime(&now);

		printf("start time is %i:%i:%i\n", current->tm_hour, current->tm_min, current->tm_sec);

		uint error = sendSolutionAndGetTask(); //no solution in first call, just get task
		if (error)
		{
			handle_error(error);
			continue;
		}

		startSolvers();
		waitSolution();
		if (storeSolution())
			showStatistics(); //statistics should be deleted by receiver
		else
		{
			initRequestWithNoSolution();
			printf("fail, press enter key");
			//getchar();
		}
		cleanMemory();
//getchar();
	}
	return 0;
}
