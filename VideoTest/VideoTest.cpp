#include <iostream>
#include "VideoProc.h"

int main()
{
	VideoInit(640, 360);
	for (int i = 0 ; i < 100; i++)
		VideoProc(NULL);
	VideoTerminate();
    std::cout << "Hello World!\n";
	return 0;
}