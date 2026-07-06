#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Engine)
class CGameObject;

class CCreatureEditor final : public CEngineBase
{
public:


private:
	CCreatureEditor();
	~CCreatureEditor();

public:
	HRESULT Initilize();
public:

	void Update(_float fTimeDelta);

public:
	void UpdateGUI();
private:
	CHandle m_hTestModel{};

public:
	static UPtr<CCreatureEditor> Create();
};

NS_END