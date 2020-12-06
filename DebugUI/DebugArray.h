#pragma once
//template<class T>
//class DebugArray final : public IDebugUIElement
//{
//
//public:
//	SelectableList(const char* name, std::map<std::string, T>* list) :m_Name{ name }, m_pList{ list }{};
//
//	void Render() override
//	{
//		std::vector<const char*> keys;
//		std::map<std::string, T>* map = (std::map<std::string, T>*)(m_pList);
//		for (auto it = map->begin(); it != map->end(); ++it)
//		{
//			keys.push_back(it->first.c_str());
//		}
//		RenderList(m_Name, m_CurrentItemId, keys);
//	}
//
//	T& GetSelectedItem()
//	{
//		std::vector<std::string> keys;
//		std::map<std::string, T>* map = (std::map<std::string, T>*)(m_pList);
//		for (auto it = map->begin(); it != map->end(); ++it)
//		{
//			keys.push_back(it->first);
//		}
//		return (*m_pList)[keys[m_CurrentItemId]];
//	}
//
//private:
//	std::map<std::string, T>* m_pList = nullptr;
//	const char* m_Name = nullptr;
//	int								m_CurrentItemId = 0;
//};

