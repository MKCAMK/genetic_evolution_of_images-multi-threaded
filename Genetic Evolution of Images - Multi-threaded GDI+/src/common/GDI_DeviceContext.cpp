#include "common/GDI_DeviceContext.h"
#include <stdexcept>

GDI_DEVICE_CONTEXT_WRAPPER::GDI_DEVICE_CONTEXT_WRAPPER(HDC DeviceContextWithWhichToBeCompatible) : DeviceContext(NULL), m_OriginalPen(NULL), m_OriginalBrush(NULL), m_OriginalBitmap(NULL)
{
	DeviceContext = CreateCompatibleDC(DeviceContextWithWhichToBeCompatible);

	if (DeviceContext == NULL)
		throw std::runtime_error("Could not create a device context. The device context passed as the argument may have been invalid.");
}

GDI_DEVICE_CONTEXT_WRAPPER::~GDI_DEVICE_CONTEXT_WRAPPER()
{
	if (m_OriginalPen != NULL)
	{
		HPEN CurrentPen = (HPEN)SelectObject(DeviceContext, m_OriginalPen);
		DeleteObject(CurrentPen);
	}

	if (m_OriginalBrush != NULL)
	{
		HBRUSH CurrentBrush = (HBRUSH)SelectObject(DeviceContext, m_OriginalBrush);
		DeleteObject(CurrentBrush);
	}

	if (m_OriginalBitmap != NULL)
	{
		HBITMAP CurrentBitmap = (HBITMAP)SelectObject(DeviceContext, m_OriginalBitmap);
		DeleteObject(CurrentBitmap);
	}

	if (DeviceContext != NULL)
	{
		DeleteDC(DeviceContext);
	}
}

inline bool SwapObjectSubroutine(const HDC& DeviceContext, HGDIOBJ& SwappedOutObject, const HGDIOBJ& ObjectToBeSwappedIn)
{
	SwappedOutObject = SelectObject(DeviceContext, ObjectToBeSwappedIn);
	if (SwappedOutObject == NULL || SwappedOutObject == HGDI_ERROR)
	{
		SwappedOutObject = ObjectToBeSwappedIn;
		return true;	// Error
	}
	return false;		// No error
}

HGDIOBJ GDI_DEVICE_CONTEXT_WRAPPER::SwapObject(HGDIOBJ ObjectToBeSwappedIn)
{
	HGDIOBJ SwappedOutObject;

	switch (GetObjectType(ObjectToBeSwappedIn))
	{
		case OBJ_PEN:
		{
			if (SwapObjectSubroutine(DeviceContext, SwappedOutObject, ObjectToBeSwappedIn))
				break;

			if (m_OriginalPen == NULL)
			{
				m_OriginalPen = (HPEN)SwappedOutObject;
				SwappedOutObject = NULL;
			}
			break;
		}
		case OBJ_BRUSH:
		{
			if (SwapObjectSubroutine(DeviceContext, SwappedOutObject, ObjectToBeSwappedIn))
				break;

			if (m_OriginalBrush == NULL)
			{
				m_OriginalBrush = (HBRUSH)SwappedOutObject;
				SwappedOutObject = NULL;
			}
			break;
		}
		case OBJ_BITMAP:
		{
			if (SwapObjectSubroutine(DeviceContext, SwappedOutObject, ObjectToBeSwappedIn))
				break;

			if (m_OriginalBitmap == NULL)
			{
				m_OriginalBitmap = (HBITMAP)SwappedOutObject;
				SwappedOutObject = NULL;
			}
			break;
		}
		default:
		{
			SwappedOutObject = ObjectToBeSwappedIn;
			break;
		}
	}

	return SwappedOutObject;
}

HGDIOBJ GDI_DEVICE_CONTEXT_WRAPPER::LoadObject(HGDIOBJ ObjectToBeLoadedIn)
{
	HGDIOBJ SwappedOutObject = SwapObject(ObjectToBeLoadedIn);

	if (SwappedOutObject == ObjectToBeLoadedIn)
		return ObjectToBeLoadedIn;

	if (SwappedOutObject != NULL)
		DeleteObject(SwappedOutObject);

	return NULL;
}

HGDIOBJ GDI_DEVICE_CONTEXT_WRAPPER::UnloadObject(DWORD ObjectType)
{
	switch (ObjectType)
	{
		case OBJ_PEN:
		{
			if (m_OriginalPen == NULL)
				break;

			HGDIOBJ ReturnedObject = SelectObject(DeviceContext, m_OriginalPen);
			if (ReturnedObject == NULL || ReturnedObject == HGDI_ERROR)
				break;

			return ReturnedObject;
		}
		case OBJ_BRUSH:
		{
			if (m_OriginalBrush == NULL)
				break;

			HGDIOBJ ReturnedObject = SelectObject(DeviceContext, m_OriginalBrush);
			if (ReturnedObject == NULL || ReturnedObject == HGDI_ERROR)
				break;

			return ReturnedObject;
		}
		case OBJ_BITMAP:
		{
			if (m_OriginalBitmap == NULL)
				break;

			HGDIOBJ ReturnedObject = SelectObject(DeviceContext, m_OriginalBitmap);
			if (ReturnedObject == NULL || ReturnedObject == HGDI_ERROR)
				break;

			return ReturnedObject;
		}
		default:
			break;
	}

	return NULL;
}