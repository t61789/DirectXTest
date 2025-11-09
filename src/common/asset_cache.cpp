#include "asset_cache.h"

namespace dt
{
    size_t AssetCache::GetAssetFileHash(crstr assetPath)
    {
        auto assetHash = Utils::GetFileHash(assetPath);
        auto assetMetaFilePath = Utils::GetResourceMetaPath(assetPath);
        if (std::filesystem::exists(assetMetaFilePath))
        {
            assetHash = Utils::CombineHash(assetHash, Utils::GetFileHash(assetMetaFilePath));
        }

        return assetHash;
    }

    std::string AssetCache::GetAssetCachePath(cr<std::string> path)
    {
        return "cache/" + path + ".cache";
    }

    std::string AssetCache::GetAssetCacheMetaPath(cr<std::string> path)
    {
        return "cache/" + path + ".cacheMeta";
    }
}
