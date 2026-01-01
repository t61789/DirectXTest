#include "utils.h"

#include <chrono>
#include <iomanip>
#include <windows.h>
#include <comdef.h>
#include <filesystem>
#include <fstream>
#include <boost/beast/core/detail/base64.hpp>

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

    std::wstring Utils::StringToWString(crstr str)
    {
        if (str.empty())
        {
            return L"";
        }
    
        int charsNeeded = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
        if (charsNeeded == 0)
        {
            return L"";
        }
    
        std::wstring result(charsNeeded - 1, 0); // -1 因为包含null终止符
        MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &result[0], charsNeeded);
        return result;
    }

    str Utils::ToString(const long hr)
    {
        _com_error err(hr);
        auto errMsg = err.ErrorMessage();
        return errMsg;
    }

    str Utils::ToAbsPath(crstr relativePath)
    {
        return (std::filesystem::current_path() / relativePath).generic_string();
    }
    
    str Utils::ToRelativePath(crstr absPath)
    {
        return relative(absPath, std::filesystem::current_path()).generic_string();
    }

    bool Utils::EndsWith(crstr str, crstr suffix)
    {
        if (suffix.length() > str.length())
        {
            return false;
        }
        
        return str.rfind(suffix) == str.length() - suffix.length();
    }

    size_t Utils::GetFileHash(const std::string& path)
    {
        auto filename = ToAbsPath(path);
        if (std::filesystem::is_directory(filename))
        {
            size_t result = 0;
            for (const auto& entry : std::filesystem::directory_iterator(filename))
            {
                if (entry.is_regular_file())
                {
                    auto hash = GetFileHash(ToRelativePath(entry.path().generic_string()));
                    result = CombineHash(result, hash);
                }
            }

            return result;
        }
        
        std::ifstream file(filename, std::ios::binary);
        if (!file)
        {
            throw std::runtime_error("无法打开文件");
        }
    
        std::hash<std::string> hasher;
        size_t finalHash = 0;
        char buffer[4096];
    
        while (file.read(buffer, sizeof(buffer)))
        {
            std::string chunk(buffer, sizeof(buffer));
            size_t chunkHash = hasher(chunk);
            finalHash = CombineHash(finalHash, chunkHash);
        }
    
        if (file.gcount() > 0)
        {
            std::string lastChunk(buffer, file.gcount());
            size_t chunkHash = hasher(lastChunk);
            finalHash = CombineHash(finalHash, chunkHash);
        }
    
        return finalHash;
    }

    size_t Utils::CombineHash(const size_t hash1, const size_t hash2)
    {
        return hash1 ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash2 >> 2));
    }

    size_t Utils::GetMemoryHash(const void* data, const size_t sizeB)
    {
        static std::hash<std::string> hasher;
        return hasher(std::string(static_cast<const char*>(data), sizeB));
    }
    
    bool Utils::AssetExists(const std::string& path)
    {
        auto absPath = ToAbsPath(path);
        return std::filesystem::exists(absPath);
    }

    nlohmann::json Utils::GetResourceMeta(const std::string& assetPath)
    {
        auto metaPath = GetResourceMetaPath(assetPath);
        if (!std::filesystem::exists(ToAbsPath(metaPath)))
        {
            return nlohmann::json::object();
        }

        return LoadJson(metaPath);
    }
    
    str Utils::GetResourceMetaPath(crstr assetPath)
    {
        return assetPath + ".meta";
    }

    std::vector<uint8_t> Utils::Base64ToBinary(const std::string& base64Str)
    {
        std::size_t decodedSize = boost::beast::detail::base64::decoded_size(base64Str.size());
        std::vector<uint8_t> binaryData(decodedSize);
    
        auto result = boost::beast::detail::base64::decode(
            binaryData.data(), 
            base64Str.data(), 
            base64Str.size());
    
        binaryData.resize(result.first);
        return binaryData;
    }
    
    std::vector<uint32_t> Utils::Binary8To32(const std::vector<uint8_t>& data)
    {
        std::vector<uint32_t> result;
        result.resize(data.size() / 4);
        memcpy(result.data(), data.data(), data.size());
        return result;
    }
    
    nlohmann::json Utils::LoadJson(const std::string& assetPath)
    {
        auto s = std::ifstream(ToAbsPath(assetPath));
        nlohmann::json json;
        s >> json;
        s.close();

        return json;
    }

    void Utils::MergeJson(nlohmann::json& json1, const nlohmann::json& json2, bool combineArray)
    {
        for (auto& it : json2.items())
        {
            const auto& key = it.key();
            const auto& value = it.value();

            if (!json1.contains(key))
            {
                // 1里没这个key，就直接添加
                json1[key] = value;
                continue;
            }

            if (combineArray && value.type() == nlohmann::json::value_t::array && json1[key].type() == nlohmann::json::value_t::array)
            {
                // 是数组就将2的加在1的后面
                json1[key].insert(json1[key].end(), value.begin(), value.end());
                continue;
            }

            if (value.type() == nlohmann::json::value_t::object && json1[key].type() == nlohmann::json::value_t::object)
            {
                // 是dict就递归合并
                MergeJson(json1[key], value);
                continue;
            }

            json1[key] = value;
        }
    }
    
    bool Utils::IsVec(cr<nlohmann::json> jsonValue, const size_t components)
    {
        if (jsonValue.is_array() && jsonValue.size() == components)
        {
            bool allFloat = true;
            for (auto& e : jsonValue)
            {
                allFloat &= e.is_number();
            }
            return allFloat;
        }

        return false;
    }
    
    bool Utils::IsVec3(cr<nlohmann::json> jsonValue)
    {
        return IsVec(jsonValue, 3);
    }
    
    bool Utils::IsVec4(cr<nlohmann::json> jsonValue)
    {
        return IsVec(jsonValue, 4);
    }
    
    std::string Utils::Join(const std::vector<std::string>& strings, const std::string& delimiter)
    {
        std::stringstream ss;
        for (size_t i = 0; i < strings.size(); ++i)
        {
            ss << strings[i];
            if (i != strings.size() - 1)
            {
                ss << delimiter;
            }
        }
        return ss.str();
    }

    bool Utils::IsMainThread()
    {
        static std::thread::id mainTheadId = std::this_thread::get_id();
        return std::this_thread::get_id() == mainTheadId;
    }

}
