#include "data_set.h"

#include "utils.h"

namespace dt
{
    DataSet::~DataSet()
    {
        for (auto& item : m_data)
        {
            delete[] item.data;
        }
    }

    bool DataSet::TrySetImp(const string_hash nameId, const void* data, const uint32_t sizeB)
    {
        auto changed = false;
        
        auto dataInfo = find(m_data, &DataInfo::nameId, nameId);
        if (!dataInfo)
        {
            m_data.push_back({ nameId, 0, nullptr});
            dataInfo = &m_data.back();
            changed |= true;
        }
        
        if (dataInfo->sizeB != sizeB)
        {
            delete[] dataInfo->data;
            dataInfo->data = new uint8_t[sizeB];
            dataInfo->sizeB = sizeB;
            changed |= true;
        }

        if (memcmp(dataInfo->data, data, sizeB) != 0)
        {
            memcpy(dataInfo->data, data, sizeB);
            changed |= true;
        }

        return changed;
    }

    bool DataSet::TryGetImp(const string_hash nameId, void* data, const uint32_t sizeB)
    {
        auto dataInfo = find(m_data, &DataInfo::nameId, nameId);
        if (!dataInfo)
        {
            return false;
        }

        auto clampedSizeB = std::min(sizeB, dataInfo->sizeB);

        memcpy(data, dataInfo->data, clampedSizeB);

        return true;
    }
}
