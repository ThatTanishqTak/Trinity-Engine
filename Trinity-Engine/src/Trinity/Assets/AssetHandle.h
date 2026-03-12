#pragma once

#include <cstdint>
#include <functional>

namespace Trinity
{
    using AssetUUID = uint64_t;

    static constexpr AssetUUID NullAssetUUID = 0;

    template<typename T>
    class AssetHandle
    {
    public:
        AssetHandle() = default;
        explicit AssetHandle(AssetUUID uuid) : m_UUID(uuid) {}

        AssetUUID GetUUID() const { return m_UUID; }

        bool IsValid() const { return m_UUID != NullAssetUUID; }
        explicit operator bool() const { return IsValid(); }

        bool operator==(const AssetHandle<T>& other) const { return m_UUID == other.m_UUID; }
        bool operator!=(const AssetHandle<T>& other) const { return m_UUID != other.m_UUID; }

    private:
        AssetUUID m_UUID = NullAssetUUID;
    };
}

namespace std
{
    template<typename T>
    struct hash<Trinity::AssetHandle<T>>
    {
        size_t operator()(const Trinity::AssetHandle<T>& handle) const
        {
            return hash<Trinity::AssetUUID>{}(handle.GetUUID());
        }
    };
}