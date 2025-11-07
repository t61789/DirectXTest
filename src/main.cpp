#include <windows.h>
#include <string>

#include "window.h"

// 使用示例
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    dt::Window window(hInstance, "现代Windows窗口", 800, 600);
    
    if (!window.Create())
    {
        return -1;
    }
    
    return window.Run();
}