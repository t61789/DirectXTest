#include <windows.h>

#include "window.h"
#include <ShellScalingApi.h>
#pragma comment(lib, "Shcore.lib")

int WINAPI WinMain(const HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
    
    dt::Window window(hInstance, "现代Windows窗口", 1600, 900);
    
    if (!window.Create())
    {
        return -1;
    }
    
    return window.Run();
}
