#pragma once
#include <boost/serialization/vector.hpp>

#include "const.h"
#include "utils.h"

namespace dt
{
    class AssetCache
    {
    public:
        template <typename BasicType, typename CacheType>
        static sp<BasicType> GetFromCache(cr<str> assetPath);

    private:
        struct AssetCacheMeta
        {
            size_t objFileHash = 0;

            template <class Archive>
            void serialize(Archive& ar, unsigned int version);
        };
    
        template <class BasicType, class CacheType>
        static void TryCacheAsset(crstr assetPath);
        template <typename BasicType, typename CacheType>
        static void DoCache(cr<str> assetPath);
        static size_t GetAssetFileHash(crstr assetPath);
        
        static str GetAssetCachePath(cr<str> path);
        static str GetAssetCacheMetaPath(cr<str> path);
    };

    template <class Archive>
    void AssetCache::AssetCacheMeta::serialize(Archive& ar, unsigned int version)
    {
        ar & objFileHash;
    }

    template <typename BasicType, typename CacheType>
    sp<BasicType> AssetCache::GetFromCache(cr<str> assetPath)
    {
        TryCacheAsset<BasicType, CacheType>(assetPath);
        
        CacheType assetCache;
        Utils::BinaryDeserialize(assetCache, GetAssetCachePath(assetPath));

        return BasicType::CreateAssetFromCache(std::move(assetCache));
    }

    template <typename BasicType, typename CacheType>
    void AssetCache::TryCacheAsset(crstr assetPath)
    {
        auto assetCachePath = GetAssetCachePath(assetPath);
        auto assetCacheMetaPath = GetAssetCacheMetaPath(assetPath);
        auto absAssetCachePath = Utils::ToAbsPath(assetCachePath);
        auto absAssetCacheMetaPath = Utils::GetResourceMetaPath(assetCacheMetaPath);

        auto needDoCache = !std::filesystem::exists(absAssetCachePath) || !std::filesystem::exists(absAssetCacheMetaPath);
        if (!needDoCache)
        {
            AssetCacheMeta assetCacheMeta;
            Utils::BinaryDeserialize(assetCacheMeta, assetCacheMetaPath);
            needDoCache = assetCacheMeta.objFileHash != GetAssetFileHash(assetPath);
        }
        
        if (needDoCache)
        {
            DoCache<BasicType, CacheType>(assetPath);
        }
    }

    template <typename BasicType, typename CacheType>
    void AssetCache::DoCache(crstr assetPath)
    {
        AssetCacheMeta assetCacheMeta;
        assetCacheMeta.objFileHash = GetAssetFileHash(assetPath);
        Utils::BinarySerialize(assetCacheMeta, GetAssetCacheMetaPath(assetPath));

        auto assetCachePath = GetAssetCachePath(assetPath);
        CacheType assetCache = BasicType::CreateCacheFromAsset(assetPath);
        Utils::BinarySerialize(assetCache, assetCachePath);

        log_info("Cache asset: %s", assetPath.c_str());
    }
}
