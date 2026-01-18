#include "Engine/Layer/LayerStack.h"
#include <algorithm>

namespace Engine
{
    LayerStack::LayerStack() = default;

    LayerStack::~LayerStack()
    {
        for (Layer* it_Layer : m_Layers)
        {
            it_Layer->OnShutdown();
            delete it_Layer;
        }
    }

    void LayerStack::PushLayer(Layer* layer)
    {
        m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
        ++m_LayerInsertIndex;
        layer->OnInitialize();
    }

    void LayerStack::PushOverlay(Layer* overlay)
    {
        m_Layers.push_back(overlay);
        overlay->OnInitialize();
    }

    void LayerStack::PopLayer(Layer* layer)
    {
        auto a_Index = std::find(m_Layers.begin(), m_Layers.end(), layer);
        if (a_Index != m_Layers.end())
        {
            layer->OnShutdown();
            m_Layers.erase(a_Index);
            --m_LayerInsertIndex;
        }
    }

    void LayerStack::PopOverlay(Layer* overlay)
    {
        auto a_Index = std::find(m_Layers.begin(), m_Layers.end(), overlay);
        if (a_Index != m_Layers.end())
        {
            overlay->OnShutdown();
            m_Layers.erase(a_Index);
        }
    }
}