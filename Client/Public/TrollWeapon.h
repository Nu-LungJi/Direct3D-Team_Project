#pragma once
#include "Mon_Weapon.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CComPxRigidBody;
class CComPxConvexCollider;
NS_END

NS_BEGIN(Client)
class CTrollWeapon final : public CMon_Weapon
{
public:
	DECLARE_DERIVED_TYPE(CTrollWeapon, CMon_Weapon)

public:
	typedef struct strTrollweapon : public CMon_Weapon::WEAPON_DESC
	{
		_float3 vOwnerScale{ 1.f, 1.f, 1.f };
		PX_FILTER_DESC tFilter{
			.iLayer = ETOUI(COLLISION_LAYER::DEBRIS),
			.iSimulationMask =
				ETOUI(COLLISION_LAYER::WORLD_STATIC) |
				ETOUI(COLLISION_LAYER::WORLD_DYNAMIC) |
				ETOUI(COLLISION_LAYER::MOVING_PLATFORM),
			.iQueryMask =
				ETOUI(COLLISION_LAYER::WORLD_STATIC) |
				ETOUI(COLLISION_LAYER::WORLD_DYNAMIC) |
				ETOUI(COLLISION_LAYER::MOVING_PLATFORM)
		};
	}TROLL_WEAPON_DESC;

private:
	CTrollWeapon();
	~CTrollWeapon() override;

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
	void Set_Dead() { m_bDead = true; }
public:
	_bool				Weapon_CallBack() { return m_bDissolve; }

private:
	void				Dead_Parent(_float fTimeDelta);
	_bool				UpdateSocketParentMatrix();

private:
	CComPxRigidBody* m_pComPxRigidBody{};
	CComPxConvexCollider* m_pComPxConvexCollider{};

public:
	static E::UPtr<CTrollWeapon> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
