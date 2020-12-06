#include "Button.h"
#include "imgui.h"

vkw::Button::Button(const char* name, const std::function<void()>& callback)
	:m_Name{name}
	,m_Callback{callback}
{
}

void vkw::Button::Render()
{
	if (ImGui::Button(m_Name))
	{
		m_Callback();
	}
}
