#pragma once
#include "Monster.h"
#include "Client_Defines.h"

NS_BEGIN(Client)
class CTestGob final : public CMonster
{
public:
	DECLARE_DERIVED_TYPE(CTestGob, CMonster)

private:
	CTestGob();
	~CTestGob() override;

public:
	void UpdateGUI() override;
public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void FixedUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;

private:
	_bool bShow{ false };	
public:
	static E::UPtr<CTestGob> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
