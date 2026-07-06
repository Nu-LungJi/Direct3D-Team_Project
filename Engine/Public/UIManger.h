#pragma once

#include "Engine_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)

class ENGINE_DLL CUIManager final : public CEngineBase
{
private:
	CUIManager();
	~CUIManager();

private:
	HRESULT Initialize();

public:
	static UPtr<CUIManager> Create();
};

NS_END