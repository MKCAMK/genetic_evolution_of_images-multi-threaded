#pragma once
#include "Evolution/Individual.h"
#include "common/Settings.h"

#include "common/Windows_include.h"
#include <atomic>

enum struct THREAD_COMMAND { SHUTDOWN, INITIALIZATION, IDLE, DRAW, RATE };

struct THREAD_DATA
{
	HANDLE ThreadHandle					{ NULL };

	INDIVIDUAL* ObjectBeingWorkedOn		{ nullptr };
	int CurrentOffset					{ -1 };

	BYTE* BitmapDataPointer				{ nullptr };

	int FirstAssignedElement_Ellipses	{ -1 };
	int FinalAssignedElement_Ellipses	{ -1 };
	int FirstAssignedElement_Pixels		{ -1 };
	int FinalAssignedElement_Pixels		{ -1 };
	unsigned long long PartialResult	{ 0 };

	std::atomic<THREAD_COMMAND> Command	{ THREAD_COMMAND::SHUTDOWN };
};

class THREADS_CONTROL
{
public:
	int NumberOfThreads			{ -1 };
	THREAD_DATA* ThreadsData	{ nullptr };
	
	~THREADS_CONTROL();
	void SetUpThreadsData(int, int, int, int, int);
	void BroadcastCommand(THREAD_COMMAND);
	void BroadcastNextObject(INDIVIDUAL*);
	void BroadcastCurrentOffset(int);
	void ResumeThreads();
	void SuspendThreads();
	void ShutdownThreads();
};