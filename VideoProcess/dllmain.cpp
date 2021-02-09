// dllmain.cpp : DLL アプリケーションのエントリ ポイントを定義します。
#define WIN32_LEAN_AND_MEAN
#include "VideoProc.h"
#include "dxwindow.h"
#include <stdlib.h>

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

dxwindow *dw;

EXTERN_C int _stdcall VideoInit(const long width, const long height)
{
	dw = new dxwindow(width, height);
	return dw->enable();
}

EXTERN_C void _stdcall VideoTerminate()
{
	delete dw;
}

EXTERN_C void _stdcall VideoProc(const HDC dist)
{
	dw->draw(dist);
	//for (long i = 0; i < size; ++i)
	//	pData[i] = rand();
}