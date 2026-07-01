#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Engine)

class CUIManager final : public CEngineBase
{
private:
	CUIManager();
	~CUIManager();

public:
	void UpdateGUI();

	void Load();
	void Find_UiDesc();

	std::vector<UI_DESC> Ui_Desces{};

public:
	static UPtr<CUIManager> Create();
};

NS_END