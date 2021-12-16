#pragma once
#include "Program/Parameters_Images.h"

#include "common/Windows_include.h"
#include <objidl.h>
#include <gdiplus.h>
#pragma comment (lib,"Gdiplus.lib")

struct ELLIPSE
{
	Gdiplus::Rect GdiPlusShape;
	Gdiplus::Color GdiPlusColor;

	static inline EVOLUTION_PARAMETERS* EvolutionParametersPointer = nullptr;
	static inline EVOLUTION_IMAGES* EvolutionImagesPointer = nullptr;
	RECT Shape;
	int Red;
	int Green;
	int Blue;
	COLORREF Color;

	ELLIPSE();
	void operator=(const ELLIPSE&);
	void MutateShape();
	void MutateColor();
	void CheckBoundary();

private:
	void ComputeGdiPlusShape();
	void ComputeGdiPlusColor();
};