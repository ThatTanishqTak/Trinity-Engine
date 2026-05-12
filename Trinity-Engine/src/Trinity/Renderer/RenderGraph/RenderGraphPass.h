#pragma once

#include "Trinity/Renderer/RenderGraph/RenderGraphContext.h"
#include "Trinity/Renderer/RenderGraph/RenderGraphResource.h"

#include <array>
#include <functional>
#include <string>
#include <vector>

namespace Trinity
{
    class RenderGraph;

    class RenderGraphPass
    {
    public:
        RenderGraphPass() = default;
        explicit RenderGraphPass(std::string name);

        RenderGraphPass& SetType(RenderGraphPassType type);
        RenderGraphPass& SetEnabled(bool enabled);
        RenderGraphPass& SetCullable(bool cullable);
        RenderGraphPass& SetDebugColor(float r, float g, float b, float a);

        RenderGraphPass& Read(RenderGraphResourceHandle resource);
        RenderGraphPass& Read(RenderGraphResourceHandle resource, RenderGraphAccess access);
        RenderGraphPass& Write(RenderGraphResourceHandle resource);
        RenderGraphPass& Write(RenderGraphResourceHandle resource, RenderGraphAccess access);
        RenderGraphPass& ReadWrite(RenderGraphResourceHandle resource, RenderGraphAccess access);

        RenderGraphPass& SetExecuteCallback(std::function<void(RenderGraphContext&)> callback);

        const std::string& GetName() const { return m_Name; }
        RenderGraphPassType GetType() const { return m_Type; }

        bool IsEnabled() const { return m_Enabled; }
        bool IsCullable() const { return m_Cullable; }
        bool HasExecuteCallback() const { return static_cast<bool>(m_ExecuteCallback); }

        const std::vector<RenderGraphResourceUsage>& GetReads() const { return m_Reads; }
        const std::vector<RenderGraphResourceUsage>& GetWrites() const { return m_Writes; }
        const std::vector<RenderGraphResourceUsage>& GetReadWrites() const { return m_ReadWrites; }

        const std::array<float, 4>& GetDebugColor() const { return m_DebugColor; }

        void Execute(RenderGraphContext& context) const;

    private:
        static bool ContainsUsage(const std::vector<RenderGraphResourceUsage>& usages, RenderGraphResourceHandle resource, RenderGraphAccess access);

    private:
        std::string m_Name;

        RenderGraphPassType m_Type = RenderGraphPassType::Graphics;

        bool m_Enabled = true;
        bool m_Cullable = true;

        std::vector<RenderGraphResourceUsage> m_Reads;
        std::vector<RenderGraphResourceUsage> m_Writes;
        std::vector<RenderGraphResourceUsage> m_ReadWrites;

        std::function<void(RenderGraphContext&)> m_ExecuteCallback;

        std::array<float, 4> m_DebugColor = { 0.25f, 0.45f, 0.95f, 1.0f };
    };
}