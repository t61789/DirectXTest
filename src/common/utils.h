#pragma once
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <optional>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include "nlohmann/json.hpp"
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
        
        static str GetCurrentTimeFormatted();
        static str WStringToString(cr<std::wstring> wstr);
        static str WStringToString(wchar_t* wstr);
        static std::wstring StringToWString(crstr str);
        static str ToString(long hr);
        static str ToAbsPath(crstr relativePath);
        static str ToRelativePath(crstr absPath);

        static size_t GetFileHash(const std::string& path);
        static size_t CombineHash(size_t hash1, size_t hash2);
        static size_t GetMemoryHash(const void* data, size_t sizeB);
        
        static nlohmann::json GetResourceMeta(const std::string& assetPath);
        static str GetResourceMetaPath(crstr assetPath);
        static std::vector<uint8_t> Base64ToBinary(const std::string& base64Str);
        static std::vector<uint32_t> Binary8To32(const std::vector<uint8_t>& data);
        
        static nlohmann::json LoadJson(const std::string& assetPath);
        static void MergeJson(nlohmann::json& json1, const nlohmann::json& json2, bool combineArray = false);
        
        static bool IsVec(cr<nlohmann::json> jsonValue, size_t components);
        static bool IsVec3(cr<nlohmann::json> jsonValue);
        static bool IsVec4(cr<nlohmann::json> jsonValue);
        
        template <typename T>
        static void BinarySerialize(T& obj, crstr path);
        template <class T>
        static void BinaryDeserialize(T& obj, crstr path);
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
    
    template <typename T, size_t N>
    static std::optional<size_t> find_index(const std::array<T, N>& arr, const T& value)
    {
        auto it = std::find(arr.begin(), arr.end(), value);
        if (it == arr.end())
        {
            return std::nullopt;
        }

        return std::distance(arr.begin(), it);
    }
    
    template <typename T>
    static std::optional<size_t> find_index(const std::vector<T>& vec, const T& value)
    {
        auto it = std::find(vec.begin(), vec.end(), value);
        if (it == vec.end())
        {
            return std::nullopt;
        }

        return std::distance(vec.begin(), it);
    }

    template <typename T, typename Predicate>
    static std::optional<size_t> find_index_if(const std::vector<T>& vec, Predicate&& predicate)
    {
        auto it = std::find_if(vec.begin(), vec.end(), predicate);
        if (it == vec.end())
        {
            return std::nullopt;
        }

        return std::distance(vec.begin(), it);
    }
    
    template <typename T, typename Predicate>
    static const T* find_if(const std::vector<T*>& vec, Predicate&& predicate)
    {
        auto it = std::find_if(vec.begin(), vec.end(), predicate);
        if (it == vec.end())
        {
            return nullptr;
        }

        return *it;
    }
    
    template <typename T, typename Predicate>
    static const T* find_if(const std::vector<T>& vec, Predicate&& predicate)
    {
        auto it = std::find_if(vec.begin(), vec.end(), predicate);
        if (it == vec.end())
        {
            return nullptr;
        }

        return &*it;
    }
    
    template <typename T, typename Predicate>
    static T* find_if(std::vector<T*>& vec, Predicate&& predicate)
    {
        auto it = std::find_if(vec.begin(), vec.end(), predicate);
        if (it == vec.end())
        {
            return nullptr;
        }

        return *it;
    }
    
    template <typename T, typename Predicate>
    static T* find_if(std::vector<T>& vec, Predicate&& predicate)
    {
        auto it = std::find_if(vec.begin(), vec.end(), predicate);
        if (it == vec.end())
        {
            return nullptr;
        }

        return &*it;
    }
    
    template <typename T, typename V>
    static const T* find(const std::vector<T*>& vec, V T::* field, const V& value)
    {
        auto it = std::find_if(vec.begin(), vec.end(), [field, &value](T* t)
        {
            return t->*field == value;
        });
        if (it == vec.end())
        {
            return nullptr;
        }

        return *it;
    }

    template <typename T, typename V>
    static const T* find(const std::vector<T>& vec, V T::* field, const V& value)
    {
        auto it = std::find_if(vec.begin(), vec.end(), [field, &value](const T& t)
        {
            return t.*field == value;
        });
        if (it == vec.end())
        {
            return nullptr;
        }

        return &*it;
    }
    
    template <typename T, typename V>
    static T* find(std::vector<T*>& vec, V T::* field, const V& value)
    {
        auto it = std::find_if(vec.begin(), vec.end(), [field, &value](T* t)
        {
            return t->*field == value;
        });
        if (it == vec.end())
        {
            return nullptr;
        }

        return *it;
    }

    template <typename T, typename V>
    static T* find(std::vector<T>& vec, V T::* field, const V& value)
    {
        auto it = std::find_if(vec.begin(), vec.end(), [field, &value](T& t)
        {
            return t.*field == value;
        });
        if (it == vec.end())
        {
            return nullptr;
        }

        return &*it;
    }
    
    template <typename T>
    static T* find(std::vector<std::pair<string_hash, T>>& vec, const string_hash nameId)
    {
        auto p = find_if(vec, [nameId](const std::pair<string_hash, T>& pair)
        {
            return pair.first == nameId;
        });
        if (p)
        {
            return &p->second;
        }

        return nullptr;
    }

    template <typename T>
    static bool exists(const std::vector<T>& vec, const T& value)
    {
        return std::find(vec.begin(), vec.end(), value) != vec.end();
    }

    template <typename T, typename Predicate>
    static bool exists_if(const std::vector<T>& vec, Predicate&& predicate)
    {
        return std::find_if(vec.begin(), vec.end(), predicate) != vec.end();
    }
    
    template <typename T, typename Predicate>
    static void insert(std::vector<T>& v, const T& o, Predicate&& p)
    {
        // predicate: return true when element is on the left of o
        
        auto it = std::partition_point(v.begin(), v.end(), p);

        if (it == v.end())
        {
            v.push_back(o);
        }
        else
        {
            v.insert(it, o);
        }
    }

    template <typename T>
    static void insert(std::vector<std::pair<string_hash, T>>& vec, const string_hash& nameId, const T& o)
    {
        if (auto p = find(vec, nameId))
        {
            *p = o;
        }
        else
        {
            vec.emplace_back(nameId, o);
        }
    }
    
    template <typename T>
    static void remove(std::vector<T>& vec, const T& obj)
    {
        vec.erase(std::remove(vec.begin(), vec.end(), obj), vec.end());
    }
    
    template <typename T>
    static void remove(std::vector<std::pair<string_hash, T>>& vec, const string_hash nameId)
    {
        vec.erase(std::remove_if(vec.begin(), vec.end(), [nameId](const std::pair<string_hash, T>& pair)
        {
            return pair.first == nameId;
        }), vec.end());
    }

    template <typename T, typename Predicate>
    static void remove_if(std::vector<T>& vec, Predicate&& p)
    {
        vec.erase(std::remove_if(vec.begin(), vec.end(), p), vec.end());
    }

    template <typename T>
    void Utils::BinarySerialize(T& obj, crstr path)
    {
        auto absPath = ToAbsPath(path);
        auto parentDirPath = std::filesystem::path(absPath).parent_path();
        if (!exists(parentDirPath))
        {
            create_directories(parentDirPath);
        }
        
        std::ofstream ofs(absPath, std::ios::binary);
        if (!ofs)
        {
            throw std::runtime_error(format_log(LOG_ERROR, "Unable to serialize: %s", path.c_str()));
        }
        
        boost::archive::binary_oarchive oa(ofs);
        oa << obj;
    }

    template <typename T>
    void Utils::BinaryDeserialize(T& obj, crstr path)
    {
        auto absPath = ToAbsPath(path);
        std::ifstream ifs(absPath, std::ios::binary);
        if (!ifs)
        {
            throw std::runtime_error(format_log(LOG_ERROR, "Unable to deserialize: %s", path.c_str()));
        }
    
        boost::archive::binary_iarchive ia(ifs);
        ia >> obj;
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
