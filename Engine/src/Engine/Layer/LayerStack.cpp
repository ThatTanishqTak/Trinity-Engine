#include "Engine/Layer/LayerStack.h"

#include <algorithm>
#include <utility>

namespace Engine
{
    LayerStack::LayerStack() = default;

    LayerStack::~LayerStack()
    {
        Shutdown();
    }

    void LayerStack::Shutdown()
    {
        for (const std::unique_ptr<Layer>& it_Layer : m_Layers)
        {
            it_Layer->OnShutdown();
        }

        m_Layers.clear();
        m_LayerInsertIndex = 0;
    }

    void LayerStack::PushLayer(std::unique_ptr<Layer> layer)
    {
        Layer* l_Layer = layer.get();
        m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, std::move(layer));
        ++m_LayerInsertIndex;
        l_Layer->OnInitialize();
    }

    void LayerStack::PushOverlay(std::unique_ptr<Layer> overlay)
    {
        Layer* l_Overlay = overlay.get();
        m_Layers.push_back(std::move(overlay));
        l_Overlay->OnInitialize();
    }

    void LayerStack::PopLayer(Layer* layer)
    {
        auto a_Index = std::find_if(m_Layers.begin(), m_Layers.end(), [layer](const std::unique_ptr<Layer>& it_Layer) { return it_Layer.get() == layer; });
        if (a_Index != m_Layers.end())
        {
            (*a_Index)->OnShutdown();
            m_Layers.erase(a_Index);
            --m_LayerInsertIndex;
        }
    }

    void LayerStack::PopOverlay(Layer* overlay)
    {
        auto a_Index = std::find_if(m_Layers.begin(), m_Layers.end(), [overlay](const std::unique_ptr<Layer>& it_Layer) { return it_Layer.get() == overlay; });
        if (a_Index != m_Layers.end())
        {
            (*a_Index)->OnShutdown();
            m_Layers.erase(a_Index);
        }
    }
}