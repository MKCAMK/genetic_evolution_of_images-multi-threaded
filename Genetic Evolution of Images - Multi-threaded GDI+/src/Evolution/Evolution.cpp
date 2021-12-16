#include "Program/Program.h"
#include "Evolution/Individual.h"
#include "common/RNG_Engine.h"

#include <memory>

RNG_ENGINE g_RNG_Engine;

DWORD WINAPI MAIN_PROGRAM_CLASS::StaticThreadStart(LPVOID That)
{
	((MAIN_PROGRAM_CLASS*)That)->MainEvolutionThread();
	return 0;
}

inline void ShuffleIndividuals(INDIVIDUAL** const& Individuals, const int& NumberOfIndividuals)
{
	for (int i = NumberOfIndividuals - 1; i > 0; --i)
	{
		int j = g_RNG_Engine.RandomFromOne(NumberOfIndividuals);
		--j;
		INDIVIDUAL* copy = Individuals[i];
		Individuals[i] = Individuals[j];
		Individuals[j] = copy;
	}
}

inline void SortIndividuals(INDIVIDUAL** const& Individuals, const int& NumberOfIndividuals)
{
	//insertion sort algorithm
	for (int i = 1; i < NumberOfIndividuals; ++i)
	{
		INDIVIDUAL* key_pointer = Individuals[i];
		unsigned long long key = Individuals[i]->Rating;

		int j = i - 1;
		while (j >= 0 && Individuals[j]->Rating > key)
		{
			Individuals[j + 1] = Individuals[j];
			--j;
		}
		Individuals[j + 1] = key_pointer;
	}
}

void MAIN_PROGRAM_CLASS::MainEvolutionThread()
{
#if SETTING_x_DEBUG_VERSION == true
	g_RNG_Engine.Seed(0);
#else
	{
		LARGE_INTEGER PartOfSeed{ 0 };
		QueryPerformanceCounter(&PartOfSeed);
		g_RNG_Engine.Seed(static_cast<unsigned int>(std::random_device{}() + PartOfSeed.QuadPart));
	}
#endif

	INDIVIDUAL::EvolutionParametersPointer = &EvolutionParameters;
	INDIVIDUAL::EvolutionImagesPointer = &EvolutionImages;

	ELLIPSE::EvolutionParametersPointer = &EvolutionParameters;
	ELLIPSE::EvolutionImagesPointer = &EvolutionImages;

	PrepareSubthreads();

	const std::unique_ptr<INDIVIDUAL[]> guard_Population = std::make_unique<INDIVIDUAL[]>(EvolutionParameters.NumberOfImages);
	INDIVIDUAL* const Population = guard_Population.get();
	const std::unique_ptr<INDIVIDUAL* []> guard_Individuals = std::make_unique<INDIVIDUAL* []>(EvolutionParameters.NumberOfImages);
	INDIVIDUAL** const Individuals = guard_Individuals.get();
	for (int i = 0; i < EvolutionParameters.NumberOfImages; ++i)
	{
		Individuals[i] = &Population[i];
		RateIndividual(Individuals[i]);
	}

	SortIndividuals(Individuals, EvolutionParameters.NumberOfImages);

	DrawIndividual(Individuals[0]);
	BitBlt(EvolutionImages.EvolvedImage_PresentationBuffer_DeviceContextHandle, 0, 0, EvolutionImages.OriginalImage_Info.bmWidth, EvolutionImages.OriginalImage_Info.bmHeight, EvolutionImages.EvolvedImage_ComparisonBuffer_DeviceContextHandle, 0, 0, SRCCOPY);
	CurrentGeneration = 1;
	CurrentRating = Individuals[0]->Rating;

	ReportedGeneration.store(CurrentGeneration, std::memory_order_relaxed);
	ReportedRating = CurrentRating;
	ReportedPrecision = EvolutionParameters.ComparisonPrecision;

	CurrentStatus.store(PROGRAM_STATUS::WORKING, std::memory_order_relaxed);
	EnableWindow(WindowControls.Pause_Resume_Button, true);

	bool WasPausedLastLoop = false;
	const int HalfOfIndividuals = EvolutionParameters.NumberOfImages / 2;
	for (;;)
	{
		switch (CurrentStatus.load(std::memory_order_relaxed))
		{
			case PROGRAM_STATUS::WORKING:
			{
				if (WasPausedLastLoop == true)
				{
					ThreadsControl.ResumeThreads();
					WasPausedLastLoop = false;
				}

				if (CurrentRating == 0)
				{
					if (EvolutionParameters.ComparisonPrecision > 1)
					{
						EvolutionParameters.ComparisonPrecision /= 2;

						for (int i = 0; i < EvolutionParameters.NumberOfImages; ++i)
							RateIndividual(Individuals[i]);
					}
					else
					{
						Pause_Resume_ButtonOperation(true);
						continue;
					}
				}

				for (int i = 0; i < HalfOfIndividuals; ++i)
				{
					INDIVIDUAL* const EvolvingIndividual = Individuals[(EvolutionParameters.NumberOfImages - 1) - i];
					EvolvingIndividual->Inherit(Individuals[i]);
					RateIndividual(EvolvingIndividual);
				}

				ShuffleIndividuals(Individuals, EvolutionParameters.NumberOfImages);
				SortIndividuals(Individuals, EvolutionParameters.NumberOfImages);

				DrawIndividual(Individuals[0]);

				++CurrentGeneration;
				CurrentRating = Individuals[0]->Rating;

				EnterCriticalSection(&CriticalSection_EvolvedImage_PresentationBuffer);

				BitBlt(EvolutionImages.EvolvedImage_PresentationBuffer_DeviceContextHandle, 0, 0, EvolutionImages.OriginalImage_Info.bmWidth, EvolutionImages.OriginalImage_Info.bmHeight, EvolutionImages.EvolvedImage_ComparisonBuffer_DeviceContextHandle, 0, 0, SRCCOPY);

				ReportedGeneration.store(CurrentGeneration, std::memory_order_relaxed);
				ReportedRating = CurrentRating;
				ReportedPrecision = EvolutionParameters.ComparisonPrecision;

				LeaveCriticalSection(&CriticalSection_EvolvedImage_PresentationBuffer);

				continue;
			}
			case PROGRAM_STATUS::PAUSED:
			{
				if (WasPausedLastLoop == false)
				{
					ThreadsControl.SuspendThreads();
					SaveImage_ButtonEnable();
					WasPausedLastLoop = true;
				}

				Sleep(200);

				continue;
			}
			case PROGRAM_STATUS::SHUTDOWN:
			default:
				ThreadsControl.ShutdownThreads();
		}

		break;
	}
}

