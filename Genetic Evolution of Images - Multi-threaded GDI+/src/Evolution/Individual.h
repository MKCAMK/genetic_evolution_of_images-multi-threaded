#pragma once
#include "Evolution/Ellipse.h"
#include "Program/Parameters_Images.h"
#include "common/Settings.h"

#include "common/Windows_include.h"
#include <objidl.h>
#include <gdiplus.h>
#pragma comment (lib,"Gdiplus.lib")

class INDIVIDUAL
{
public:
	static inline EVOLUTION_PARAMETERS* EvolutionParametersPointer = nullptr;
	static inline EVOLUTION_IMAGES* EvolutionImagesPointer = nullptr;

	static inline const Gdiplus::Color TransparentBackground = Gdiplus::Color(0, 0, 0, 0);

private:
	void MutateOrder(int);

public:
	unsigned long long Rating;
	ELLIPSE* const Genotype;

	INDIVIDUAL();
	~INDIVIDUAL();
	unsigned long long PartialRate(int, const int, const int) const;
	void Inherit(INDIVIDUAL*);
	void PartialDrawToComparisonBuffer(const HDC, const int, const int) const;
	void PartialDrawToComparisonBuffer(Gdiplus::Graphics&, const int, const int) const;
};