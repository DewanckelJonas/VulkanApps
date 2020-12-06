#include "DebugWindow.h"
#include <imgui.h>
#include "DebugUIElements.h"

vkw::DebugWindow::DebugWindow(const std::string& name)
	:m_Name{ name }
{
}

vkw::DebugWindow::~DebugWindow()
{
	Cleanup();
}

void vkw::DebugWindow::Render() const
{
	ImGui::Begin(m_Name.c_str(), nullptr, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar);
	for (const std::pair<std::string, std::vector<IDebugUIElement*>>& uiElements : m_UIElements)
	{
		if(uiElements.first == "") //no collapsing header
		{
			for (IDebugUIElement* uiElement : uiElements.second)
			{
				uiElement->Render();
			}
		}
		else 
		if (ImGui::CollapsingHeader(uiElements.first.c_str()))
		{
			for (IDebugUIElement* uiElement : uiElements.second)
			{
				uiElement->Render();
			}
		}
	}
	ImGui::End();
}

void vkw::DebugWindow::AddUIElement(IDebugUIElement* variable, const std::string& group)
{
	m_UIElements[group].push_back(variable);
}

void vkw::DebugWindow::Cleanup()
{
	for (const std::pair<std::string, std::vector<IDebugUIElement*>>& uiElements : m_UIElements)
	{
		for (IDebugUIElement* uiElement : uiElements.second)
		{
			delete uiElement;
		}
	}
}