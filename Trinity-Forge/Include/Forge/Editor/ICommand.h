#pragma once

#include <string>

namespace Trinity
{
    class ICommand
    {
    public:
        virtual ~ICommand() = default;

        virtual void Execute() = 0;
        virtual void Undo() = 0;
        virtual std::string GetName() const = 0;

        virtual bool MergeWith(const ICommand& other)
        {
            (void)other;

            return false;
        }
    };
}