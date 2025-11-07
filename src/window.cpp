#include "window.h"

namespace dt
{
    Window::Window(
        const HINSTANCE hInstance,
        std::string title,
        const int width,
        const int height):
        m_hwnd(nullptr),
        m_hInstance(hInstance),
        m_title(std::move(title)),
        m_width(width),
        m_height(height)
    {
        m_className = "ModernWindowClass_" + std::to_string(GetTickCount());
    }
    
    Window::~Window()
    {
        if (m_hwnd)
        {
            DestroyWindow(m_hwnd);
        }
    }

    bool Window::Create()
    {
        WNDCLASS wc = {};
        wc.lpfnWndProc = WindowProcStatic;
        wc.hInstance = m_hInstance;
        wc.lpszClassName = m_className.c_str();
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>((COLOR_WINDOW + 1));
        
        if (!RegisterClass(&wc))
            return false;
        
        m_hwnd = CreateWindowEx(
            0,
            m_className.c_str(),
            m_title.c_str(),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            m_width, m_height,
            nullptr, nullptr, m_hInstance, this
        );
        
        return m_hwnd != nullptr;
    }

    int Window::Run()
    {
        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);
        
        MSG msg = {};
        while (GetMessage(&msg, nullptr, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        return static_cast<int>(msg.wParam);
    }

    LRESULT CALLBACK Window::WindowProcStatic(const HWND hwnd, const UINT uMsg, const WPARAM wParam, const LPARAM lParam)
    {
        Window* pWindow = nullptr;
        
        if (uMsg == WM_NCCREATE)
        {
            auto pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            pWindow = static_cast<Window*>(pCreate->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));
            
            if (pWindow) {
                pWindow->m_hwnd = hwnd;
            }
        }
        else
        {
            pWindow = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }
        
        if (pWindow)
        {
            return pWindow->HandleMessage(uMsg, wParam, lParam);
        }
        
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    LRESULT Window::HandleMessage(const UINT uMsg, const WPARAM wParam, const LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(m_hwnd, &ps);
                
                RECT rect;
                GetClientRect(m_hwnd, &rect);
                
                DrawText(hdc, "现代C++ Windows窗口", -1, &rect, 
                        DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                
                EndPaint(m_hwnd, &ps);
            }
            return 0;
            
        case WM_SIZE:
            // 处理窗口大小改变
            InvalidateRect(m_hwnd, nullptr, TRUE);
            return 0;
        default:
            return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
        }
    }
}
