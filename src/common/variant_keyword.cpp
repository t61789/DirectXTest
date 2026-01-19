#include "variant_keyword.h"

#include "utils.h"

namespace dt
{
    VariantKeyword::VariantKeyword(crvec<str> keywords)
    {
        for (auto& keyword : keywords)
        {
            EnableKeyword(keyword);
        }
    }

    void VariantKeyword::EnableKeyword(cr<StringHandle> keyword)
    {
        assert(keyword.Hash() != 0);

        Utils::CombineHashNoOrder(m_hash, keyword.Hash());
        m_keywords.push_back(keyword);

        m_keywordsWStr.push_back(Utils::StringToWString(keyword.Str()));
    }

    void VariantKeyword::DisableKeyword(cr<StringHandle> keyword)
    {
        assert(keyword.Hash() != 0);
        
        remove_if(m_keywords, [&keyword](cr<StringHandle> k){ return keyword.Hash() == k.Hash(); });

        m_hash = 0;
        for (auto& k : m_keywords)
        {
            Utils::CombineHashNoOrder(m_hash, k.Hash());
        }

        auto keywordWStr = Utils::StringToWString(keyword);
        remove(m_keywordsWStr, keywordWStr);
    }

    str VariantKeyword::GetStr() const
    {
        if (m_keywords.empty())
        {
            return "NONE";
        }
        
        vec<str> keywords;
        for (auto& k : m_keywords)
        {
            keywords.push_back(k.Str());
        }

        return Utils::Join(keywords, " ");
    }

    bool operator==(const VariantKeyword& lhs, const VariantKeyword& rhs)
    {
        return lhs.m_hash == rhs.m_hash;
    }

    bool operator!=(const VariantKeyword& lhs, const VariantKeyword& rhs)
    {
        return !(lhs == rhs);
    }
}
