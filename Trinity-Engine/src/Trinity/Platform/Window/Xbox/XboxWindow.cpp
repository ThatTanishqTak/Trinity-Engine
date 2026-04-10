#include "Trinity/Platform/Window/Xbox/XboxWindow.h"

#include "Trinity/Events/Event.h"

namespace Trinity
{
	XboxWindow::XboxWindow()
	{

	}

	XboxWindow::~XboxWindow()
	{

	}

	void XboxWindow::Initialize(const WindowProperties& properties)
	{
		m_Width = properties.Width;
		m_Height = properties.Height;
		m_VSync = properties.VSync;

		m_Minimized = (m_Width == 0 || m_Height == 0);
		
		m_ShouldClose = false;
		m_CursorVisible = true;
		m_CursorLocked = false;
		m_Initialized = true;
	}

	void XboxWindow::Shutdown()
	{
		m_Initialized = false;
		m_ShouldClose = false;
		m_Minimized = false;
	}

	void XboxWindow::OnUpdate()
	{
		if (!m_Initialized)
		{
			return;
		}
	}

	void XboxWindow::OnEvent(Event&)
	{
	}

	void XboxWindow::SetVSync(bool enabled)
	{
		m_VSync = enabled;
	}

	void XboxWindow::SetCursorVisible(bool visible)
	{
		m_CursorVisible = visible;
	}

	void XboxWindow::SetCursorLocked(bool locked)
	{
		m_CursorLocked = locked;
	}

	NativeWindowHandle XboxWindow::GetNativeHandle() const
	{
		return {};
	}
}