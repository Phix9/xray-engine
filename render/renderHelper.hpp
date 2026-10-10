#pragma once

#include <memory>

namespace XrayEngine
{
    inline uint32_t RoundUp(uint32_t value, uint32_t alignment)
    {
        uint32_t temp = value + alignment - static_cast<uint32_t>(1);
        return (temp - temp % alignment);
    }
}