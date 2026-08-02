#pragma once
#include "Mon_Weapon.h"
#include "Client_Defines.h"
NS_BEGIN(Client)
class CGurdianWeapon final : public CMon_Weapon
{
public:
	DECLARE_DERIVED_TYPE(CGurdianWeapon, CMon_Weapon)
private:
	CGurdianWeapon();
	~CGurdianWeapon() override;

public:
	void UpdateGUI() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;

public:
	void OnTriggerEnter(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info) override;
public:
	_bool					Weapon_CallBack() { return m_bDissolve; }
private:
	void					Weapon_Throw(_float fTimeDelta);
public:
	static E::UPtr<CGurdianWeapon> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
