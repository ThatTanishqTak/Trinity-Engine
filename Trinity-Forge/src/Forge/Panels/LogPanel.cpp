#include "Forge/Panels/LogPanel.h"

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <imgui.h>

#include <algorithm>
#include <mutex>

namespace Forge
{
    class ImGuiLogSink final : public spdlog::sinks::base_sink<std::mutex>
    {
    public:
        explicit ImGuiLogSink(std::shared_ptr<std::deque<LogEntry>> entries) : m_Entries(std::move(entries))
        {

        }

    protected:
        void sink_it_(const spdlog::details::log_msg& msg) override
        {
            spdlog::memory_buf_t l_MemoryBuffer;
            formatter_->format(msg, l_MemoryBuffer);

            LogEntry l_Entry;
            l_Entry.Message = fmt::to_string(l_MemoryBuffer);
            l_Entry.Level = static_cast<int>(msg.level);

            m_Entries->push_back(std::move(l_Entry));

            constexpr std::size_t l_MaxEntries = 1000;
            if (m_Entries->size() > l_MaxEntries)
            {
                m_Entries->pop_front();
            }
        }

        void flush_() override
        {

        }

    private:
        std::shared_ptr<std::deque<LogEntry>> m_Entries;
    };

    LogPanel::LogPanel(std::string name) : Panel(std::move(name)), m_Entries(std::make_shared<std::deque<LogEntry>>())
    {

    }

    void LogPanel::OnInitialize()
    {
        auto a_Sink = std::make_shared<ImGuiLogSink>(m_Entries);
        a_Sink->set_pattern("[%T] [%n] %v");

        m_Sink = a_Sink;

        auto a_AddSink = [&](const std::string& loggerName)
        {
            auto a_Logger = spdlog::get(loggerName);
            if (a_Logger)
            {
                a_Logger->sinks().push_back(a_Sink);
            }
        };

        a_AddSink("TRINITY-ENGINE");
        a_AddSink("TRINITY-FORGE");
    }

    void LogPanel::OnShutdown()
    {
        auto a_Sink = std::static_pointer_cast<spdlog::sinks::sink>(m_Sink);

        auto a_RemoveSink = [&](const std::string& loggerName)
        {
            auto a_Logger = spdlog::get(loggerName);
            if (!a_Logger)
            {
                return;
            }

            auto& a_Sinks = a_Logger->sinks();
            a_Sinks.erase(std::remove(a_Sinks.begin(), a_Sinks.end(), a_Sink), a_Sinks.end());
        };

        a_RemoveSink("TRINITY-ENGINE");
        a_RemoveSink("TRINITY-FORGE");
        m_Sink.reset();
    }

    void LogPanel::OnRender()
    {
        ImGui::Begin(m_Name.c_str(), &m_Open);

        if (ImGui::Button("Clear"))
        {
            m_Entries->clear();
        }

        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &m_AutoScroll);

        ImGui::Separator();

        ImGui::BeginChild("##LogScroll", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

        for (const auto& l_Entry : *m_Entries)
        {
            ImVec4 l_Color;
            switch (l_Entry.Level)
            {
                case 0:
                    l_Color = ImVec4(0.60f, 0.60f, 0.60f, 1.0f);
                    break;  // trace
                case 1:
                    l_Color = ImVec4(0.70f, 0.70f, 0.70f, 1.0f);
                    break;  // debug
                case 2:
                    l_Color = ImVec4(1.00f, 1.00f, 1.00f, 1.0f);
                    break;  // info
                case 3:
                    l_Color = ImVec4(1.00f, 0.90f, 0.00f, 1.0f);
                    break;  // warn
                case 4:
                    l_Color = ImVec4(1.00f, 0.30f, 0.30f, 1.0f);
                    break;  // error
                case 5:
                    l_Color = ImVec4(1.00f, 0.00f, 0.50f, 1.0f);
                    break;  // critical
                default:
                    l_Color = ImVec4(1.00f, 1.00f, 1.00f, 1.0f);
                    break;
            }

            ImGui::TextColored(l_Color, "%s", l_Entry.Message.c_str());
        }

        if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndChild();
        ImGui::End();
    }
}