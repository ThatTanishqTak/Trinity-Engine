#pragma once

#include <cstdint>
#include <functional>

namespace Trinity
{
    class UUID
    {
    public:
        UUID();
        UUID(uint64_t uuid);
        UUID(const UUID&) = default;

        operator uint64_t() const { return m_UUID; }

    private:
        uint64_t m_UUID;
    };
}

namespace std
{
    template<>
    struct hash<Trinity::UUID>
    {
        std::size_t operator()(const Trinity::UUID& uuid) const noexcept { return hash<uint64_t>()(static_cast<uint64_t>(uuid)); }
    };
}