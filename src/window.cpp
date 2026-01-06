#include "window.h"

#include <iostream>
#include <imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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
        m_keyboard = std::make_unique<Keyboard>();
    }
    
    Window::~Window()
    {
        m_keyboard.reset();
        
        if (m_hwnd)
        {
            DestroyWindow(m_hwnd);
        }
    }

    bool Window::Create()
    {
        if (!InitWindow())
        {
            return false;
        }
        
        m_game = mup<Game>(m_width, m_height);

        return true;
    }

    void Window::Destroy()
    {
        m_game.reset();
    }

    int Window::Run()
    {
        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);
        
        MSG msg = {};
        while (msg.message != WM_QUIT)
        {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
            {
                m_game->Update();
                m_game->Render();
            }
        }
        
        return static_cast<int>(msg.wParam);
    }

    bool Window::InitWindow()
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

    LRESULT CALLBACK Window::WindowProcStatic(const HWND hwnd, const UINT uMsg, const WPARAM wParam, const LPARAM lParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
        {
            return true;
        }
        
        Window* pWindow = nullptr;
        
        if (uMsg == WM_NCCREATE)
        {
            auto pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            pWindow = static_cast<Window*>(pCreate->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));
            
            if (pWindow)
            {
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
            Destroy();
            PostQuitMessage(0);
            return 0;
            
        case WM_PAINT:
            ValidateRect(m_hwnd, nullptr);
            return 0;
            
        case WM_SIZE:
            InvalidateRect(m_hwnd, nullptr, TRUE);
            return 0;

        case WM_KEYDOWN:
            m_keyboard->SetKeyDown(wParam);
            return 0;

        case WM_KEYUP:
            m_keyboard->SetKeyUp(wParam);
            return 0;

        case WM_KILLFOCUS:
            m_keyboard->ResetAllKey();
            return 0;
            
        default:
            return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
        }
    }
}
