#include "dxwindow.h"
#include <fstream>

HWND handle = NULL;
HDC dc = NULL;

BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
	//	ewindow‚ª‚ ‚éê‡‚ÍƒXƒ‹[
	if (GetWindow(hWnd, GW_OWNER) != NULL)
		return TRUE;

	PROCESS_INFORMATION* pi = (PROCESS_INFORMATION*)lParam;
	DWORD lpdwProcessId = 0;
	GetWindowThreadProcessId(hWnd, &lpdwProcessId);

	if (pi->dwProcessId == lpdwProcessId)
	{
		handle = hWnd;
		dc = GetDC(handle);
		return FALSE;
	}
	return TRUE;
}

dxwindow::dxwindow(const long _width, const long _height)
	: width(_width), height(_height)
{
	STARTUPINFO stinfo = { 0 };
	stinfo.dwXSize = width;
	stinfo.dwYSize = height;
	stinfo.dwX = 0;
	stinfo.dwY = 0;
	fail = !CreateProcess(L"C:\\Users\\Haru\\Documents\\svcam\\Release\\DXWindow.exe", { NULL }, NULL, NULL, FALSE,
		0, NULL, NULL, &stinfo, &pi);
	if (fail)
	{
	/*	std::ofstream ofs("C:\\Users\\Haru\\Documents\\svcam\\Release\\log.log");
		if (!ofs.fail())
		{
			ofs << GetLastError();
			ofs.close();
		}
	*/
		return;
	}
	Sleep(1000);
	EnumWindows(EnumWindowsProc, (LPARAM)&pi);
}

dxwindow::~dxwindow()
{
	terminate();
}

void dxwindow::terminate()
{
	if (fail) return;

	CloseHandle(pi.hThread);
	if (handle != NULL)
	{
		ReleaseDC(handle, dc);
		PostMessage(handle, WM_CLOSE, 0, 0);
		WaitForSingleObject(handle, 5);
		handle = NULL;		
	}

	TerminateProcess(pi.hProcess, 0);
	fail = true;
}

bool dxwindow::enable()
{
	return !fail;
}

void dxwindow::draw(HDC dist)
{
	if (dc == NULL)
		return;
	BitBlt(dist, 0, 0, width, height, dc, 0, 0, SRCCOPY);
}