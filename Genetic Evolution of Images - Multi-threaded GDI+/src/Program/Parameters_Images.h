#pragma once
#include "common/Windows_include.h"

struct EVOLUTION_PARAMETERS
{
	int NumberOfImages;
	int NumberOfEllipses;
	int EllipseMutationChance;
	int EllipseSizeAtCreation;
	int EllipseShapeMutationRange;
	int EllipseColorMutationRange;
	int BoundingBoxMargin;
	int ComparisonPrecision;
};

struct EVOLUTION_IMAGES
{
	HDC ClientBuffer_DeviceContextHandle				{ NULL };
	RECT ClientBuffer_Rect								{};
	HBITMAP ClientBuffer_DeviceDependentBitmapHandle	{ NULL };

	BITMAP OriginalImage_Info							{};
	int OriginalImage_Info_NumberOfPixels				{ -1 };
	RECT OriginalImage_Info_AreaRect					{};
	BITMAPINFO DeviceIndependentBitmap_Format			{};

	HDC OriginalImage_PresentationBuffer_DeviceContextHandle				{ NULL };
	HBITMAP OriginalImage_PresentationBuffer_DeviceDependentBitmapHandle	{ NULL };

	HBITMAP OriginalImage_ComparisonBuffer_DeviceIndependentBitmapHandle	{ NULL };
	BYTE* OriginalImage_ComparisonBuffer_DeviceIndependentBitmapPointer		{ nullptr };

	HDC EvolvedImage_PresentationBuffer_DeviceContextHandle					{ NULL };
	HBITMAP EvolvedImage_PresentationBuffer_DeviceDependentBitmapHandle		{ NULL };

	HDC EvolvedImage_ComparisonBuffer_DeviceContextHandle					{ NULL };
	HBITMAP EvolvedImage_ComparisonBuffer_DeviceIndependentBitmapHandle		{ NULL };
	BYTE* EvolvedImage_ComparisonBuffer_DeviceIndependentBitmapPointer		{ nullptr };

	HBRUSH WindowBackgroundBrush			{ NULL };
	HBRUSH EvolvedImageBackgroundBrush		{ NULL };

	ULONG_PTR GdiPlusToken					{ NULL };
};