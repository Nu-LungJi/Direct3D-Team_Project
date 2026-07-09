#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)

class CSerializeManager final : public CEngineBase
{
private:
	CSerializeManager();
	~CSerializeManager();

public:
	void UpdateGUI();

public:
	HRESULT Initialize();

public:
	static UPtr<CSerializeManager> Create();
};

NS_END
