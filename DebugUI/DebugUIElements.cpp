#include "DebugUIElements.h"
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

void vkw::DebugParameter::Render()
{
	switch (m_VarType)
	{
	case vkw::VarType::Int:
		ImGui::InputInt(m_Name, (int*)(m_Data));
		break;
	case vkw::VarType::Float:
		ImGui::InputFloat(m_Name, (float*)(m_Data));
		break;
	case vkw::VarType::Bool:
		ImGui::Checkbox(m_Name, (bool*)(m_Data));
		break;
	case vkw::VarType::String:
		ImGui::InputText(m_Name, (std::string*)(m_Data));
		break;
	default:
		assert(0 && "Unsupported DebugParameter");
		break;
	}
}

void vkw::DebugStat::Render()
{
	std::string text(m_Name);
	switch (m_VarType)
	{
	case vkw::VarType::Int:
		text += ": %d";
		ImGui::Text(text.c_str(), m_Data);
		break;
	case vkw::VarType::Float:
		text += ": %f";
		ImGui::Text(text.c_str(), (float*)(m_Data));
		break;
	case vkw::VarType::Bool:
		text += ": %b";
		ImGui::Text(text.c_str(), (bool*)(m_Data));
		break;
	case vkw::VarType::String:
		text += ": " + *(std::string*)(m_Data);
		ImGui::Text(text.c_str());
		break;
	default:
		assert(0 && "Unsupported DebugStat");
		break;
	}
}

void vkw::RenderList(const char* name, int& currentItemId, const std::vector<const char*>& keys)
{
	ImGui::Combo(name, &currentItemId, keys.data(), int(keys.size()));
}
