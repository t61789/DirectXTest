#pragma once
#include <typeindex>

#include "common/const.h"
#include "nlohmann/json.hpp"


namespace dt
{
    class Scene;
    class Object;
    class GameFramework;

    class Comp
    {
        friend class Object;
        friend class GameFramework;
        friend class SceneRegistry;
        friend class Scene;
        friend class CompStorage;
        
    public:
        Object* GetOwner() const { return m_owner; }
        cr<StringHandle> GetName() const { return m_name; }
        std::type_index GetType() const { return m_type; }
        
        void SetEnable(bool enable);
        bool IsEnable() const { return m_realEnable; }

        virtual void Awake(){}
        virtual void Start(){}
        virtual void Update(){}
        virtual void OnDestroy(){}
        virtual void OnEnable(){}
        virtual void OnDisable(){}

        Comp() = default;
        virtual ~Comp() = default;
        Comp(const Comp& other) = delete;
        Comp(Comp&& other) noexcept = delete;
        Comp& operator=(const Comp& other) = delete;
        Comp& operator=(Comp&& other) noexcept = delete;

        virtual void LoadFromJson(cr<nlohmann::json> objJson);

        static sp<Comp> Create(cr<StringHandle> compName);

    private:
        bool m_enable = true;
        bool m_realEnable = false;
        bool m_hasAwake = false;
        bool m_hasStart = false;
        StringHandle m_name;
        Object* m_owner = nullptr;
        std::type_index m_type = std::type_index(typeid(Comp));

        void CallUpdate();
        void Destroy();
        void UpdateRealEnable();

        friend class Game;
    };
}
