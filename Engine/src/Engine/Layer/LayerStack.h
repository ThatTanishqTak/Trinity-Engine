#pragma once

#include "Engine/Layer/Layer.h"

#include <memory>
#include <vector>

namespace Engine
{
    class LayerStack
    {
    public:
        LayerStack();
        ~LayerStack();

        void PushLayer(std::unique_ptr<Layer> layer);
        void PushOverlay(std::unique_ptr<Layer> overlay);

        void PopLayer(Layer* layer);
        void PopOverlay(Layer* overlay);

        void Shutdown();

        std::vector<std::unique_ptr<Layer>>::iterator begin() { return m_Layers.begin(); }
        std::vector<std::unique_ptr<Layer>>::iterator end() { return m_Layers.end(); }
        std::vector<std::unique_ptr<Layer>>::reverse_iterator rbegin() { return m_Layers.rbegin(); }
        std::vector<std::unique_ptr<Layer>>::reverse_iterator rend() { return m_Layers.rend(); }

        std::vector<std::unique_ptr<Layer>>::const_iterator begin() const { return m_Layers.begin(); }
        std::vector<std::unique_ptr<Layer>>::const_iterator end() const { return m_Layers.end(); }
        std::vector<std::unique_ptr<Layer>>::const_reverse_iterator rbegin() const { return m_Layers.rbegin(); }
        std::vector<std::unique_ptr<Layer>>::const_reverse_iterator rend() const { return m_Layers.rend(); }

    private:
        std::vector<std::unique_ptr<Layer>> m_Layers;
        size_t m_LayerInsertIndex = 0;
    };
}