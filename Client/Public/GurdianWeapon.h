#pragma once
#include "Mon_Weapon.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CComPxRigidBody;
class CComPxConvexCollider;
NS_END

NS_BEGIN(Client)
class CGurdianWeapon final : public CMon_Weapon
{
public:
	DECLARE_DERIVED_TYPE(CGurdianWeapon, CMon_Weapon)

public:
	struct DESC : public CMon_Weapon::WEAPON_DESC
	{
		// [LSY] 소켓 Parent World에 포함될 몬스터 스케일을 Convex 크기에 반영한다.
		_float3 vOwnerScale{ 1.f, 1.f, 1.f };
		_float fMass{ 3.f };
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
	};

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
	_bool ActivateDebrisPhysics();
	_bool IsDebrisPhysicsActivated() const { return m_bDebrisPhysicsActivated; }

public:
	_bool					Weapon_CallBack() { return m_bDissolve; }

private:
	HRESULT				InitializeDebrisPhysics(const DESC& Desc);
	_bool				UpdateSocketParentMatrix();
	static const char*	ResolveConvexPath(std::string_view sWeaponName);
	void					Weapon_Throw(_float fTimeDelta);

private:
	CComPxRigidBody*		m_pComPxRigidBody{};
	CComPxConvexCollider*	m_pComPxConvexCollider{};
	_bool					m_bDebrisPhysicsActivated{};

public:
	static E::UPtr<CGurdianWeapon> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