void MAIN_PROGRAM_CLASS::RateIndividual(INDIVIDUAL* const IndividualToRate)
{
	DrawIndividual(IndividualToRate);

	ThreadsControl.BroadcastNextObject(IndividualToRate);
	ThreadsControl.BroadcastCurrentOffset(g_RNG_Engine.RandomFromZero(EvolutionParameters.ComparisonPrecision - 1));
	ThreadsControl.BroadcastCommand(THREAD_COMMAND::RATE);
	ThreadsControl.ThreadsData[0].PartialResult =
		IndividualToRate->
		PartialRate(ThreadsControl.ThreadsData[0].CurrentOffset, ThreadsControl.ThreadsData[0].FirstAssignedElement_Pixels, ThreadsControl.ThreadsData[0].FinalAssignedElement_Pixels);
	ThreadsControl.ThreadsData[0].Command.store(THREAD_COMMAND::IDLE, std::memory_order_relaxed);

	unsigned long long FinalRating = 0;
	for (int i = 0; i < ThreadsControl.NumberOfThreads; ++i)
	{
		THREAD_COMMAND ThreadStatus;
		do
			ThreadStatus = ThreadsControl.ThreadsData[i].Command.load(std::memory_order_relaxed);
		while (ThreadStatus != THREAD_COMMAND::IDLE);

		FinalRating += ThreadsControl.ThreadsData[i].PartialResult;
	}

	IndividualToRate->Rating = FinalRating;
}

inline void FastAlphaBlend(BYTE* Destination, BYTE* Source, const int& NumberOfPixels)
{
	for (int i = 0; i < NumberOfPixels; ++i)
	{
		if (Source[CONSTANT_x_APLHA_CHANNEL_OFFSET] != 0)
		{
			std::memcpy(Destination, Source, sizeof(BYTE) * CONSTANT_x_BYTES_PER_PIXEL);
		}

		Destination += CONSTANT_x_BYTES_PER_PIXEL;
		Source += CONSTANT_x_BYTES_PER_PIXEL;
	}
}

