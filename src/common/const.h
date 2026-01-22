#pragma once
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "string_handle.h"
#include "utils/simple_list.h"

namespace dt
{
    #define TRACY_IDLE_COLOR 0x25281E

    #define JOB_THREAD_COUNT 3

    #define SRV_DESC_POOL_SIZE 0xFFFF
    #define SAMPLER_DESC_POOL_SIZE 0xFF
    #define RTV_DESC_POOL_SIZE 128

    #define MAX_POINT_LIGHT_COUNT 5
    
    #define PI 3.1415926535f
    #define DEG2RAD 0.0174532925f
    #define RAD2DEG 57.2957795f
    #define EPSILON 1e-6
    
    #define THROW_ERROR(msg) throw std::runtime_error(format_log(LOG_ERROR, msg));
    #define THROW_ERRORF(msg, ...) throw std::runtime_error(format_log(LOG_ERROR, msg, __VA_ARGS__));
    #define THROW_IF(cond, msg) if (cond) throw std::runtime_error(msg)
    #define THROW_IF_FAILED(cond) if (auto hr = (cond); FAILED(hr)) throw std::runtime_error("The execution of d3d command failed: " + Utils::ToString(hr))
    #define ASSERT_THROW(cond) if (!(cond)) throw std::runtime_error("Assertion failed: " + std::string(#cond))
    #define ASSERT_THROWM(cond, msg) if (!(cond)) throw std::runtime_error(msg)

    #define ELEM_TYPE(v) decltype(v)::value_type
    #define CR_ELEM_TYPE(v) cr<decltype(v)::value_type>

    #define STRING_HANDLE(KEY, VALUE) static auto KEY = StringHandle(#VALUE);

    STRING_HANDLE(GLOBAL_CBUFFER, GlobalCBuffer)
    STRING_HANDLE(PER_VIEW_CBUFFER, PerViewCBuffer)
    STRING_HANDLE(PER_OBJECT_CBUFFER, PerObjectCBuffer)
    STRING_HANDLE(ROOT_CONSTANTS_CBUFFER, RootConstantsCBuffer)
    STRING_HANDLE(PER_MATERIAL_CBUFFER, PerMaterialCBuffer)

    STRING_HANDLE(V, _V)
    STRING_HANDLE(P, _P)
    STRING_HANDLE(VP, _VP)
    STRING_HANDLE(IVP, _IVP)
    STRING_HANDLE(MVP, _MVP)
    STRING_HANDLE(ITM, _ITM)
    STRING_HANDLE(M, _M)
    STRING_HANDLE(IM, _IM)
    STRING_HANDLE(CAMERA_POSITION_WS, _CameraPositionWS)
    STRING_HANDLE(BINDLESS_2D_TEXTURES, _Bindless2dTextures)
    STRING_HANDLE(BINDLESS_CUBE_TEXTURES, _BindlessCubeTextures)
    STRING_HANDLE(BINDLESS_BYTE_BUFFERS, _BindlessByteBuffers)
    STRING_HANDLE(BINDLESS_SAMPLERS, _BindlessSamplers)
    STRING_HANDLE(GBUFFER_0_TEX, _GBuffer0Tex)
    STRING_HANDLE(GBUFFER_1_TEX, _GBuffer1Tex)
    STRING_HANDLE(GBUFFER_2_TEX, _GBuffer2Tex)
    STRING_HANDLE(MAIN_TEX, _MainTex)
    STRING_HANDLE(SKYBOX_TEX, _SkyboxTex)
    STRING_HANDLE(SCREEN_SIZE, _ScreenSize)
    STRING_HANDLE(EXPOSURE, _Exposure)
    STRING_HANDLE(MAIN_LIGHT_DIR, _MainLightDir)
    STRING_HANDLE(MAIN_LIGHT_COLOR, _MainLightColor)
    STRING_HANDLE(POINT_LIGHT_COUNT, _PointLightCount)
    STRING_HANDLE(POINT_LIGHT_INFOS, _PointLightInfos)
    STRING_HANDLE(SHC, _Shc)
    STRING_HANDLE(MAIN_LIGHT_SHADOW_TEX, _MainLightShadowTex)
    STRING_HANDLE(MAIN_LIGHT_SHADOW_VP, _MainLightShadowVP)
    STRING_HANDLE(MAIN_LIGHT_SHADOW_RANGE, _MainLightShadowRange)
    STRING_HANDLE(BATCH_MATRICES, _BatchMatrices)
    STRING_HANDLE(BATCH_INDICES, _BatchIndices)
    STRING_HANDLE(BASE_INSTANCE_ID, _BaseInstanceId)
    STRING_HANDLE(ENABLE_INSTANCING, ENABLE_INSTANCING)

