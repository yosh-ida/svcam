#include "dxwindow.h"
#include <fstream>

static HWND handle = NULL;

BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
	PROCESS_INFORMATION* pi = (PROCESS_INFORMATION*)lParam;
	DWORD lpdwProcessId = 0;
	GetWindowThreadProcessId(hWnd, &lpdwProcessId);

	if (pi->dwProcessId == lpdwProcessId)
	{
		handle = hWnd;
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
	//fail = !CreateProcess(L"notepad", { NULL }, NULL, NULL, FALSE,
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
	if (handle == NULL)
		return;
	HDC dc = GetDC(handle);
	if (dc == NULL)
		return;
	BitBlt(dist, 0, 0, width, height, dc, 0, 0, SRCCOPY);
	ReleaseDC(handle, dc);
}