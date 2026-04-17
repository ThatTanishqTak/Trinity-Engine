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
            spdlog::memory_buf_t l_Buf;
            formatter_->format(msg, l_Buf);

            LogEntry l_Entry;
            l_Entry.Message = fmt::to_string(l_Buf);
            l_Entry.Level = static_cast<int>(msg.level);

            m_Entries->push_back(std::move(l_Entry));

            constexpr std::size_t k_MaxEntries = 1000;
            if (m_Entries->size() > k_MaxEntries)
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

    // -------------------------------------------------------------------------

    LogPanel::LogPanel(std::string name) : Panel(std::move(name)), m_Entries(std::make_shared<std::deque<LogEntry>>())
    {

    }

    void LogPanel::OnInitialize()
    {
        auto l_Sink = std::make_shared<ImGuiLogSink>(m_Entries);
        l_Sink->set_pattern("[%T] [%n] %v");
        m_Sink = l_Sink;

        auto l_AddSink = [&](const std::string& loggerName)
        {
            auto l_Logger = spdlog::get(loggerName);
            if (l_Logger)
            {
                l_Logger->sinks().push_back(l_Sink);
            }
        };

        l_AddSink("TRINITY-ENGINE");
        l_AddSink("TRINITY-FORGE");
    }

    void LogPanel::OnShutdown()
    {
        auto l_Sink = std::static_pointer_cast<spdlog::sinks::sink>(m_Sink);

        auto l_RemoveSink = [&](const std::string& loggerName)
        {
            auto l_Logger = spdlog::get(loggerName);
            if (!l_Logger)
            {
                return;
            }

            auto& l_Sinks = l_Logger->sinks();
            l_Sinks.erase(std::remove(l_Sinks.begin(), l_Sinks.end(), l_Sink), l_Sinks.end());
        };

        l_RemoveSink("TRINITY-ENGINE");
        l_RemoveSink("TRINITY-FORGE");
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