#pragma once

#include <memory>
#include <string>
#include <vector>

#include <Forge/Editor/ICommand.h>

namespace Trinity
{
    class CommandHistory
    {
    public:
        void Execute(std::unique_ptr<ICommand> command, bool allowMerge = false);
        void Undo();
        void Redo();

        bool CanUndo() const { return !m_UndoStack.empty(); }
        bool CanRedo() const { return !m_RedoStack.empty(); }

        std::string UndoName() const;
        std::string RedoName() const;

        void Clear();

        bool IsDirty() const { return m_Dirty; }
        void MarkSaved() { m_Dirty = false; }

    private:
        std::vector<std::unique_ptr<ICommand>> m_UndoStack;
        std::vector<std::unique_ptr<ICommand>> m_RedoStack;
        bool m_Dirty = false;
    };
}