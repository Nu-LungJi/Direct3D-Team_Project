#pragma once

#include "Engine_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)

class ENGINE_DLL CPrefabManager final : public CEngineBase
{
	private:
	CPrefabManager();
	~CPrefabManager();

private:
	HRESULT Initialize();

public:
	static UPtr<CPrefabManager> Create();

};

NS_END