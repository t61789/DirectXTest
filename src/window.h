#pragma once

#include <memory>
#include <windows.h>
#include <string>

#include "common/keyboard.h"
#include "game/game.h"

namespace dt
{
    class Window : public Singleton<Window>
    {
    public:
        Window(HINSTANCE hInstance, std::string title, int width, int height);
        ~Window();
        Window(const Window& other) = delete;
        Window(Window&& other) noexcept = delete;
        Window& operator=(const Window& other) = delete;
        Window& operator=(Window&& other) noexcept = delete;
        
        HWND GetHandle() const { return m_hwnd; }
        uint32_t GetWidth() const { return m_width; }
        uint32_t GetHeight() const { return m_height; }

        bool Create();
        void Destroy();
        int Run();
        
    private:
        bool InitWindow();
        LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
        
        static LRESULT CALLBACK WindowProcStatic(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        
        HWND m_hwnd;
        HINSTANCE m_hInstance;
        std::string m_title;
        std::string m_className;
        int m_width, m_height;
        std::unique_ptr<Game> m_game;
        std::unique_ptr<Keyboard> m_keyboard;
    };
}
