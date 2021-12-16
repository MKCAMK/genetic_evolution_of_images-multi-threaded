#include "Evolution/Individual.h"
#include "common/RNG_Engine.h"

extern RNG_ENGINE g_RNG_Engine;

INDIVIDUAL::INDIVIDUAL(): Rating(0), Genotype(new ELLIPSE[EvolutionParametersPointer->NumberOfEllipses]) {}

INDIVIDUAL::~INDIVIDUAL()
{
	delete[] Genotype;
}

unsigned long long INDIVIDUAL::PartialRate(int CurrentOffset, const int FirstPixel, const int LastPixel) const
{
	unsigned long long PartialRating = 0;

	const int l_ComparisonPrecision = EvolutionParametersPointer->ComparisonPrecision;
	constexpr int BytesPerPixel = CONSTANT_x_BYTES_PER_PIXEL;
	const int ComparisonPrecision_InBytes = l_ComparisonPrecision * BytesPerPixel;

	{
		int Difference = FirstPixel - CurrentOffset;
		int Times = Difference / l_ComparisonPrecision;
		CurrentOffset -= (l_ComparisonPrecision * Times);

		while (CurrentOffset < FirstPixel)
		{
			CurrentOffset += l_ComparisonPrecision;
		}
	}

	const int CurrentOffset_InBytes = CurrentOffset * BytesPerPixel;

	const BYTE* OriginalPixel_BasePointer = EvolutionImagesPointer->OriginalImage_ComparisonBuffer_DeviceIndependentBitmapPointer + CurrentOffset_InBytes;
	const BYTE* CurrentPixel_BasePointer = EvolutionImagesPointer->EvolvedImage_ComparisonBuffer_DeviceIndependentBitmapPointer + CurrentOffset_InBytes;

	for (int i = CurrentOffset; i <= LastPixel; i += l_ComparisonPrecision)
	{
		const int OriginalBlue = OriginalPixel_BasePointer[0];
		const int OriginalGreen = OriginalPixel_BasePointer[1];
		const int OriginalRed = OriginalPixel_BasePointer[2];

		int CurrentBlue = CurrentPixel_BasePointer[0];
		int CurrentGreen = CurrentPixel_BasePointer[1];
		int CurrentRed = CurrentPixel_BasePointer[2];

		CurrentRed -= OriginalRed;
		CurrentGreen -= OriginalGreen;
		CurrentBlue -= OriginalBlue;

		if (CurrentRed < 0)
			CurrentRed = -CurrentRed;
		if (CurrentGreen < 0)
			CurrentGreen = -CurrentGreen;
		if (CurrentBlue < 0)
			CurrentBlue = -CurrentBlue;

		PartialRating += static_cast<unsigned long long>(CurrentRed) + static_cast<unsigned long long>(CurrentGreen) + static_cast<unsigned long long>(CurrentBlue);

		OriginalPixel_BasePointer += ComparisonPrecision_InBytes;
		CurrentPixel_BasePointer += ComparisonPrecision_InBytes;
	}

	return PartialRating;
}

void INDIVIDUAL::MutateOrder(int index)
{
	int SwitchPartner = g_RNG_Engine.RandomFromOne(EvolutionParametersPointer->NumberOfEllipses);
	--SwitchPartner;

	ELLIPSE copy = Genotype[index];
	Genotype[index] = Genotype[SwitchPartner];
	Genotype[SwitchPartner] = copy;
}

void INDIVIDUAL::Inherit(INDIVIDUAL* Parent)
{
	for (int i = 0; i < EvolutionParametersPointer->NumberOfEllipses; ++i)
		Genotype[i] = Parent->Genotype[i];
		
	for (int i = 0; i < EvolutionParametersPointer->NumberOfEllipses; ++i)
	{
		if (g_RNG_Engine.RandomFromOne(EvolutionParametersPointer->EllipseMutationChance) == 1)
		{
			switch (g_RNG_Engine.RandomFromZero(2))
			{
				case 0:
					Genotype[i].MutateShape();
					break;
				case 1:
					Genotype[i].MutateColor();
					break;
				case 2:
					MutateOrder(i);
					break;
			}
		}
	}
}

void INDIVIDUAL::PartialDrawToComparisonBuffer(const HDC l_hdc, const int FirstEllipse, const int LastEllipse) const
{
	for (int i = FirstEllipse; i <= LastEllipse; ++i)
	{
		const COLORREF l_Color = Genotype[i].Color;
		DeleteObject(SelectObject(l_hdc, CreateSolidBrush(l_Color)));
		Ellipse(l_hdc, Genotype[i].Shape.left, Genotype[i].Shape.top, Genotype[i].Shape.right, Genotype[i].Shape.bottom);
	}
}

void INDIVIDUAL::PartialDrawToComparisonBuffer(Gdiplus::Graphics& Graphics, const int FirstEllipse, const int LastEllipse) const
{
	for (int i = FirstEllipse; i <= LastEllipse; ++i)
	{
		Gdiplus::SolidBrush Brush(Genotype[i].GdiPlusColor);
		Graphics.FillEllipse(&Brush, Genotype[i].GdiPlusShape);
	}
}