#pragma once
#include "WorldAgent.h"
#include "Client_Defines.h"

NS_BEGIN(Client)
class CWorldAnimal : public CWorldAgent
{
public:
	DECLARE_DERIVED_TYPE(CWorldAnimal, CWorldAgent)

private:
	CWorldAnimal();
	~CWorldAnimal() override;

public:
	void UpdateGUI() override;
public:
	HRESULT						InitializePrototype(void* pArg = nullptr) override;
	HRESULT						Initialize(void* pArg) override;
	void						PriorityUpdate(E::_float fTimeDelta) override;
	void						FixedUpdate(E::_float fTimeDelta) override;
	void						Update(E::_float fTimeDelta) override;
	void						LateUpdate(E::_float fTimeDelta) override;
public:
	void						Set_Gravity(_bool bGravity);
public:
	static E::UPtr<CWorldAnimal> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
