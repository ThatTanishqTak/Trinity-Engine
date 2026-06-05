#include <Editor/CommandHistory.h>

namespace Trinity
{
    void CommandHistory::Execute(std::unique_ptr<ICommand> command, bool allowMerge)
    {
        if (command == nullptr)
        {
            return;
        }

        command->Execute();

        if (allowMerge && !m_UndoStack.empty() && m_UndoStack.back()->MergeWith(*command))
        {
            m_RedoStack.clear();
            m_Dirty = true;

            return;
        }

        m_UndoStack.push_back(std::move(command));
        m_RedoStack.clear();
        m_Dirty = true;
    }

    void CommandHistory::Undo()
    {
        if (m_UndoStack.empty())
        {
            return;
        }

        std::unique_ptr<ICommand> l_Command = std::move(m_UndoStack.back());
        m_UndoStack.pop_back();

        l_Command->Undo();

        m_RedoStack.push_back(std::move(l_Command));
        m_Dirty = true;
    }

    void CommandHistory::Redo()
    {
        if (m_RedoStack.empty())
        {
            return;
        }

        std::unique_ptr<ICommand> l_Command = std::move(m_RedoStack.back());
        m_RedoStack.pop_back();

        l_Command->Execute();

        m_UndoStack.push_back(std::move(l_Command));
        m_Dirty = true;
    }

    std::string CommandHistory::UndoName() const
    {
        return m_UndoStack.empty() ? std::string() : m_UndoStack.back()->GetName();
    }

    std::string CommandHistory::RedoName() const
    {
        return m_RedoStack.empty() ? std::string() : m_RedoStack.back()->GetName();
    }

    void CommandHistory::Clear()
    {
        m_UndoStack.clear();
        m_RedoStack.clear();
        m_Dirty = false;
    }
}