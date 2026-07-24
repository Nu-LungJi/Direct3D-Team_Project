#pragma once
#include "Monster.h"
#include "Client_Defines.h"

NS_BEGIN(Client)
class CBossTMB final : public CMonster
{
public:
	DECLARE_DERIVED_TYPE(CBossTMB, CMonster)

private:
	CBossTMB();
	~CBossTMB() override;

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
public:
	static E::UPtr<CBossTMB> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
