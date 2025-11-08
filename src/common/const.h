#pragma once
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

namespace dt
{
    #define THROW_IF(cond, msg) if (cond) throw std::runtime_error(msg)
    #define Assert(cond) assert(!!(cond))
    
    template <typename T>
    using up = std::unique_ptr<T>;
    template <typename T>
    using wp = std::weak_ptr<T>;
    template <typename T>
    using sp = std::shared_ptr<T>;
    template <typename T>
    using cr = const T&;
    
    template <typename T>
    using vec = std::vector<T>;
    template <typename T>
    using vecsp = std::vector<sp<T>>;
    template <typename T>
    using vecwp = std::vector<wp<T>>;
    template <typename T>
    using vecup = std::vector<up<T>>;
    template <typename T>
    using vecpt = std::vector<T*>;
    template <typename T>
    using cvec = const vec<T>;
    template <typename T>
    using crvec = cr<vec<T>>;
    template <typename T>
    using crvecsp = cr<vecsp<T>>;
    template <typename T>
    using crvecwp = cr<vecwp<T>>;
    template <typename T>
    using crvecpt = cr<vecpt<T>>;
    
    template <typename K, typename V>
    using umap = std::unordered_map<K, V>;
    template <typename K, typename V>
    using crumap = cr<umap<K, V>>;
    template <typename T>
    using crsp = const std::shared_ptr<T>&;
    template <typename T>
    using crwp = cr<wp<T>>;
    template <typename T>
    using cpt = const T*;
    template <typename T>
    using c = const T;
    using cstr = const char*;
    template <typename T, size_t N>
    using arr = std::array<T, N>;
    using str = std::string;
    using crstr = cr<std::string>;
    
    template <typename T, typename... Args>
    std::unique_ptr<T> mup(Args&&... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }
    template <typename T, typename... Args>
    std::shared_ptr<T> msp(Args&&... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
}
