#pragma once

#include "GameObject.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CComPxRigidBody;
class CComPxBoxCollider;
NS_END

NS_BEGIN(Client)

class CTestPhysXTrigger final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CTestPhysXTrigger, CGameObject)

public:
	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		_float3 vInitialPos{};
		_float3 vHalfExtents{ 1.f, 1.f, 1.f };
		_bool bIsTrigger{ true };
		PX_FILTER_DESC tFilter{
			.iLayer = ETOUI(COLLISION_LAYER::INTERACTION),
			.iSimulationMask = PX_ALL_LAYERS,
			.iQueryMask = PX_ALL_LAYERS
		};
	};

private:
	CTestPhysXTrigger();
	~CTestPhysXTrigger() override;

public:
	HRESULT Initialize(void* pArg) override;
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
	CComPxRigidBody* m_pComPxRigidBody{};
	CComPxBoxCollider* m_pComPxBoxCollider{};

public:
	static E::UPtr<CTestPhysXTrigger> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
