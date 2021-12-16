#include "Evolution/Subthread.h"
#include "Program/Program.h"
#include "common/GDI_DeviceContext.h"

#include <stdexcept>
#include <memory>
#include <vector>

THREADS_CONTROL::~THREADS_CONTROL() { delete[] ThreadsData; }// "delete[]" handles nullptr; no need to check for it

void SetUpThreadsData_Loop(const int NumberOfElements, const int NumberOfThreadsUsed, THREAD_DATA* ThreadsData, int THREAD_DATA::* FirstAssignedElement, int THREAD_DATA::* FinalAssignedElement)
{
	const int ElementsPerThread = NumberOfElements / NumberOfThreadsUsed;
	int Remainder = NumberOfElements % NumberOfThreadsUsed;
	int ElementIndex = NumberOfElements - 1;
	for (int i = NumberOfThreadsUsed - 1; i >= 0; --i)
	{
		ThreadsData[i].*FinalAssignedElement = ElementIndex;
		ElementIndex -= ElementsPerThread;
		if (Remainder > 0)
		{
			ElementIndex -= 1;
			Remainder -= 1;;
		}
		ThreadsData[i].*FirstAssignedElement = ElementIndex + 1;
	}
}

void THREADS_CONTROL::SetUpThreadsData(int arg_TotalNumberOfThreads, int arg_TotalNumberOfEllements_Ellipses, int arg_NumberOfThreadsUsed_Ellipses, int arg_TotalNumberOfEllements_Pixels, int arg_NumberOfThreadsUsed_Pixels)
{
	if (NumberOfThreads != -1)
	{
		ShutdownThreads();
		delete[]  ThreadsData;
	}

	NumberOfThreads = arg_TotalNumberOfThreads;

	ThreadsData = new THREAD_DATA[NumberOfThreads];

	for (int i = 0; i < NumberOfThreads; ++i)
	{
		ThreadsData[i].ThreadHandle = NULL;
		ThreadsData[i].Command.store(THREAD_COMMAND::SHUTDOWN, std::memory_order_relaxed);
		ThreadsData[i].ObjectBeingWorkedOn = nullptr;
		ThreadsData[i].CurrentOffset = -1;
		ThreadsData[i].BitmapDataPointer = nullptr;
		ThreadsData[i].FirstAssignedElement_Ellipses = -1;
		ThreadsData[i].FinalAssignedElement_Ellipses = -1;
		ThreadsData[i].FirstAssignedElement_Pixels = -1;
		ThreadsData[i].FinalAssignedElement_Pixels = -1;
		ThreadsData[i].PartialResult = 0;
	}

	SetUpThreadsData_Loop(arg_TotalNumberOfEllements_Ellipses, arg_NumberOfThreadsUsed_Ellipses, ThreadsData, &THREAD_DATA::FirstAssignedElement_Ellipses, &THREAD_DATA::FinalAssignedElement_Ellipses);

	SetUpThreadsData_Loop(arg_TotalNumberOfEllements_Pixels, arg_NumberOfThreadsUsed_Pixels, ThreadsData, &THREAD_DATA::FirstAssignedElement_Pixels, &THREAD_DATA::FinalAssignedElement_Pixels);

#if SETTING_x_DEBUG_VERSION == true // Check for errors in assigning work to threads.
	if (ThreadsData[0].FirstAssignedElement_Ellipses != 0)
		throw std::logic_error("Subthreads preparation error - assigning ellipses");
	if (ThreadsData[0].FirstAssignedElement_Pixels != 0)
		throw std::logic_error("Subthreads preparation error - assigning pixels");
#endif
}

void THREADS_CONTROL::BroadcastNextObject(INDIVIDUAL* arg_NextObject)
{
	for (int i = 0; i < NumberOfThreads; ++i)
		ThreadsData[i].ObjectBeingWorkedOn = arg_NextObject;
}

