#pragma once
#include <windows.h>
__control_entrypoint(DllExport)
EXTERN_C void _stdcall VideoProc(const HDC dist);
__control_entrypoint(DllExport)
EXTERN_C int _stdcall VideoInit(const long width, const long height);
__control_entrypoint(DllExport)
EXTERN_C void _stdcall VideoTerminate();