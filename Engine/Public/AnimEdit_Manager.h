
#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Engine)
class CGameObject;

class CAnimEdit_Manager final : public CEngineBase
{
private:
	CAnimEdit_Manager();
	~CAnimEdit_Manager();

public:
	HRESULT Initilize();
public:

	void Update(_float fTimeDelta);

	void SetTestModelHandle(const CHandle& handle) { m_hTestModel = handle; }

private:
	CGameObject* m_pTestModel{ nullptr };
	CHandle m_hTestModel{};

public:
	static UPtr<CAnimEdit_Manager> Create();
};

NS_END