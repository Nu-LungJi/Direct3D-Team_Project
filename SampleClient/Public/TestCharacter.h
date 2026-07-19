#pragma once
#include "GameObject.h"
#include "Client_Defines.h"
#include "ComCollider.h"

NS_BEGIN(Engine)
class CComPxRigidBody;
class CComPxBoxCollider;
class CResPhysXBoxGeometry;
class CComPxCharacterController;
class CComLocomotion;
class CComCharacterMotor;
NS_END
NS_BEGIN(Client)

class CTestCharacter final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CTestCharacter, CGameObject)

public:
	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		PX_FILTER_DESC tFilter{
			.iLayer = ETOUI(COLLISION_LAYER::PLAYER_BODY),
			.iSimulationMask = PX_ALL_LAYERS,
			.iQueryMask = PX_ALL_LAYERS
		};
	};

private:
	CTestCharacter();
	~CTestCharacter() override;

public:
	HRESULT Initialize(void* pArg) override;
	void FixedUpdate(_float fTimeDelta) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

public:
	void OnWake() override;
	void OnSleep() override;
	void OnCollisionEnter(CGameObject* pObj, const PX_ON_COLLISION_DATA& info) override;
	void OnCollisionExit(CGameObject* pObj, const PX_ON_COLLISION_DATA& info) override;
	void OnTriggerEnter(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info) override;
	void OnTriggerExit(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info) override;

private:
	struct PROJECTILE_LIFETIME
	{
		CHandle hProjectile{};
		_float fRemainingTime{};
	};

	CComPxRigidBody* m_pComPxRigidBody{};
	CComPxBoxCollider* m_pComPxBoxCollider{};
	CComCollider* m_pComCollider{};
	CComPxCharacterController* m_pComCharacterController{};
	CComLocomotion* m_pComLocomotion{};
	CComCharacterMotor* m_pComCharacterMotor{};
	std::vector<PROJECTILE_LIFETIME> m_Projectiles{};

public:
	static E::UPtr<CTestCharacter> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