void MAIN_PROGRAM_CLASS::DrawIndividual(INDIVIDUAL* const IndividualToDraw)
{
	ThreadsControl.BroadcastNextObject(IndividualToDraw);
	ThreadsControl.BroadcastCommand(THREAD_COMMAND::DRAW);

	FillRect(EvolutionImages.EvolvedImage_ComparisonBuffer_DeviceContextHandle, &EvolutionImages.OriginalImage_Info_AreaRect, EvolutionImages.EvolvedImageBackgroundBrush);

	IndividualToDraw->
		PartialDrawToComparisonBuffer(EvolutionImages.EvolvedImage_ComparisonBuffer_DeviceContextHandle, ThreadsControl.ThreadsData[0].FirstAssignedElement_Ellipses, ThreadsControl.ThreadsData[0].FinalAssignedElement_Ellipses);
	ThreadsControl.ThreadsData[0].Command.store(THREAD_COMMAND::IDLE, std::memory_order_relaxed);

	for (int i = 1; i < ThreadsControl.NumberOfThreads; ++i)// Skips the first thread (itself)
	{
		THREAD_COMMAND ThreadStatus;
		do
			ThreadStatus = ThreadsControl.ThreadsData[i].Command.load(std::memory_order_relaxed);
		while (ThreadStatus != THREAD_COMMAND::IDLE);

		if (ThreadsControl.ThreadsData[i].FirstAssignedElement_Ellipses == -1)
			break;

		FastAlphaBlend(EvolutionImages.EvolvedImage_ComparisonBuffer_DeviceIndependentBitmapPointer, ThreadsControl.ThreadsData[i].BitmapDataPointer, EvolutionImages.OriginalImage_Info_NumberOfPixels);
	}
}

double ProcessResults(LARGE_INTEGER* const Start, LARGE_INTEGER* const Stop, LARGE_INTEGER* const Result, const int ArraySize, const double Frequency)
{
	double FinalAvarage = 0;
	for (int i = 1; i < ArraySize; ++i)
	{
		Result[i].QuadPart = Stop[i].QuadPart - Start[i].QuadPart;
		FinalAvarage += static_cast<double>(Result[i].QuadPart) / Frequency;
	}
	FinalAvarage /= ArraySize - 1;

	return FinalAvarage;
}

bool MAIN_PROGRAM_CLASS::DrawingSpeedTest(double& DrawingTime, double& AlphaBlendingTime)
{
	const int l_NumberOfPixels = EvolutionImages.OriginalImage_Info_NumberOfPixels;
	const int NumberOfBytes = l_NumberOfPixels * CONSTANT_x_BYTES_PER_PIXEL;

	INDIVIDUAL TestSpecimen;

	LARGE_INTEGER Frequency;
	if (QueryPerformanceFrequency(&Frequency) == false)
		return false;

	const double f_Frequency = static_cast<double>(Frequency.QuadPart);

	constexpr int ARRAY_SIZE = 4;
	LARGE_INTEGER Start[ARRAY_SIZE], Stop[ARRAY_SIZE], Result[ARRAY_SIZE];

	std::vector<BYTE> Destination, Source;
	Destination.resize(NumberOfBytes);
	Source.resize(NumberOfBytes);
	for (int i = 0; i < NumberOfBytes; ++i)
		Source[i] = 0xff;

	DrawingTime = 0;
	AlphaBlendingTime = 0;

	BOOL Success_Start, Success_Stop;

	for (int i = 0; i < ARRAY_SIZE; ++i)
	{
		Success_Start = QueryPerformanceCounter(&Start[i]);
		TestSpecimen.PartialDrawToComparisonBuffer(EvolutionImages.EvolvedImage_ComparisonBuffer_DeviceContextHandle, 0, EvolutionParameters.NumberOfEllipses - 1);
		Success_Stop = QueryPerformanceCounter(&Stop[i]);

		if (Success_Start == false || Success_Stop == false)
			return false;
	}

	DrawingTime = ProcessResults(Start, Stop, Result, ARRAY_SIZE, f_Frequency);

	for (int i = 0; i < ARRAY_SIZE; ++i)
	{
		Success_Start = QueryPerformanceCounter(&Start[i]);
		FastAlphaBlend(Destination.data(), Source.data(), l_NumberOfPixels);
		Success_Stop = QueryPerformanceCounter(&Stop[i]);

		if (Success_Start == false || Success_Stop == false)
			return false;
	}

	AlphaBlendingTime = ProcessResults(Start, Stop, Result, ARRAY_SIZE, f_Frequency);

	return true;
}