#pragma once
#include "Monster.h"
#include "Client_Defines.h"

NS_BEGIN(Client)
class CTmbGudian final : public CMonster
{
public:
	struct TMB_DESC :public  CMonster::MONSTER_DESC
	{

	};
public:
	DECLARE_DERIVED_TYPE(CTmbGudian, CMonster)

private:
	CTmbGudian();
	~CTmbGudian() override;

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
	std::vector				m_DeadMeshes;


public:
	static E::UPtr<CTmbGudian> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
