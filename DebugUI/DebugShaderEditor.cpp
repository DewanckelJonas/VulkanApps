#include "DebugShaderEditor.h"
#include <tchar.h>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/DirStackFileIncluder.h>
#include <VulkanWrapper/AppInfo.h>
#include <DataHandling/Helper.h>
bool vkw::ShaderEditor::s_GlslangInitialized = false;

vkw::ShaderEditor::ShaderEditor(const char* filepath)
{
	TextEditor::LanguageDefinition lang = TextEditor::LanguageDefinition::GLSL();

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

	//init glslang should only happen once per process
	if (!s_GlslangInitialized)
	{
		if(!glslang::InitializeProcess())
		{
			assert(0 && "Failed to initialize glslang!");
		}
		s_GlslangInitialized = true;
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
				Save();
			}
			if (ImGui::MenuItem("Compile"))
			{
				Compile();
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
//helper function for Compile function TODO: Move this somewhere better
EShLanguage GetShaderStage(const std::string& extension)
{
	if (extension == "vert") {
		return EShLangVertex;
	}
	else if (extension == "tesc") {
		return EShLangTessControl;
	}
	else if (extension == "tese") {
		return EShLangTessEvaluation;
	}
	else if (extension == "geom") {
		return EShLangGeometry;
	}
	else if (extension == "frag") {
		return EShLangFragment;
	}
	else if (extension == "comp") {
		return EShLangCompute;
	}
	else {
		assert(0 && "Unknown shader stage");
		return EShLangCount;
	}
}
//Shader compiling default values TODO: move this to a better place
const TBuiltInResource DefaultTBuiltInResource = {
	/* .MaxLights = */ 32,
	/* .MaxClipPlanes = */ 6,
	/* .MaxTextureUnits = */ 32,
	/* .MaxTextureCoords = */ 32,
	/* .MaxVertexAttribs = */ 64,
	/* .MaxVertexUniformComponents = */ 4096,
	/* .MaxVaryingFloats = */ 64,
	/* .MaxVertexTextureImageUnits = */ 32,
	/* .MaxCombinedTextureImageUnits = */ 80,
	/* .MaxTextureImageUnits = */ 32,
	/* .MaxFragmentUniformComponents = */ 4096,
	/* .MaxDrawBuffers = */ 32,
	/* .MaxVertexUniformVectors = */ 128,
	/* .MaxVaryingVectors = */ 8,
	/* .MaxFragmentUniformVectors = */ 16,
	/* .MaxVertexOutputVectors = */ 16,
	/* .MaxFragmentInputVectors = */ 15,
	/* .MinProgramTexelOffset = */ -8,
	/* .MaxProgramTexelOffset = */ 7,
	/* .MaxClipDistances = */ 8,
	/* .MaxComputeWorkGroupCountX = */ 65535,
	/* .MaxComputeWorkGroupCountY = */ 65535,
	/* .MaxComputeWorkGroupCountZ = */ 65535,
	/* .MaxComputeWorkGroupSizeX = */ 1024,
	/* .MaxComputeWorkGroupSizeY = */ 1024,
	/* .MaxComputeWorkGroupSizeZ = */ 64,
	/* .MaxComputeUniformComponents = */ 1024,
	/* .MaxComputeTextureImageUnits = */ 16,
	/* .MaxComputeImageUniforms = */ 8,
	/* .MaxComputeAtomicCounters = */ 8,
	/* .MaxComputeAtomicCounterBuffers = */ 1,
	/* .MaxVaryingComponents = */ 60,
	/* .MaxVertexOutputComponents = */ 64,
	/* .MaxGeometryInputComponents = */ 64,
	/* .MaxGeometryOutputComponents = */ 128,
	/* .MaxFragmentInputComponents = */ 128,
	/* .MaxImageUnits = */ 8,
	/* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
	/* .MaxCombinedShaderOutputResources = */ 8,
	/* .MaxImageSamples = */ 0,
	/* .MaxVertexImageUniforms = */ 0,
	/* .MaxTessControlImageUniforms = */ 0,
	/* .MaxTessEvaluationImageUniforms = */ 0,
	/* .MaxGeometryImageUniforms = */ 0,
	/* .MaxFragmentImageUniforms = */ 8,
	/* .MaxCombinedImageUniforms = */ 8,
	/* .MaxGeometryTextureImageUnits = */ 16,
	/* .MaxGeometryOutputVertices = */ 256,
	/* .MaxGeometryTotalOutputComponents = */ 1024,
	/* .MaxGeometryUniformComponents = */ 1024,
	/* .MaxGeometryVaryingComponents = */ 64,
	/* .MaxTessControlInputComponents = */ 128,
	/* .MaxTessControlOutputComponents = */ 128,
	/* .MaxTessControlTextureImageUnits = */ 16,
	/* .MaxTessControlUniformComponents = */ 1024,
	/* .MaxTessControlTotalOutputComponents = */ 4096,
	/* .MaxTessEvaluationInputComponents = */ 128,
	/* .MaxTessEvaluationOutputComponents = */ 128,
	/* .MaxTessEvaluationTextureImageUnits = */ 16,
	/* .MaxTessEvaluationUniformComponents = */ 1024,
	/* .MaxTessPatchComponents = */ 120,
	/* .MaxPatchVertices = */ 32,
	/* .MaxTessGenLevel = */ 64,
	/* .MaxViewports = */ 16,
	/* .MaxVertexAtomicCounters = */ 0,
	/* .MaxTessControlAtomicCounters = */ 0,
	/* .MaxTessEvaluationAtomicCounters = */ 0,
	/* .MaxGeometryAtomicCounters = */ 0,
	/* .MaxFragmentAtomicCounters = */ 8,
	/* .MaxCombinedAtomicCounters = */ 8,
	/* .MaxAtomicCounterBindings = */ 1,
	/* .MaxVertexAtomicCounterBuffers = */ 0,
	/* .MaxTessControlAtomicCounterBuffers = */ 0,
	/* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
	/* .MaxGeometryAtomicCounterBuffers = */ 0,
	/* .MaxFragmentAtomicCounterBuffers = */ 1,
	/* .MaxCombinedAtomicCounterBuffers = */ 1,
	/* .MaxAtomicCounterBufferSize = */ 16384,
	/* .MaxTransformFeedbackBuffers = */ 4,
	/* .MaxTransformFeedbackInterleavedComponents = */ 64,
	/* .MaxCullDistances = */ 8,
	/* .MaxCombinedClipAndCullDistances = */ 8,
	/* .MaxSamples = */ 4,
	/* .maxMeshOutputVerticesNV = */ 256,
	/* .maxMeshOutputPrimitivesNV = */ 512,
	/* .maxMeshWorkGroupSizeX_NV = */ 32,
	/* .maxMeshWorkGroupSizeY_NV = */ 1,
	/* .maxMeshWorkGroupSizeZ_NV = */ 1,
	/* .maxTaskWorkGroupSizeX_NV = */ 32,
	/* .maxTaskWorkGroupSizeY_NV = */ 1,
	/* .maxTaskWorkGroupSizeZ_NV = */ 1,
	/* .maxMeshViewCountNV = */ 4,
	/* .maxDualSourceDrawBuffersEXT = */ 1,

	/* .limits = */ {
	/* .nonInductiveForLoops = */ 1,
	/* .whileLoops = */ 1,
	/* .doWhileLoops = */ 1,
	/* .generalUniformIndexing = */ 1,
	/* .generalAttributeMatrixVectorIndexing = */ 1,
	/* .generalVaryingIndexing = */ 1,
	/* .generalSamplerIndexing = */ 1,
	/* .generalVariableIndexing = */ 1,
	/* .generalConstantMatrixVectorIndexing = */ 1,
} };

void vkw::ShaderEditor::Compile()
{
	Save();
	
	std::string text = m_Editor.GetText();
	const char* compileInput = text.c_str();
	EShLanguage shaderType = GetShaderStage(GetSuffix(GetFileName(std::string(m_FilePath))));
	glslang::TShader shader(shaderType);
	shader.setStrings(&compileInput, 1);

	int ClientInputSemanticsVersion = 450; // maps to, say, #define VULKAN 100
	glslang::EShTargetClientVersion VulkanClientVersion = glslang::EShTargetClientVersion(APP_INFO.apiVersion);
	glslang::EShTargetLanguageVersion TargetVersion = glslang::EShTargetSpv_1_3;

	shader.setEnvInput(glslang::EShSourceGlsl, shaderType, glslang::EShClientVulkan, ClientInputSemanticsVersion);
	shader.setEnvClient(glslang::EShClientVulkan, VulkanClientVersion);
	shader.setEnvTarget(glslang::EShTargetSpv, TargetVersion);

	TBuiltInResource resources = DefaultTBuiltInResource;
	const int defaultVersion = 450;
	std::string PreprocessedGLSL;
	EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

	DirStackFileIncluder Includer;

	//Get path of the directory the file is in.
	std::string dirPath = GetFilePath(m_FilePath);
	Includer.pushExternalLocalDirectory(dirPath);

	std::string preprocessedGLSL;

	if (!shader.preprocess(&resources, defaultVersion, ENoProfile, false, false, messages, &preprocessedGLSL, Includer))
	{
		std::cout << "GLSL Preprocessing Failed for: " << m_FilePath << std::endl;
		std::cout << shader.getInfoLog() << std::endl;
		std::cout << shader.getInfoDebugLog() << std::endl;
	}

	
	const char* preprocessedCStr = preprocessedGLSL.c_str();
	shader.setStrings(&preprocessedCStr, 1);

	if (!shader.parse(&resources, 450, false, messages))
	{
		std::cout << "GLSL Parsing Failed for: " << m_FilePath << std::endl;
		std::cout << shader.getInfoLog() << std::endl;
		std::cout << shader.getInfoDebugLog() << std::endl;
		return;
	}

	glslang::TProgram program;
	program.addShader(&shader);

	if (!program.link(messages))
	{
		std::cout << "GLSL Linking Failed for: " << m_FilePath << std::endl;
		std::cout << shader.getInfoLog() << std::endl;
		std::cout << shader.getInfoDebugLog() << std::endl;
		return;
	}

	std::vector<unsigned int> spirV{};
	spv::SpvBuildLogger logger{};
	glslang::SpvOptions spvOptions{};
	glslang::GlslangToSpv(*program.getIntermediate(shaderType), spirV, &logger, &spvOptions);

	std::ofstream file;
	std::string compiledFilePath = dirPath + "/" + GetFileName(m_FilePath) + ".spv";
	file.open(compiledFilePath, std::ios::binary);
	file.write((char*)&spirV[0], spirV.size()*sizeof(unsigned int));
	file.close();

	return;
}

