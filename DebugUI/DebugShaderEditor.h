#pragma once
#include "DebugUIElements.h"
#include "ImGuiColorTextEdit/TextEditor.h"
namespace vkw
{
	class ShaderEditor final : public IDebugUIElement
	{
	public:
		ShaderEditor(const char* filepath);
		void Render() override;
		void Save();
		void Compile();
	private:
		TextEditor m_Editor;
		const char* m_FilePath = nullptr;
	};
}

