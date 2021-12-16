#pragma once
#include "common/Windows_include.h"

struct WRAPPER_CoInitializeEx
{
	explicit WRAPPER_CoInitializeEx(HRESULT&);
	~WRAPPER_CoInitializeEx();
};