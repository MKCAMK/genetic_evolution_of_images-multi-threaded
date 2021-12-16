#pragma once
#include "common/Windows_include.h"

class GDI_DEVICE_CONTEXT_WRAPPER
{
public:
	HDC DeviceContext;
private:
	HPEN m_OriginalPen;
	HBRUSH m_OriginalBrush;
	HBITMAP m_OriginalBitmap;

public:
	explicit GDI_DEVICE_CONTEXT_WRAPPER(HDC = NULL);
	~GDI_DEVICE_CONTEXT_WRAPPER();

	HGDIOBJ LoadObject(HGDIOBJ);
	HGDIOBJ SwapObject(HGDIOBJ);
	HGDIOBJ UnloadObject(DWORD);
};