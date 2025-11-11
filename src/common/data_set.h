#pragma once
#include <stdexcept>

#include "const.h"
#include "utils.h"

namespace dt
{
    class DataSet
    {
        struct DataInfo
        {
            string_hash nameId = 0;
            uint32_t sizeB = 0;
            uint8_t* data = nullptr;
        };
        
    public:
        DataSet() = default;
        ~DataSet();
        DataSet(const DataSet& other) = delete;
        DataSet(DataSet&& other) noexcept = delete;
        DataSet& operator=(const DataSet& other) = delete;
        DataSet& operator=(DataSet&& other) noexcept = delete;
        
        bool Exists(string_hash nameId);
        bool Remove(string_hash nameId);

        bool TrySetImp(string_hash nameId, const void* data, uint32_t sizeB);
        bool TryGetImp(string_hash nameId, void* data, uint32_t sizeB);

        crvec<DataInfo> GetAllData() const { return m_data; }

    private:

        vec<DataInfo> m_data;
    };

    inline bool DataSet::Exists(const string_hash nameId)
    {
        return find(m_data, &DataInfo::nameId, nameId) != nullptr;
    }

    inline bool DataSet::Remove(string_hash nameId)
    {
        auto it = std::find_if(m_data.begin(), m_data.end(), [nameId](cr<DataInfo> info){ return info.nameId == nameId;});
        if (it == m_data.end())
        {
            return false;
        }

        delete[] it->data;
        m_data.erase(it);

        return true;
    }

}
