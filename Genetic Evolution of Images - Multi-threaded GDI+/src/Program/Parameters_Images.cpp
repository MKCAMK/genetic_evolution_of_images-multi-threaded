#include "Program/Program.h"
#include "common/Settings.h"

int SetStretchBltMode_Helper(HDC DeviceContext, int Mode, int NewOriginX = 0, int NewOriginY = 0, LPPOINT ret_PreviousOrigin = NULL)
{
	int PreviousMode = SetStretchBltMode(DeviceContext, Mode);

	if (Mode == HALFTONE)//Need to be called after setting the strech mode to HALFTONE, to avoid misalignment - according to Microsoft.
		SetBrushOrgEx(DeviceContext, NewOriginX, NewOriginY, ret_PreviousOrigin);

	return PreviousMode;
}

bool MAIN_PROGRAM_CLASS::BitmapPreparation()
{
	//The original file is in the ComparisonBuffer.

	/*
	CreateDIBitmap function uses the supplied DC's bitmap to decide the bit depth for the bitmap it produces.
	When a new DC is created, it is provided with a 1×1 pixel monochrome bitmap. Using such a DC as the argument for CreateDIBitmap will result in a black and white bitmap.
	Therefore, it is necessary to temporarily select a different bitmap, which has full color, into the DC to receive a color bitmap.
	*/

	if (NULL == (EvolutionImages.OriginalImage_PresentationBuffer_DeviceContextHandle = CreateCompatibleDC(NULL)))
		return false;
	SaveDC(EvolutionImages.OriginalImage_PresentationBuffer_DeviceContextHandle);
	if (NULL == SelectObject(EvolutionImages.OriginalImage_PresentationBuffer_DeviceContextHandle, EvolutionImages.OriginalImage_ComparisonBuffer_DeviceIndependentBitmapHandle))
		return false;
	EvolutionImages.OriginalImage_PresentationBuffer_DeviceDependentBitmapHandle = CreateDIBitmap(EvolutionImages.OriginalImage_PresentationBuffer_DeviceContextHandle, &EvolutionImages.DeviceIndependentBitmap_Format.bmiHeader, CBM_INIT, (void*)EvolutionImages.OriginalImage_ComparisonBuffer_DeviceIndependentBitmapPointer, &EvolutionImages.DeviceIndependentBitmap_Format, DIB_RGB_COLORS);
	if (NULL == SelectObject(EvolutionImages.OriginalImage_PresentationBuffer_DeviceContextHandle, EvolutionImages.OriginalImage_PresentationBuffer_DeviceDependentBitmapHandle))
		return false;
	//The original file is now in the ComparisonBuffer and PresentationBuffer.

	if (NULL == (EvolutionImages.EvolvedImage_PresentationBuffer_DeviceContextHandle = CreateCompatibleDC(NULL)))
		return false;
	SaveDC(EvolutionImages.EvolvedImage_PresentationBuffer_DeviceContextHandle);
	EvolutionImages.EvolvedImage_PresentationBuffer_DeviceDependentBitmapHandle = CreateBitmapIndirect(&EvolutionImages.OriginalImage_Info);
	if (NULL == SelectObject(EvolutionImages.EvolvedImage_PresentationBuffer_DeviceContextHandle, EvolutionImages.EvolvedImage_PresentationBuffer_DeviceDependentBitmapHandle))
		return false;
	//The original file is now in the ComparisonBuffer and PresentationBuffer. The evolved image's PresentationBuffer has now been set.

	if (NULL == (EvolutionImages.EvolvedImage_ComparisonBuffer_DeviceContextHandle = CreateCompatibleDC(NULL)))
		return false;
	SaveDC(EvolutionImages.EvolvedImage_ComparisonBuffer_DeviceContextHandle);
	EvolutionImages.EvolvedImage_ComparisonBuffer_DeviceIndependentBitmapHandle = CreateDIBSection(NULL, &EvolutionImages.DeviceIndependentBitmap_Format, DIB_RGB_COLORS, (void**)&EvolutionImages.EvolvedImage_ComparisonBuffer_DeviceIndependentBitmapPointer, NULL, NULL);
	if (NULL == SelectObject(EvolutionImages.EvolvedImage_ComparisonBuffer_DeviceContextHandle, EvolutionImages.EvolvedImage_ComparisonBuffer_DeviceIndependentBitmapHandle))
		return false;
	if (NULL == SelectObject(EvolutionImages.EvolvedImage_ComparisonBuffer_DeviceContextHandle, CreateSolidBrush(RGB(0, 0, 0))))//This stopgap object is needed because during drawing, when a newly created object are loaded, the old one is being deleted. Thus the default object would end up being deleted. 
		return false;
	if (NULL == SelectObject(EvolutionImages.EvolvedImage_ComparisonBuffer_DeviceContextHandle, GetStockObject(NULL_PEN)))
		return false;
	//The original file is now in the ComparisonBuffer and PresentationBuffer. The evolved image's PresentationBuffer and ComparisonBuffer has now been set.

	if (NULL == (EvolutionImages.WindowBackgroundBrush = CreateSolidBrush(SETTING_x_EVOLUTION_WINDOW_BACKGROUND_COLOR)))
		return false;

	if (NULL == (EvolutionImages.EvolvedImageBackgroundBrush = CreateSolidBrush(SETTING_x_IMAGE_EVOLUTION_BACKGROUND_COLOR)))
		return false;

	if (NULL == (EvolutionImages.ClientBuffer_DeviceContextHandle = CreateCompatibleDC(NULL)))
		return false;
	SaveDC(EvolutionImages.ClientBuffer_DeviceContextHandle);
	EvolutionImages.ClientBuffer_Rect = { 0, 0, 1, 1 };
	EvolutionImages.ClientBuffer_DeviceDependentBitmapHandle = CreateCompatibleBitmap(EvolutionImages.EvolvedImage_ComparisonBuffer_DeviceContextHandle, EvolutionImages.ClientBuffer_Rect.right, EvolutionImages.ClientBuffer_Rect.bottom);
	if (NULL == SelectObject(EvolutionImages.ClientBuffer_DeviceContextHandle, EvolutionImages.ClientBuffer_DeviceDependentBitmapHandle))
		return false;

	SetStretchBltMode_Helper(EvolutionImages.ClientBuffer_DeviceContextHandle, HALFTONE);//COLORONCOLOR gives results closer to the bitmaps' internal representation. However, HALFTONE give better looking images. For presentation's sake HALFTONE is being used.

	Gdiplus::GdiplusStartupInput Input;
	if (Gdiplus::GdiplusStartup(&EvolutionImages.GdiPlusToken, &Input, NULL) != Gdiplus::Status::Ok)
		return false;

	return true;
}

