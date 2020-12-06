#pragma once
#include <string>
#include <map>
#include <vector>
namespace vkw
{
	class IDebugUIElement;
	class DebugWindow final
	{
	public:
		DebugWindow(const std::string& name = "DebugWindow");
		~DebugWindow();
		void Render() const;
		//This function takes ownership over DebugElementPointer passed.
		void AddUIElement(IDebugUIElement* variable, const std::string& group = "");
	private:
		void Cleanup();

		std::string m_Name;
		std::map<std::string, std::vector<IDebugUIElement*>> m_UIElements;
	};
}

