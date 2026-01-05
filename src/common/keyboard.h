#pragma once
#include <bitset>

#include "utils.h"

namespace dt
{
    enum class KeyCode : uint8_t
    {
        // 字母键
        A,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,
        
        // 数字键
        Num0,
        Num1,
        Num2,
        Num3,
        Num4,
        Num5,
        Num6,
        Num7,
        Num8,
        Num9,
        
        // 功能键
        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,
        F13,
        F14,
        F15,
        F16,
        F17,
        F18,
        F19,
        F20,
        F21,
        F22,
        F23,
        F24,
        
        // 方向键
        Left,
        Right,
        Up,
        Down,
        
        // 特殊键
        Space,
        Enter,
        Escape,
        Backspace,
        Tab,
        CapsLock,
        
        // 修饰键
        Shift,
        LeftShift,
        RightShift,
        Control,
        LeftControl,
        RightControl,
        Alt,
        LeftAlt,
        RightAlt,
        
        // 系统键
        LeftWin,
        RightWin,
        Menu,
        
        // 编辑键
        Insert,
        Delete,
        Home,
        End,
        PageUp,
        PageDown,
        
        // 数字小键盘
        NumPad0,
        NumPad1,
        NumPad2,
        NumPad3,
        NumPad4,
        NumPad5,
        NumPad6,
        NumPad7,
        NumPad8,
        NumPad9,
        Multiply,
        Add,
        Subtract,
        Decimal,
        Divide,
        NumLock,
        Separator,
        
        // 符号键
        Comma,
        Period,
        Semicolon,
        Quote,
        LeftBracket,
        RightBracket,
        Backslash,
        Minus,
        Plus,
        Grave,
        Slash,
        
        // 其他
        PrintScreen,
        ScrollLock,
        Pause,
        Apps,  // 应用程序键
        Sleep,
        
        Unknown
    };
    
    class Keyboard : public Singleton<Keyboard>
    {
    public:
        bool KeyPressed(KeyCode keyCode);
        
        void SetKeyDown(size_t wParam);
        void SetKeyUp(size_t wParam);
        void ResetAllKey();

    private:
        static KeyCode WParamToKeyCode(size_t wParam);

        std::bitset<256> m_keyStates;
    };
}
