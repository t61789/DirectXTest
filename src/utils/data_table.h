#pragma once
#include "common/const.h"

namespace dt
{
    class ITexture;
    
    struct DataParamInfo
    {
        StringHandle key;
        vec<uint8_t> data;
        sp<ITexture> texture = nullptr;
    };

    class DataTable
    {
    public:
        void Set(crstrh key, const void* data, size_t sizeB);
        bool Set(string_hash key, const void* data, size_t sizeB);
        bool Set(string_hash key, sp<ITexture> texture);

        bool Get(string_hash key, void* data, size_t sizeB);
        
        bool Exists(string_hash key);

        template <typename F>
        void ForeachParam(F&& f);
        void AddParam(crstrh key, cr<nlohmann::json> value);
        void AddParam(crstrh key, size_t sizeB);

    private:
        DataParamInfo* GetParamInfo(string_hash key);
        
        static bool IsTextureParam(cr<StringHandle> name);
        
        vec<DataParamInfo> m_params;
    };

    template <typename F>
    void DataTable::ForeachParam(F&& f)
    {
        for (auto& param : m_params)
        {
            f(param);
        }
    }
}
