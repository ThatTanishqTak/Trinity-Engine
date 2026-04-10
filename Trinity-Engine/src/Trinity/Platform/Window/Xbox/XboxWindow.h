#pragma once

#include "Trinity/Events/EventQueue.h"
#include "Trinity/Platform/Window/Window.h"

namespace Trinity
{
	class XboxWindow final : public Window
	{
	public:
		XboxWindow();
		~XboxWindow() override;

		void Initialize(const WindowProperties& properties = WindowProperties()) override;
		void Shutdown() override;

		void OnUpdate() override;
		void OnEvent(Event& e) override;

		EventQueue& GetEventQueue() override { return m_EventQueue; }

		uint32_t GetWidth() const override { return m_Width; }
		uint32_t GetHeight() const override { return m_Height; }

		void SetVSync(bool enabled) override;
		bool IsVSync() const override { return m_VSync; }

		void SetCursorVisible(bool visible) override;
		void SetCursorLocked(bool locked) override;
		bool IsCursorVisible() const override { return m_CursorVisible; }
		bool IsCursorLocked() const override { return m_CursorLocked; }

		bool ShouldClose() const override { return m_ShouldClose; }
		bool IsMinimized() const override { return m_Minimized; }

		NativeWindowHandle GetNativeHandle() const override;

	private:
		EventQueue m_EventQueue{};
		
		uint32_t m_Width = 0;
		uint32_t m_Height = 0;

		bool m_VSync = true;
		bool m_CursorVisible = true;
		bool m_CursorLocked = false;
		bool m_ShouldClose = false;
		bool m_Minimized = false;
		bool m_Initialized = false;
	};
}