void THREADS_CONTROL::BroadcastCurrentOffset(int arg_CurrentOffset)
{
	for (int i = 0; i < NumberOfThreads; ++i)
		ThreadsData[i].CurrentOffset = arg_CurrentOffset;
}

void THREADS_CONTROL::BroadcastCommand(THREAD_COMMAND arg_Command)
{
	for (int i = 0; i < NumberOfThreads; ++i)
		ThreadsData[i].Command.store(arg_Command, std::memory_order_relaxed);
}

void THREADS_CONTROL::ResumeThreads()
{
	for (int i = 0; i < NumberOfThreads; ++i)
	{
		if (ThreadsData[i].ThreadHandle != NULL)
			ResumeThread(ThreadsData[i].ThreadHandle);
	}
}

void THREADS_CONTROL::SuspendThreads()
{
	for (int i = 0; i < NumberOfThreads; ++i)
	{
		if (ThreadsData[i].ThreadHandle != NULL)
			SuspendThread(ThreadsData[i].ThreadHandle);
	}
}

void THREADS_CONTROL::ShutdownThreads()
{
	BroadcastCommand(THREAD_COMMAND::SHUTDOWN);
	ResumeThreads();
	for (int i = 0; i < NumberOfThreads; ++i)
	{
		if (ThreadsData[i].ThreadHandle != NULL)
		{
			if (WaitForSingleObject(ThreadsData[i].ThreadHandle, 10000))
			{
				TerminateThread(ThreadsData[i].ThreadHandle, 0);
			}
			CloseHandle(ThreadsData[i].ThreadHandle);
			ThreadsData[i].ThreadHandle = NULL;
		}
	}
}

int AdvanceProcessorCoreInformation(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX& ProcessorCoreInformation)
{
	const int Size = ProcessorCoreInformation->Size;
	ProcessorCoreInformation = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(reinterpret_cast<unsigned char*>(ProcessorCoreInformation) + Size);
	return Size;
}

int HowManyProcessorsAreThere(bool PhysicalProcessorsOnly)
{
	int ReturnValue = 1;

	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Buffer = NULL;
	DWORD BufferLength = 0;
	BOOL Result = GetLogicalProcessorInformationEx(RelationProcessorCore, Buffer, &BufferLength);
	if (Result == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		const std::unique_ptr<BYTE[]> guard_Buffer = std::make_unique<BYTE[]>(BufferLength);
		Buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)guard_Buffer.get();
		Result = GetLogicalProcessorInformationEx(RelationProcessorCore, Buffer, &BufferLength);
		if (Result == TRUE)
		{
			int NumberOfProcessors = 0;

			PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX ProcessorCoreInformation = Buffer;
			while (BufferLength > 0)
			{
				if (PhysicalProcessorsOnly == true)
				{
					++NumberOfProcessors;
				}
				else
				{
					KAFFINITY Mask = ProcessorCoreInformation->Processor.GroupMask->Mask;
					for (int i = sizeof(KAFFINITY) * 8; i > 0; --i)
					{
						if (Mask & 0x1)
						{
							++NumberOfProcessors;
						}

						Mask >>= 1;
					}
				}

				BufferLength -= AdvanceProcessorCoreInformation(ProcessorCoreInformation);
			}
			
			if (NumberOfProcessors > 0)
				ReturnValue = NumberOfProcessors;
		}
	}

	return ReturnValue;
}

double CalculateWorkingTime(double DrawingTime, double AlphaBlendingTime, double Threads)
{
	DrawingTime /= Threads;
	return DrawingTime + (AlphaBlendingTime * --Threads);
}