void MAIN_PROGRAM_CLASS::BitmapCleanup()
{
	HGDIOBJ CurrentBrush = GetCurrentObject(EvolutionImages.EvolvedImage_ComparisonBuffer_DeviceContextHandle, OBJ_BRUSH);

	if (NULL != EvolutionImages.ClientBuffer_DeviceContextHandle)
	{
		RestoreDC(EvolutionImages.ClientBuffer_DeviceContextHandle, -1);
		DeleteDC(EvolutionImages.ClientBuffer_DeviceContextHandle);
		EvolutionImages.ClientBuffer_DeviceContextHandle = NULL;
	}
	if (NULL != EvolutionImages.OriginalImage_PresentationBuffer_DeviceContextHandle)
	{
		RestoreDC(EvolutionImages.OriginalImage_PresentationBuffer_DeviceContextHandle, -1);
		DeleteDC(EvolutionImages.OriginalImage_PresentationBuffer_DeviceContextHandle);
		EvolutionImages.OriginalImage_PresentationBuffer_DeviceContextHandle = NULL;
	}
	if (NULL != EvolutionImages.EvolvedImage_PresentationBuffer_DeviceContextHandle)
	{
		RestoreDC(EvolutionImages.EvolvedImage_PresentationBuffer_DeviceContextHandle, -1);
		DeleteDC(EvolutionImages.EvolvedImage_PresentationBuffer_DeviceContextHandle);
		EvolutionImages.EvolvedImage_PresentationBuffer_DeviceContextHandle = NULL;
	}
	if (NULL != EvolutionImages.EvolvedImage_ComparisonBuffer_DeviceContextHandle)
	{
		RestoreDC(EvolutionImages.EvolvedImage_ComparisonBuffer_DeviceContextHandle, -1);
		DeleteDC(EvolutionImages.EvolvedImage_ComparisonBuffer_DeviceContextHandle);
		EvolutionImages.EvolvedImage_ComparisonBuffer_DeviceContextHandle = NULL;
	}

	if (NULL != EvolutionImages.ClientBuffer_DeviceDependentBitmapHandle)
	{
		DeleteObject(EvolutionImages.ClientBuffer_DeviceDependentBitmapHandle);
		EvolutionImages.ClientBuffer_DeviceDependentBitmapHandle = NULL;
	}
	if (NULL != EvolutionImages.OriginalImage_PresentationBuffer_DeviceDependentBitmapHandle)
	{
		DeleteObject(EvolutionImages.OriginalImage_PresentationBuffer_DeviceDependentBitmapHandle);
		EvolutionImages.OriginalImage_PresentationBuffer_DeviceDependentBitmapHandle = NULL;
	}
	if (NULL != EvolutionImages.OriginalImage_ComparisonBuffer_DeviceIndependentBitmapHandle)
	{
		DeleteObject(EvolutionImages.OriginalImage_ComparisonBuffer_DeviceIndependentBitmapHandle);
		EvolutionImages.OriginalImage_ComparisonBuffer_DeviceIndependentBitmapHandle = NULL;
	}
	if (NULL != EvolutionImages.EvolvedImage_PresentationBuffer_DeviceDependentBitmapHandle)
	{
		DeleteObject(EvolutionImages.EvolvedImage_PresentationBuffer_DeviceDependentBitmapHandle);
		EvolutionImages.EvolvedImage_PresentationBuffer_DeviceDependentBitmapHandle = NULL;
	}
	if (NULL != EvolutionImages.EvolvedImage_ComparisonBuffer_DeviceIndependentBitmapHandle)
	{
		DeleteObject(EvolutionImages.EvolvedImage_ComparisonBuffer_DeviceIndependentBitmapHandle);
		EvolutionImages.EvolvedImage_ComparisonBuffer_DeviceIndependentBitmapHandle = NULL;
	}
	EvolutionImages.EvolvedImage_ComparisonBuffer_DeviceIndependentBitmapPointer = nullptr;

	DeleteObject(CurrentBrush);

	if (NULL != EvolutionImages.WindowBackgroundBrush)
	{
		DeleteObject(EvolutionImages.WindowBackgroundBrush);
		EvolutionImages.WindowBackgroundBrush = NULL;
	}
	if (NULL != EvolutionImages.EvolvedImageBackgroundBrush)
	{
		DeleteObject(EvolutionImages.EvolvedImageBackgroundBrush);
		EvolutionImages.EvolvedImageBackgroundBrush = NULL;
	}

	if (EvolutionImages.GdiPlusToken != NULL)
	{
		Gdiplus::GdiplusShutdown(EvolutionImages.GdiPlusToken);
		EvolutionImages.GdiPlusToken = NULL;
	}
}