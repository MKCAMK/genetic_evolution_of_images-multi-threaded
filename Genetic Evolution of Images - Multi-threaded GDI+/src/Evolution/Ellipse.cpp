#include "Evolution/Ellipse.h"
#include "common/RNG_Engine.h"

extern RNG_ENGINE g_RNG_Engine;

ELLIPSE::ELLIPSE()
{
	Shape.left = g_RNG_Engine.RandomFromZero(EvolutionImagesPointer->OriginalImage_Info.bmWidth);
	Shape.top = g_RNG_Engine.RandomFromZero(EvolutionImagesPointer->OriginalImage_Info.bmHeight);
	if (g_RNG_Engine.RandomFromZero(1))
		Shape.right = Shape.left - g_RNG_Engine.RandomFromOne(EvolutionParametersPointer->EllipseSizeAtCreation);
	else
		Shape.right = Shape.left + g_RNG_Engine.RandomFromOne(EvolutionParametersPointer->EllipseSizeAtCreation);
	if (g_RNG_Engine.RandomFromZero(1))
		Shape.bottom = Shape.top - g_RNG_Engine.RandomFromOne(EvolutionParametersPointer->EllipseSizeAtCreation);
	else
		Shape.bottom = Shape.top + g_RNG_Engine.RandomFromOne(EvolutionParametersPointer->EllipseSizeAtCreation);
	CheckBoundary();
	Red = g_RNG_Engine.RandomFromZero(255);
	Green = g_RNG_Engine.RandomFromZero(255);
	Blue = g_RNG_Engine.RandomFromZero(255);
	Color = RGB(Red, Green, Blue);

	ComputeGdiPlusShape();
	ComputeGdiPlusColor();
}

void ELLIPSE::operator=(const ELLIPSE& source)
{
	this->Shape = source.Shape;
	this->Red = source.Red;
	this->Green = source.Green;
	this->Blue = source.Blue;
	this->Color = source.Color;

	this->GdiPlusShape = source.GdiPlusShape;
	this->GdiPlusColor = source.GdiPlusColor;
}

void ELLIPSE::MutateShape()
{
	switch (g_RNG_Engine.RandomFromZero(3))
	{
		case 0:
			if (g_RNG_Engine.RandomFromZero(1))
				Shape.left -= g_RNG_Engine.RandomFromOne(EvolutionParametersPointer->EllipseShapeMutationRange);
			else
				Shape.left += g_RNG_Engine.RandomFromOne(EvolutionParametersPointer->EllipseShapeMutationRange);
			break;
		case 1:
			if (g_RNG_Engine.RandomFromZero(1))
				Shape.top -= g_RNG_Engine.RandomFromOne(EvolutionParametersPointer->EllipseShapeMutationRange);
			else
				Shape.top += g_RNG_Engine.RandomFromOne(EvolutionParametersPointer->EllipseShapeMutationRange);
			break;
		case 2:
			if (g_RNG_Engine.RandomFromZero(1))
				Shape.right -= g_RNG_Engine.RandomFromOne(EvolutionParametersPointer->EllipseShapeMutationRange);
			else
				Shape.right += g_RNG_Engine.RandomFromOne(EvolutionParametersPointer->EllipseShapeMutationRange);
			break;
		case 3:
			if (g_RNG_Engine.RandomFromZero(1))
				Shape.bottom -= g_RNG_Engine.RandomFromOne(EvolutionParametersPointer->EllipseShapeMutationRange);
			else
				Shape.bottom += g_RNG_Engine.RandomFromOne(EvolutionParametersPointer->EllipseShapeMutationRange);
			break;
	}
	CheckBoundary();

	ComputeGdiPlusShape();
}

void ELLIPSE::MutateColor()
{
	int Value = g_RNG_Engine.RandomFromOne(EvolutionParametersPointer->EllipseColorMutationRange);
	if (g_RNG_Engine.RandomFromZero(1))
		Value = -Value;

	switch (g_RNG_Engine.RandomFromZero(2))
	{
		case 0:
		{
			Red += Value;
			if (Red > 255)
				Red = 255;
			else if (Red < 0)
				Red = 0;

			Color &= 0xFFFFFF00;
			Color |= Red;
			break;
		}
		case 1:
		{
			Green += Value;
			if (Green > 255)
				Green = 255;
			else if (Green < 0)
				Green = 0;

			Color &= 0xFFFF00FF;
			Color |= (Green << 8);
			break;
		}
		case 2:
		{
			Blue += Value;
			if (Blue > 255)
				Blue = 255;
			else if (Blue < 0)
				Blue = 0;

			Color &= 0xFF00FFFF;
			Color |= (Blue << 16);
			break;
		}
	}

	ComputeGdiPlusColor();
}

void ELLIPSE::CheckBoundary()
{
	const int l_BoundingBoxMargin = EvolutionParametersPointer->BoundingBoxMargin;
	{
		int l_Width = EvolutionImagesPointer->OriginalImage_Info.bmWidth;
		if (Shape.left < -(l_BoundingBoxMargin))
			Shape.left = -(l_BoundingBoxMargin);
		else if (Shape.left > l_Width + l_BoundingBoxMargin)
			Shape.left = l_Width + l_BoundingBoxMargin;
		if (Shape.right < -(l_BoundingBoxMargin))
			Shape.right = -(l_BoundingBoxMargin);
		else if (Shape.right > l_Width + l_BoundingBoxMargin)
			Shape.right = l_Width + l_BoundingBoxMargin;
	}
	{
		const int l_Height = EvolutionImagesPointer->OriginalImage_Info.bmHeight;
		if (Shape.top < -(l_BoundingBoxMargin))
			Shape.top = -(l_BoundingBoxMargin);
		else if (Shape.top > l_Height + l_BoundingBoxMargin)
			Shape.top = l_Height + l_BoundingBoxMargin;
		if (Shape.bottom < -(l_BoundingBoxMargin))
			Shape.bottom = -(l_BoundingBoxMargin);
		else if (Shape.bottom > l_Height + l_BoundingBoxMargin)
			Shape.bottom = l_Height + l_BoundingBoxMargin;
	}
}

inline void ELLIPSE::ComputeGdiPlusColor()
{
	GdiPlusColor = Gdiplus::Color(255, Red, Green, Blue);
}

inline void ELLIPSE::ComputeGdiPlusShape()
{
	const INT upper_left_x = (Shape.left <= Shape.right) ? Shape.left : Shape.right;
	const INT uppper_left_y = (Shape.top <= Shape.bottom) ? Shape.top : Shape.bottom;
	const INT width = abs(Shape.right - Shape.left);
	const INT height = abs(Shape.bottom - Shape.top);
	GdiPlusShape = Gdiplus::Rect(upper_left_x, uppper_left_y, width, height);
}