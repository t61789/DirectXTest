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
    
        template <typename BasicType, typename CacheType>
        static CacheType LoadCache(crstr assetPath);
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
        CacheType assetCache = LoadCache<BasicType, CacheType>(assetPath);

        return BasicType::CreateAssetFromCache(std::move(assetCache));
    }

    template <typename BasicType, typename CacheType>
    CacheType AssetCache::LoadCache(crstr assetPath)
    {
        auto assetCachePath = GetAssetCachePath(assetPath);
        auto assetCacheMetaPath = GetAssetCacheMetaPath(assetPath);
        auto absAssetCachePath = Utils::ToAbsPath(assetCachePath);
        auto absAssetCacheMetaPath = Utils::ToAbsPath(assetCacheMetaPath);

        auto needDoCache = !std::filesystem::exists(absAssetCachePath) || !std::filesystem::exists(absAssetCacheMetaPath);
        if (!needDoCache)
        {
            // Check if cache's file hash matches asset's file hash
            AssetCacheMeta assetCacheMeta;
            Utils::BinaryDeserialize(assetCacheMeta, assetCacheMetaPath);
            needDoCache = assetCacheMeta.objFileHash != GetAssetFileHash(assetPath);
        }

        if (!needDoCache)
        {
            // Load cache from file directly
            CacheType assetCache;
            Utils::BinaryDeserialize(assetCache, GetAssetCachePath(assetPath));
            return std::move(assetCache);
        }

        // Create cache in runtime, and serialize it to the disk
        
        auto assetCache = BasicType::CreateCacheFromAsset(assetPath);
        Utils::BinarySerialize(assetCache, assetCachePath);
        
        AssetCacheMeta assetCacheMeta;
        assetCacheMeta.objFileHash = GetAssetFileHash(assetPath);
        Utils::BinarySerialize(assetCacheMeta, GetAssetCacheMetaPath(assetPath));
        
        log_info("Cache asset: %s", assetPath.c_str());

        return std::move(assetCache);
    }
}
