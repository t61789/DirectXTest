#include "utils.h"

#include <chrono>
#include <iomanip>
#include <windows.h>

namespace dt
{
    vec<str> Utils::s_logs;
    std::mutex Utils::s_logMutex;
    
    std::string Utils::GetCurrentTimeFormatted()
    {
        // 获取当前时间点
        auto now = std::chrono::system_clock::now();

        // 转换为 time_t 格式
        std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);

        // 提取毫秒部分
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        // 使用 localtime_s 提供线程安全的本地时间
        std::tm now_tm;
        localtime_s(&now_tm, &now_time_t); // 安全的本地时间转换

        // 使用字符串流格式化时间
        std::ostringstream oss;
        oss << std::put_time(&now_tm, "%H:%M:%S") // 格式化小时、分钟、秒
            << "." << std::setfill('0') << std::setw(3) << now_ms.count(); // 添加毫秒部分

        return oss.str();
    }

    str Utils::WStringToString(cr<std::wstring> wstr)
    {
        if (wstr.empty())
        {
            return "";
        }
        
        int size_needed = WideCharToMultiByte(
            CP_UTF8,
            0,
            wstr.c_str(),
            static_cast<int>(wstr.size()),
            nullptr,
            0,
            nullptr,
            nullptr);
        std::string result(size_needed, 0);
        WideCharToMultiByte(
            CP_UTF8,
            0,
            wstr.c_str(),
            static_cast<int>(wstr.size()), 
            result.data(),
            size_needed,
            nullptr,
            nullptr);
        return result;
    }

    str Utils::WStringToString(wchar_t* wstr)
    {
        if (!wstr)
        {
            return "";
        }
        return WStringToString(std::wstring(wstr));
    }
}
