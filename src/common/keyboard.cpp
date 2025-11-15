#include "keyboard.h"

#include <window.h>

namespace dt
{
    bool Keyboard::KeyPressed(const KeyCode keyCode)
    {
        return m_keyStates.test(static_cast<size_t>(keyCode));
    }

    void Keyboard::SetKeyDown(const size_t wParam)
    {
        m_keyStates.set(static_cast<size_t>(WParamToKeyCode(wParam)), true);
    }

    void Keyboard::SetKeyUp(const size_t wParam)
    {
        m_keyStates.set(static_cast<size_t>(WParamToKeyCode(wParam)), false);
    }

    void Keyboard::ResetAllKey()
    {
        m_keyStates.reset();
    }

    KeyCode Keyboard::WParamToKeyCode(const size_t wParam)
    {
        static std::unordered_map<size_t, KeyCode> wparamToKeyCode = {
            
            // 字母键
            {'A', KeyCode::A}, {'B', KeyCode::B}, {'C', KeyCode::C}, {'D', KeyCode::D},
            {'E', KeyCode::E}, {'F', KeyCode::F}, {'G', KeyCode::G}, {'H', KeyCode::H},
            {'I', KeyCode::I}, {'J', KeyCode::J}, {'K', KeyCode::K}, {'L', KeyCode::L},
            {'M', KeyCode::M}, {'N', KeyCode::N}, {'O', KeyCode::O}, {'P', KeyCode::P},
            {'Q', KeyCode::Q}, {'R', KeyCode::R}, {'S', KeyCode::S}, {'T', KeyCode::T},
            {'U', KeyCode::U}, {'V', KeyCode::V}, {'W', KeyCode::W}, {'X', KeyCode::X},
            {'Y', KeyCode::Y}, {'Z', KeyCode::Z},
            
            // 数字键
            {'0', KeyCode::Num0}, {'1', KeyCode::Num1}, {'2', KeyCode::Num2}, {'3', KeyCode::Num3},
            {'4', KeyCode::Num4}, {'5', KeyCode::Num5}, {'6', KeyCode::Num6}, {'7', KeyCode::Num7},
            {'8', KeyCode::Num8}, {'9', KeyCode::Num9},
            
            // 功能键
            {VK_F1, KeyCode::F1}, {VK_F2, KeyCode::F2}, {VK_F3, KeyCode::F3}, {VK_F4, KeyCode::F4},
            {VK_F5, KeyCode::F5}, {VK_F6, KeyCode::F6}, {VK_F7, KeyCode::F7}, {VK_F8, KeyCode::F8},
            {VK_F9, KeyCode::F9}, {VK_F10, KeyCode::F10}, {VK_F11, KeyCode::F11}, {VK_F12, KeyCode::F12},
            {VK_F13, KeyCode::F13}, {VK_F14, KeyCode::F14}, {VK_F15, KeyCode::F15}, {VK_F16, KeyCode::F16},
            {VK_F17, KeyCode::F17}, {VK_F18, KeyCode::F18}, {VK_F19, KeyCode::F19}, {VK_F20, KeyCode::F20},
            {VK_F21, KeyCode::F21}, {VK_F22, KeyCode::F22}, {VK_F23, KeyCode::F23}, {VK_F24, KeyCode::F24},
            
            // 方向键
            {VK_LEFT, KeyCode::Left}, {VK_RIGHT, KeyCode::Right},
            {VK_UP, KeyCode::Up}, {VK_DOWN, KeyCode::Down},
            
            // 特殊键
            {VK_SPACE, KeyCode::Space}, {VK_RETURN, KeyCode::Enter},
            {VK_ESCAPE, KeyCode::Escape}, {VK_BACK, KeyCode::Backspace},
            {VK_TAB, KeyCode::Tab}, {VK_CAPITAL, KeyCode::CapsLock},
            
            // 修饰键
            {VK_LSHIFT, KeyCode::LeftShift}, {VK_RSHIFT, KeyCode::RightShift},
            {VK_LCONTROL, KeyCode::LeftControl}, {VK_RCONTROL, KeyCode::RightControl},
            {VK_LMENU, KeyCode::LeftAlt}, {VK_RMENU, KeyCode::RightAlt},
            
            // 系统键
            {VK_LWIN, KeyCode::LeftWin}, {VK_RWIN, KeyCode::RightWin},
            {VK_MENU, KeyCode::Menu},
            
            // 编辑键
            {VK_INSERT, KeyCode::Insert}, {VK_DELETE, KeyCode::Delete},
            {VK_HOME, KeyCode::Home}, {VK_END, KeyCode::End},
            {VK_PRIOR, KeyCode::PageUp}, {VK_NEXT, KeyCode::PageDown},
            
            // 数字小键盘
            {VK_NUMPAD0, KeyCode::NumPad0}, {VK_NUMPAD1, KeyCode::NumPad1},
            {VK_NUMPAD2, KeyCode::NumPad2}, {VK_NUMPAD3, KeyCode::NumPad3},
            {VK_NUMPAD4, KeyCode::NumPad4}, {VK_NUMPAD5, KeyCode::NumPad5},
            {VK_NUMPAD6, KeyCode::NumPad6}, {VK_NUMPAD7, KeyCode::NumPad7},
            {VK_NUMPAD8, KeyCode::NumPad8}, {VK_NUMPAD9, KeyCode::NumPad9},
            {VK_MULTIPLY, KeyCode::Multiply}, {VK_ADD, KeyCode::Add},
            {VK_SUBTRACT, KeyCode::Subtract}, {VK_DECIMAL, KeyCode::Decimal},
            {VK_DIVIDE, KeyCode::Divide}, {VK_NUMLOCK, KeyCode::NumLock},
            
            // 符号键
            {VK_OEM_COMMA, KeyCode::Comma}, {VK_OEM_PERIOD, KeyCode::Period},
            {VK_OEM_1, KeyCode::Semicolon}, {VK_OEM_7, KeyCode::Quote},
            {VK_OEM_4, KeyCode::LeftBracket}, {VK_OEM_6, KeyCode::RightBracket},
            {VK_OEM_5, KeyCode::Backslash}, {VK_OEM_MINUS, KeyCode::Minus},
            {VK_OEM_PLUS, KeyCode::Plus}, {VK_OEM_3, KeyCode::Grave},
            
            // 其他
            {VK_SNAPSHOT, KeyCode::PrintScreen}, {VK_SCROLL, KeyCode::ScrollLock},
            {VK_PAUSE, KeyCode::Pause}, {VK_APPS, KeyCode::Apps}
        };

        return wparamToKeyCode.at(wParam);
    }
}
