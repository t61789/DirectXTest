#pragma once

#include <windows.h>
#include <string>

namespace dt
{
    class Window
    {
    public:
        Window(HINSTANCE hInstance, std::string title, int width, int height);
        ~Window();
        Window(const Window& other) = delete;
        Window(Window&& other) noexcept = delete;
        Window& operator=(const Window& other) = delete;
        Window& operator=(Window&& other) noexcept = delete;
        
        HWND GetHandle() const { return m_hwnd; }

        bool Create();
        int Run();
        
    private:
        LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
        
        static LRESULT CALLBACK WindowProcStatic(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        
        HWND m_hwnd;
        HINSTANCE m_hInstance;
        std::string m_title;
        std::string m_className;
        int m_width, m_height;
    };
}