int CalculateNumberOfDrawingThreads(double DrawingTime, double AlphaBlendingTime, int NumberOfEllipses, int NumberOfAvailableThreads)
{
	constexpr int MinimumSensibleWorkingSetSize_Ellipses = SETTING_x_MINIMUM_SENSIBLE_WORKING_SET_SIZE_x_ELLIPSES;

	std::vector<double> Times;
	
	Times.push_back(0);
	Times.push_back(CalculateWorkingTime(DrawingTime, AlphaBlendingTime, 1));
	
	int i = 2;
	for (; i <= NumberOfAvailableThreads; ++i)
	{
		if (NumberOfEllipses / i < MinimumSensibleWorkingSetSize_Ellipses)
			break;

		Times.push_back(CalculateWorkingTime(DrawingTime, AlphaBlendingTime, static_cast<double>(i)));

		if (Times[i] > Times[i - 1])
			break;
	}

	return --i;
}

struct DATA_PASSED_TO_THREAD
{
	MAIN_PROGRAM_CLASS* That;
	int ThreadIndex;
};

void MAIN_PROGRAM_CLASS::PrepareSubthreads()
{
	constexpr bool PhysicalProcessorsOnly = SETTING_x_CREATE_WORK_THREADS_FOR_PHYSICAL_PROCESSORS_ONLY;
	const int NumberOfProcessors = HowManyProcessorsAreThere(PhysicalProcessorsOnly);

	int NumberOfAvailableWorkThreads = NumberOfProcessors;
#if SETTING_x_KEEP_ONE_CORE_RESERVED_FOR_MAIN_WINDOW == true
	if (NumberOfProcessors > 1) // Reserve one processor for the aplication window's thread.
		NumberOfAvailableWorkThreads -= 1;
#endif

	int WorkingSetSize_Pixels = EvolutionImages.OriginalImage_Info_NumberOfPixels;
	constexpr int MinimumSensibleWorkingSetSize_Pixels = SETTING_x_MINIMUM_SENSIBLE_WORKING_SET_SIZE_x_PIXELS;
	int OptimalNumberOfWorkThreads_Pixels = WorkingSetSize_Pixels / MinimumSensibleWorkingSetSize_Pixels;
	if (OptimalNumberOfWorkThreads_Pixels > NumberOfAvailableWorkThreads)
		OptimalNumberOfWorkThreads_Pixels = NumberOfAvailableWorkThreads;
	if (OptimalNumberOfWorkThreads_Pixels == 0)
		OptimalNumberOfWorkThreads_Pixels = 1;

	int WorkingSetSize_Ellipses = EvolutionParameters.NumberOfEllipses;
	int OptimalNumberOfWorkThreads_Ellipses = 1;
	if (NumberOfAvailableWorkThreads > 1)
	{
		double DrawingTime, AlphaBlendingTime;
		if (DrawingSpeedTest(DrawingTime, AlphaBlendingTime) == true)
			OptimalNumberOfWorkThreads_Ellipses = CalculateNumberOfDrawingThreads(DrawingTime, AlphaBlendingTime, WorkingSetSize_Ellipses, NumberOfAvailableWorkThreads);
	}

	int OptimalNumberOfWorkThreads_Final = (OptimalNumberOfWorkThreads_Ellipses > OptimalNumberOfWorkThreads_Pixels) ? OptimalNumberOfWorkThreads_Ellipses : OptimalNumberOfWorkThreads_Pixels;

	ThreadsControl.SetUpThreadsData(OptimalNumberOfWorkThreads_Final, WorkingSetSize_Ellipses, OptimalNumberOfWorkThreads_Ellipses, WorkingSetSize_Pixels, OptimalNumberOfWorkThreads_Pixels);

#if  SETTING_x_DEBUG_VERSION == true
	NumberOfUsedThreads = std::to_wstring(OptimalNumberOfWorkThreads_Final);
	NumberOfUsedThreads_Drawing = std::to_wstring(OptimalNumberOfWorkThreads_Ellipses);
	NumberOfUsedThreads_Rating = std::to_wstring(OptimalNumberOfWorkThreads_Pixels);
#endif

	bool SubthreadsSuccesfullyCreated = true;
	for (int i = ThreadsControl.NumberOfThreads - 1; i > 0; --i)
	{
		DATA_PASSED_TO_THREAD* PassedData = new DATA_PASSED_TO_THREAD;
		PassedData->That = this;
		PassedData->ThreadIndex = i;
		HANDLE SubthreadHandle = NULL;
		SubthreadHandle = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)StaticSubthreadStart, (LPVOID)PassedData, CREATE_SUSPENDED, NULL);
		if (SubthreadHandle == NULL)
		{
			SubthreadsSuccesfullyCreated = false;
#if SETTING_x_DEBUG_VERSION == true
			MessageBox(NULL, L"Fallback to a single thread due to a failure to create a thread.", NULL, MB_OK);
#endif
			break;
		}
		ThreadsControl.ThreadsData[i].ThreadHandle = SubthreadHandle;
	}

	if (SubthreadsSuccesfullyCreated == true)
	{
		InitializeCriticalSection(&CriticalSection_CreateCompatibleDC);

		ThreadsControl.BroadcastCommand(THREAD_COMMAND::INITIALIZATION);
		ThreadsControl.ResumeThreads();
		ThreadsControl.ThreadsData[0].Command.store(THREAD_COMMAND::IDLE);
		for (int i = 0; i < ThreadsControl.NumberOfThreads; ++i)
		{
			for (;;)
			{
				switch (ThreadsControl.ThreadsData[i].Command.load(std::memory_order_relaxed))
				{
					case THREAD_COMMAND::INITIALIZATION:
						continue;
					case THREAD_COMMAND::IDLE:
						goto GOTO_PrepareSubthreads_SUBTHREADS_SUCCESS;
					case THREAD_COMMAND::SHUTDOWN:
					default:
					{
						SubthreadsSuccesfullyCreated = false;
#if SETTING_x_DEBUG_VERSION == true
						MessageBox(NULL, L"Fallback to a single thread due to a thread shutdown or abnormal behaviour.", NULL, MB_OK);
#endif
						goto GOTO_PrepareSubthreads_SUBTHREADS_FAILURE;
					}
				}
			}
		GOTO_PrepareSubthreads_SUBTHREADS_SUCCESS:;
		}
	GOTO_PrepareSubthreads_SUBTHREADS_FAILURE:;

		DeleteCriticalSection(&CriticalSection_CreateCompatibleDC);
	}

	if (SubthreadsSuccesfullyCreated == false)// Fallback to single thread execution
	{
		ThreadsControl.ShutdownThreads();
		constexpr int SingleThread = 1;
		ThreadsControl.SetUpThreadsData(SingleThread, WorkingSetSize_Ellipses, SingleThread, WorkingSetSize_Pixels, SingleThread);
		ThreadsControl.BroadcastCommand(THREAD_COMMAND::IDLE);

#if  SETTING_x_DEBUG_VERSION == true
		NumberOfUsedThreads = std::to_wstring(1);
		NumberOfUsedThreads_Drawing = std::to_wstring(1);
		NumberOfUsedThreads_Rating = std::to_wstring(1);
#endif
	}
}

