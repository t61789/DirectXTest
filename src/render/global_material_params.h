#pragma once
#include <DirectXMath.h>

#include "common/const.h"
#include "common/data_set.h"

namespace dt
{
    class Image;
    class Material;

    class GlobalMaterialParams : public Singleton<GlobalMaterialParams>
    {
    public:
        GlobalMaterialParams();
        ~GlobalMaterialParams();
        GlobalMaterialParams(const GlobalMaterialParams& other) = delete;
        GlobalMaterialParams(GlobalMaterialParams&& other) noexcept = delete;
        GlobalMaterialParams& operator=(const GlobalMaterialParams& other) = delete;
        GlobalMaterialParams& operator=(GlobalMaterialParams&& other) noexcept = delete;

        template <typename T>
        void SetParam(string_hash name, T val);
        void SetParam(string_hash name, const float* arr, uint32_t count);
        void SetParam(string_hash name, crsp<Image> image);
        template <typename T>
        T GetParam(string_hash name);

        void RegisterParam(string_hash nameId, Material* material);
        void UnregisterParam(string_hash nameId, Material* material);

    private:
        struct Param
        {
            string_hash nameId;
            uint8_t sizeB;
            sp<Image> image = nullptr;
            vec<Material*> listenerMaterials;
        };
        
        void SetParam(string_hash nameId, const void* val, size_t sizeB);
        bool GetParam(string_hash nameId, void* val, size_t sizeB);

        Param* CreateParam(string_hash nameId, size_t sizeB);
        
        template <typename T>
        static void TypeCheck();
        
        sp<DataSet> m_dataSet;
        vecpair<string_hash, Param> m_params;
    };
    
    template <typename T>
    void GlobalMaterialParams::SetParam(const string_hash name, T val)
    {
        TypeCheck<T>();
        
        SetParam(name, &val, sizeof(T));
    }

    template <typename T>
    T GlobalMaterialParams::GetParam(const string_hash name)
    {
        TypeCheck<T>();
        
        T val;
        GetParam(name, &val, sizeof(T));
        return val;
    }
    
    template <typename T>
    void GlobalMaterialParams::TypeCheck()
    {
        static_assert(
            std::is_same_v<T, DirectX::XMFLOAT4>||
            std::is_same_v<T, DirectX::XMFLOAT4X4>||
            "Invalid type");
    }
}
