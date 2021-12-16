#include "common/CoInitialize.h"

#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

WRAPPER_CoInitializeEx::WRAPPER_CoInitializeEx(HRESULT& Result)
{
	Result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
}

WRAPPER_CoInitializeEx::~WRAPPER_CoInitializeEx()
{
	CoUninitialize();
}