DWORD WINAPI MAIN_PROGRAM_CLASS::StaticSubthreadStart(LPVOID PassedData)
{
	DATA_PASSED_TO_THREAD* PassedDataConvertedPointer = static_cast<DATA_PASSED_TO_THREAD*>(PassedData);

	DATA_PASSED_TO_THREAD UnpackedData = *PassedDataConvertedPointer;

	delete PassedDataConvertedPointer;

	UnpackedData.That->EvolutionSubthread(UnpackedData.ThreadIndex);

	return 0;
}

void MAIN_PROGRAM_CLASS::EvolutionSubthread(int arg_ThreadIndex)
{
	THREAD_DATA* const ThisThreadData = &ThreadsControl.ThreadsData[arg_ThreadIndex];

	std::unique_ptr<GDI_DEVICE_CONTEXT_WRAPPER> wr_DeviceContext;
	BYTE* DeviceIndependentBitmapDataPointer = nullptr;
	constexpr int BytesPerPixel = CONSTANT_x_BYTES_PER_PIXEL;
	const int NumberOfBytes = EvolutionImages.OriginalImage_Info_NumberOfPixels * BytesPerPixel;
	std::unique_ptr<Gdiplus::Bitmap> GdiPlusBitmapSharedWithGdi;
	std::unique_ptr<Gdiplus::Graphics> Graphics;

	for (;;)
	{
		THREAD_COMMAND CurrentCommand = ThisThreadData->Command.load(std::memory_order_relaxed);
		switch (CurrentCommand)
		{
			case THREAD_COMMAND::INITIALIZATION:
			{
				//Create a device context, but only if this thread participates in the drawing phase.
				bool InitializationSuccessful = true;
				try
				{
					if (ThisThreadData->FirstAssignedElement_Ellipses != -1)
					{
						EnterCriticalSection(&CriticalSection_CreateCompatibleDC);
						wr_DeviceContext.reset(new GDI_DEVICE_CONTEXT_WRAPPER(EvolutionImages.EvolvedImage_ComparisonBuffer_DeviceContextHandle));
						LeaveCriticalSection(&CriticalSection_CreateCompatibleDC);
						wr_DeviceContext->LoadObject(CreateDIBSection(NULL, &EvolutionImages.DeviceIndependentBitmap_Format, DIB_RGB_COLORS, (void**)&DeviceIndependentBitmapDataPointer, NULL, NULL));

						ThisThreadData->BitmapDataPointer = DeviceIndependentBitmapDataPointer;

						GdiPlusBitmapSharedWithGdi.reset(new Gdiplus::Bitmap(&EvolutionImages.DeviceIndependentBitmap_Format, (void*)DeviceIndependentBitmapDataPointer));
						Graphics.reset(new Gdiplus::Graphics(GdiPlusBitmapSharedWithGdi.get()));
					}
				}
				catch (const std::runtime_error&)
				{
					InitializationSuccessful = false;
#if SETTING_x_DEBUG_VERSION == true
					MessageBox(NULL, L"Fallback to a single thread due to an exception.", NULL, MB_OK);
#endif
				}

				if (InitializationSuccessful)
					ThisThreadData->Command.store(THREAD_COMMAND::IDLE, std::memory_order_relaxed);
				else
					ThisThreadData->Command.store(THREAD_COMMAND::SHUTDOWN, std::memory_order_relaxed);
				continue;
			}
			case THREAD_COMMAND::IDLE:
				continue;
			case THREAD_COMMAND::DRAW:
			{
				if (ThisThreadData->FirstAssignedElement_Ellipses != -1)
				{
					memset(DeviceIndependentBitmapDataPointer, 0x00, NumberOfBytes);
					ThisThreadData->ObjectBeingWorkedOn->PartialDrawToComparisonBuffer(*Graphics, ThisThreadData->FirstAssignedElement_Ellipses, ThisThreadData->FinalAssignedElement_Ellipses);
				}

				ThisThreadData->Command.store(THREAD_COMMAND::IDLE, std::memory_order_relaxed);
				continue;
			}
			case THREAD_COMMAND::RATE:
			{
				if (ThisThreadData->FirstAssignedElement_Pixels == -1)
				{
					ThisThreadData->PartialResult = 0;
				}
				else
				{
					ThisThreadData->PartialResult = ThisThreadData->ObjectBeingWorkedOn->PartialRate(ThisThreadData->CurrentOffset, ThisThreadData->FirstAssignedElement_Pixels, 
						ThisThreadData->FinalAssignedElement_Pixels);
				}
				ThisThreadData->Command.store(THREAD_COMMAND::IDLE, std::memory_order_relaxed);
				continue;
			}
			case THREAD_COMMAND::SHUTDOWN:
			default:
				goto GOTO_EvolutionSubthread_SHUTDOWN;
		}
	}
GOTO_EvolutionSubthread_SHUTDOWN:;
}