    static constexpr uint32_t MAX_REGISTER_COUNT = 16;
    static constexpr uint32_t ROOT_CONSTANTS_CBUFFER_REGISTER_INDEX = 3;
    static constexpr uint32_t ROOT_CONSTANTS_CBUFFER_REGISTER_SPACE = 1;
    
    static constexpr uint32_t ROOT_CONSTANTS_BATCH_INDICES_BUFFER_DWORD = 0;
    static constexpr uint32_t ROOT_CONSTANTS_BATCH_INDICES_OFFSET_DWORD = 1;
    static constexpr uint32_t ROOT_CONSTANTS_CBUFFER_SIZE_DWORD = 2;
    
    static const auto UNNAMED_OBJECT = StringHandle("Unnamed Object");
    
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
    using vecpair = std::vector<std::pair<K, V>>;
    template <typename K, typename V>
    using crvecpair = cr<vecpair<K, V>>;
    template <typename K, typename V>
    using crpair = cr<std::pair<K, V>>;
    
    template <typename K, typename V>
    using umap = std::unordered_map<K, V>;
    template <typename K, typename V>
    using crumap = cr<umap<K, V>>;
    template <typename K>
    using uset = std::unordered_set<K>;
    template <typename K>
    using cruset = cr<uset<K>>;
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
    using crstrh = cr<StringHandle>;
    using string_hash = size_t;
    template <typename Sig>
    using func = std::function<Sig>;
    using task = std::function<void()>;
    template <typename T>
    using sl = SimpleList<T>;
    using vectask = vec<func<void()>>;
    
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
    
    enum class VertexAttr : std::uint8_t
    {
        POSITION_OS,
        NORMAL_OS,
        TANGENT_OS,
        UV0,
        UV1,
        COUNT
    };

    struct VertexAttrDefine
    {
        VertexAttr attr;
        uint32_t index;
        uint32_t strideF;
        uint32_t offsetF;
        StringHandle name;
    };

    inline std::vector<VertexAttrDefine> VERTEX_ATTR_DEFINES =
    {
        {VertexAttr::POSITION_OS, static_cast<uint32_t>(VertexAttr::POSITION_OS), 4, 0, "positionOS"},
        {VertexAttr::NORMAL_OS, static_cast<uint32_t>(VertexAttr::NORMAL_OS), 4, 4, "normalOS"},
        {VertexAttr::TANGENT_OS, static_cast<uint32_t>(VertexAttr::TANGENT_OS), 4, 8, "tangentOS"},
        {VertexAttr::UV0, static_cast<uint32_t>(VertexAttr::UV0), 2, 12, "uv0"},
        {VertexAttr::UV1, static_cast<uint32_t>(VertexAttr::UV1), 2, 14, "uv1"},
    };
    
    static constexpr uint32_t MAX_VERTEX_ATTR_STRIDE_F = 16;

    inline std::unordered_map<VertexAttr, uint32_t> VERTEX_ATTR_STRIDE =
    {
        {VertexAttr::POSITION_OS, 4},
        {VertexAttr::NORMAL_OS, 4},
        {VertexAttr::TANGENT_OS, 4},
        {VertexAttr::UV0, 2},
        {VertexAttr::UV1, 2},
    };

    inline std::unordered_map<VertexAttr, std::string> VERTEX_ATTR_NAME =
    {
        {VertexAttr::POSITION_OS, "positionOS"},
        {VertexAttr::NORMAL_OS, "normalOS"},
        {VertexAttr::TANGENT_OS, "tangentOS"},
        {VertexAttr::UV0, "uv0"},
        {VertexAttr::UV1, "uv1"},
    };
    
    inline vec<string_hash> PREDEFINED_CBUFFER = {
        GLOBAL_CBUFFER.Hash(),
        PER_VIEW_CBUFFER.Hash(),
        PER_OBJECT_CBUFFER.Hash(),
        ROOT_CONSTANTS_CBUFFER.Hash()
    };
}
