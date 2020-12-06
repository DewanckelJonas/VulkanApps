#pragma once
#include <functional>
#include "DebugUIElements.h"
namespace vkw
{
	class Button final : public IDebugUIElement
	{
	public:
		Button(const char* name, const std::function<void()>& callback);
		void Render() override;

	private:
		std::function<void()>			m_Callback;
		const char*						m_Name = nullptr;
	};
}

