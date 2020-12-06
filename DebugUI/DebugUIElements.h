#pragma once
#include <string>
#include <map>
#include <vector>
namespace vkw
{
	class IDebugUIElement
	{
	public:
		virtual void Render() = 0;
	};

	enum class VarType
	{
		Int,
		Float,
		Bool,
		String,
	};

	class DebugParameter final : public IDebugUIElement
	{

	public:

		//MACRO to automatically fill in variable name
		#define UI_CREATEPARAMETER(Variable) new vkw::DebugParameter{#Variable, &Variable}
		DebugParameter(const char* name, int* value) :m_VarType{ VarType::Int }, m_Name{ name }, m_Data{ value }{};
		DebugParameter(const char* name, float* value) :m_VarType{ VarType::Float }, m_Name{ name }, m_Data{ value }{};
		DebugParameter(const char* name, bool* value) :m_VarType{ VarType::Bool }, m_Name{ name }, m_Data{ value }{};
		DebugParameter(const char* name, std::string* value) :m_VarType{ VarType::String }, m_Name{ name }, m_Data{ value }{};

		void Render() override;

	private:

		void*			m_Data = nullptr;
		const char*		m_Name = nullptr;
		VarType			m_VarType = VarType::Int;
	};


	class DebugStat final : public IDebugUIElement
	{

	public:

		//MACRO to automatically fill in variable name
		#define UI_CREATESTAT(Variable) new vkw::DebugParameter{#Variable, &Variable}
		DebugStat(const char* name, const int* value) :m_VarType{ VarType::Int }, m_Name{ name }, m_Data{ value }{};
		DebugStat(const char* name, const float* value) :m_VarType{ VarType::Float }, m_Name{ name }, m_Data{ value }{};
		DebugStat(const char* name, const bool* value) :m_VarType{ VarType::Bool }, m_Name{ name }, m_Data{ value }{};
		DebugStat(const char* name, const std::string* value) :m_VarType{ VarType::String }, m_Name{ name }, m_Data{ value }{};

		void Render() override;

	private:
		const void*		m_Data = nullptr;
		const char*		m_Name = nullptr;
		VarType			m_VarType = VarType::Int;
	};

	//To prevent including imgui in the header file;
	void RenderList(const char* name, int& currentItemId, const std::vector<const char*>& keys);
	template<class T>
	class SelectableList final : public IDebugUIElement
	{

	public:
		SelectableList(const char* name, std::map<std::string, T>* list) :m_Name{name}, m_pList{ list }{};
		
		void Render() override 
		{
			std::vector<const char*> keys;
			std::map<std::string, T>* map = (std::map<std::string, T>*)(m_pList);
			for (auto it = map->begin(); it != map->end(); ++it)
			{
				keys.push_back(it->first.c_str());
			}
			RenderList(m_Name, m_CurrentItemId, keys);
		}

		T& GetSelectedItem()
		{
			std::vector<std::string> keys;
			std::map<std::string, T>* map = (std::map<std::string, T>*)(m_pList);
			for (auto it = map->begin(); it != map->end(); ++it)
			{
				keys.push_back(it->first);
			}
			return (*m_pList)[keys[m_CurrentItemId]];
		}

	private:
		std::map<std::string, T>*		m_pList = nullptr;
		const char*						m_Name = nullptr;
		int								m_CurrentItemId = 0;
	};

}