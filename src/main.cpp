#include <windows.h>

#include "window.h"

int WINAPI WinMain(const HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    dt::Window window(hInstance, "现代Windows窗口", 1600, 900);
    
    if (!window.Create())
    {
        return -1;
    }
    
    return window.Run();
}
