#pragma once


#include "comp.h"
#include "common/event.h"
#include "common/math.h"

namespace dt
{
    class TransformComp final : public Comp
    {
        template <typename T>
        class TransformCompProp
        {
        public:
            T localVal;
            T worldVal = T();
            bool needUpdateFromWorld = false;

            TransformCompProp(T localVal) : localVal(localVal) {}
        };
        
    public:
        Event<> dirtyEvent;

        void Awake() override;

        XMVECTOR GetWorldPosition();
        void SetWorldPosition(const XMVECTOR& pos);
        XMVECTOR GetPosition();
        void SetPosition(const XMVECTOR& pos);
        XMVECTOR GetScale();
        void SetScale(const XMVECTOR& scale);
        XMVECTOR GetRotation(); // TODO 世界空间下的旋转
        void SetRotation(const XMVECTOR& rotation);
        XMVECTOR GetEulerAngles();
        void SetEulerAngles(const XMVECTOR& ea);

        bool HasOddNegativeScale();
    
        void UpdateMatrix();
    
        const XMMATRIX& GetLocalToWorld();
        const XMMATRIX& GetWorldToLocal();

        void LoadFromJson(const nlohmann::json& objJson) override;

    private:
    
        bool m_dirty = true;

        bool m_hasOddNegativeScale = false;
        
        TransformCompProp<XMVECTOR> m_position = XMVectorZero();
        TransformCompProp<XMVECTOR> m_rotation = XMVectorZero();
        TransformCompProp<XMVECTOR> m_eulerAngles = XMQuaternionIdentity();
        TransformCompProp<XMVECTOR> m_scale = XMVectorReplicate(1.0f);
        TransformCompProp<XMMATRIX> m_matrix = XMMatrixIdentity();

        static void SetDirty(const Object* object);
    };
}
