#include "DebugShaderEditor.h"
#include <tchar.h>
#include <fstream>
#include <streambuf>

vkw::ShaderEditor::ShaderEditor(const char* filepath)
{
	auto lang = TextEditor::LanguageDefinition::GLSL();

	// set your own known preprocessor symbols...
	//static const char* ppnames[] = {};
	// ... and their corresponding values
	//static const char* ppvalues[] = {};

	//for (int i = 0; i < sizeof(ppnames) / sizeof(ppnames[0]); ++i)
	//{
	//	TextEditor::Identifier id;
	//	id.mDeclaration = ppvalues[i];
	//	lang.mPreprocIdentifiers.insert(std::make_pair(std::string(ppnames[i]), id));
	//}

	// set your own identifiers
	//static const char* identifiers[] = { };
	//static const char* idecls[] ={ };
	//for (int i = 0; i < sizeof(identifiers) / sizeof(identifiers[0]); ++i)
	//{
	//	TextEditor::Identifier id;
	//	id.mDeclaration = std::string(idecls[i]);
	//	lang.mIdentifiers.insert(std::make_pair(std::string(identifiers[i]), id));
	//}
	m_Editor.SetLanguageDefinition(lang);
	//editor.SetPalette(TextEditor::GetLightPalette());

	// error markers
	//TextEditor::ErrorMarkers markers;
	//markers.insert(std::make_pair<int, std::string>(6, "Example error here:\nInclude file not found: \"TextEditor.h\""));
	//markers.insert(std::make_pair<int, std::string>(41, "Another example error"));
	//editor.SetErrorMarkers(markers);

	// "breakpoint" markers
	//TextEditor::Breakpoints bpts;
	//bpts.insert(24);
	//bpts.insert(47);
	//editor.SetBreakpoints(bpts);

	m_FilePath = filepath;

	{
		std::ifstream t(m_FilePath);
		if (t.good())
		{
			std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
			m_Editor.SetText(str);
		}
	}
}

void vkw::ShaderEditor::Render()
{
	auto cpos = m_Editor.GetCursorPosition();
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Save"))
			{
				std::string textToSave = m_Editor.GetText();
				Save();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit"))
		{
			bool ro = m_Editor.IsReadOnly();
			if (ImGui::MenuItem("Read-only mode", nullptr, &ro))
				m_Editor.SetReadOnly(ro);
			ImGui::Separator();

			if (ImGui::MenuItem("Undo", "ALT-Backspace", nullptr, !ro && m_Editor.CanUndo()))
				m_Editor.Undo();
			if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, !ro && m_Editor.CanRedo()))
				m_Editor.Redo();

			ImGui::Separator();

			if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, m_Editor.HasSelection()))
				m_Editor.Copy();
			if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr, !ro && m_Editor.HasSelection()))
				m_Editor.Cut();
			if (ImGui::MenuItem("Delete", "Del", nullptr, !ro && m_Editor.HasSelection()))
				m_Editor.Delete();
			if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr, !ro && ImGui::GetClipboardText() != nullptr))
				m_Editor.Paste();

			ImGui::Separator();

			if (ImGui::MenuItem("Select all", nullptr, nullptr))
				m_Editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(m_Editor.GetTotalLines(), 0));

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::MenuItem("Dark palette"))
				m_Editor.SetPalette(TextEditor::GetDarkPalette());
			if (ImGui::MenuItem("Light palette"))
				m_Editor.SetPalette(TextEditor::GetLightPalette());
			if (ImGui::MenuItem("Retro blue palette"))
				m_Editor.SetPalette(TextEditor::GetRetroBluePalette());
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, m_Editor.GetTotalLines(),
		m_Editor.IsOverwrite() ? "Ovr" : "Ins",
		m_Editor.CanUndo() ? "*" : " ",
		m_Editor.GetLanguageDefinition().mName.c_str(), m_FilePath);

	m_Editor.Render("TextEditor");
}

void vkw::ShaderEditor::Save()
{
	std::ofstream file;
	file.open(m_FilePath);
	file << m_Editor.GetText();
	file.close();
	return;
}

void vkw::ShaderEditor::Compile()
{
}
