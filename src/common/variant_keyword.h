#pragma once
#include "const.h"

namespace dt
{
    struct VariantKeyword
    {
        VariantKeyword() = default;
        ~VariantKeyword() = default;
        VariantKeyword(crvec<str> keywords);
        VariantKeyword(const VariantKeyword& other) = default;
        VariantKeyword(VariantKeyword&& other) noexcept = default;
        VariantKeyword& operator=(const VariantKeyword& other) = default;
        VariantKeyword& operator=(VariantKeyword&& other) noexcept = default;

        friend bool operator==(const VariantKeyword& lhs, const VariantKeyword& rhs);
        friend bool operator!=(const VariantKeyword& lhs, const VariantKeyword& rhs);

        void EnableKeyword(cr<StringHandle> keyword);
        void DisableKeyword(cr<StringHandle> keyword);
        size_t GetHash() const { return m_hash; }
        crvec<StringHandle> GetKeywords() const { return m_keywords; }
        crvec<std::wstring> GetKeywordsWStr() const { return m_keywordsWStr; }
        str GetStr() const;
        
    private:
        size_t m_hash = 0;
        vec<StringHandle> m_keywords;
        vec<std::wstring> m_keywordsWStr;
    };
    
}
