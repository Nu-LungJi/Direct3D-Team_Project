#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Engine)
class CGameObject;

class CCreatureManager final : public CEngineBase
{//제작 예정
public:


private:
	CCreatureManager();
	~CCreatureManager();

public:
	HRESULT Initilize();
public:

	void Update(_float fTimeDelta);
	void CurrentLevelType(const _string PrototypesName, const _string ResoureseName);
public:
	void UpdateGUI();
private:
	CHandle m_hTestModel{};

	_string m_PrototypeNames{}, m_ResourcesName{};
public:
	static UPtr<CCreatureManager> Create();
};

NS_END
