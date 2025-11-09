#pragma once
#include "const.h"
#include "string_handle.h"

namespace dt
{
    class IResource
    {
    public:
        IResource() = default;
        virtual ~IResource() = default;
        IResource(const IResource& other) = delete;
        IResource(IResource&& other) noexcept = delete;
        IResource& operator=(const IResource& other) = delete;
        IResource& operator=(IResource&& other) noexcept = delete;

        virtual cr<StringHandle> GetPath() = 0;
    };
}
