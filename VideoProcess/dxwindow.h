#pragma once

#include <windows.h>

class dxwindow
{
	void terminate();
	PROCESS_INFORMATION pi;
	bool fail = false;
	const long width, height;
public:
	dxwindow(const long _width, const long _height);
	~dxwindow();
	void draw(HDC dist);
	bool enable();
};