#pragma once

#include "Forge/Panels/Panel.h"

#include <deque>
#include <memory>
#include <string>

namespace Forge
{
    struct LogEntry
    {
        std::string Message;
        int Level = 0;
    };

    class LogPanel : public Panel
    {
    public:
        explicit LogPanel(std::string name);

        void OnInitialize() override;
        void OnShutdown() override;
        void OnRender() override;

    private:
        std::shared_ptr<std::deque<LogEntry>> m_Entries;
        std::shared_ptr<void> m_Sink;

        bool m_AutoScroll = true;
    };
}