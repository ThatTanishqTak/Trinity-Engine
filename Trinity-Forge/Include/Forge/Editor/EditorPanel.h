#pragma once

namespace Trinity
{
    class Engine;
    struct EditorContext;

    class EditorPanel
    {
    public:
        EditorPanel(EditorContext& context, Engine& engine) : m_Context(context), m_Engine(engine)
        {

        }

        virtual ~EditorPanel() = default;

        EditorPanel(const EditorPanel&) = delete;
        EditorPanel& operator=(const EditorPanel&) = delete;

        virtual void OnImGuiRender() = 0;

    protected:
        EditorContext& m_Context;
        Engine& m_Engine;
    };
}