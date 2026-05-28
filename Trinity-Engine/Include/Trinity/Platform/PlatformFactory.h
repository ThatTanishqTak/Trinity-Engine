#pragma once

#include <memory>

#include <Trinity/Platform/IPlatform.h>
#include <Trinity/Platform/PlatformTypes.h>

namespace Trinity
{
    class PlatformFactory
    {
    public:
        static std::unique_ptr<IPlatform> Create();
        static std::unique_ptr<IPlatform> Create(PlatformType type);
    };
}