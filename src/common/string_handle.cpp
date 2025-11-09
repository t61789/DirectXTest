#include "string_handle.h"

namespace dt
{
    StringHandle::StringHandle()
    {
        m_str = "";
        m_hash = std::hash<std::string>{}(m_str);
    }

    StringHandle::StringHandle(const char* str)
    {
        m_str = str;
        m_hash = std::hash<std::string>{}(str);
    }

    StringHandle::StringHandle(const std::string& str)
    {
        m_str = str;
        m_hash = std::hash<std::string>{}(str);
    }

    bool StringHandle::Empty() const
    {
        static StringHandle empty;
        return *this == empty;
    }
}
