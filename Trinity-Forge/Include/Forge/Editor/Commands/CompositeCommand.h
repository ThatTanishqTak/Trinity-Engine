#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <Forge/Editor/ICommand.h>

namespace Trinity
{
    // Groups several commands into a single undo step (e.g. one edit applied to a multi-selection).
    class CompositeCommand : public ICommand
    {
    public:
        explicit CompositeCommand(const std::string& label) : m_Label(label)
        {

        }

        void Add(std::unique_ptr<ICommand> command)
        {
            m_Commands.push_back(std::move(command));
        }

        bool Empty() const { return m_Commands.empty(); }

        void Execute() override
        {
            for (auto& it_Command : m_Commands)
            {
                it_Command->Execute();
            }
        }

        void Undo() override
        {
            for (auto it_Command = m_Commands.rbegin(); it_Command != m_Commands.rend(); ++it_Command)
            {
                (*it_Command)->Undo();
            }
        }

        std::string GetName() const override { return m_Label; }

    private:
        std::string m_Label;
        std::vector<std::unique_ptr<ICommand>> m_Commands;
    };
}