#pragma once
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>

#include "const.h"

namespace dt
{
    
    enum LogType : uint8_t
    {
        LOG_INFO,
        LOG_WARNING,
        LOG_ERROR
    };
    
    class Utils
    {
    public:
        static vec<str> s_logs;
        static std::mutex s_logMutex;
        
        static std::string GetCurrentTimeFormatted();
        static str WStringToString(cr<std::wstring> wstr);
        static str WStringToString(wchar_t* wstr);
    };
    
    template <typename... Args>
    static std::string format_string(const std::string& format, Args... args)
    {
        int size = std::snprintf(nullptr, 0, format.c_str(), args...) + 1; // 计算最终格式化字符串所需空间
        if (size <= 0) {
            throw std::runtime_error("Error during formatting.");
        }
        std::unique_ptr<char[]> buf(new char[size]); // 分配临时缓冲区
        std::snprintf(buf.get(), size, format.c_str(), args...); // 格式化字符串
        return std::string(buf.get(), buf.get() + size - 1); // 返回 std::string（去掉末尾的 \0）
    }

    template <typename... Args>
    std::string format_log(const LogType type, const std::string& format, Args... args)
    {
        std::stringstream ss;
        ss << "[";
        ss << Utils::GetCurrentTimeFormatted();
        ss << "]";
        switch (type)
        {
            case LogType::LOG_INFO:
                ss<<"[Info] ";
            break;
            case LogType::LOG_WARNING:
                ss<<"[Warning] ";
            break;
            case LogType::LOG_ERROR:
                ss<<"[Error] ";
            break;
        }

        ss << format_string(format, args...);
        return ss.str();
    }

    template <typename... Args>
    static void log(const LogType logType, const std::string& msg, Args... args)
    {
        std::lock_guard lock(Utils::s_logMutex);
        
        auto logStr = format_log(logType, msg, args...);
        Utils::s_logs.push_back(logStr);
        if (Utils::s_logs.size() > 50)
        {
            Utils::s_logs.erase(Utils::s_logs.begin());
        }
        
        std::cout << logStr << '\n' << std::flush;
    }
    
    template <typename... Args>
    static void log_info(const std::string& msg, Args... args)
    {
        return log(LOG_INFO, msg, args...);
    }

    template <typename... Args>
    static void log_warning(const std::string& msg, Args... args)
    {
        return log(LOG_WARNING, msg, args...);
    }

    template <typename... Args>
    static void log_error(const std::string& msg, Args... args)
    {
        return log(LOG_ERROR, msg, args...);
    }
    
    template<typename T>
    class Singleton
    {
    public:
        Singleton(const Singleton& other) = delete;
        Singleton(Singleton&& other) noexcept = delete;
        Singleton& operator=(const Singleton& other) = delete;
        Singleton& operator=(Singleton&& other) noexcept = delete;

        ~Singleton() { m_instance = nullptr; }
        static T* Ins() { return m_instance; }
        
    private:
        Singleton() { m_instance = static_cast<T*>(this); }
        
        inline static T* m_instance = nullptr;
        
        friend T;
    };
}
