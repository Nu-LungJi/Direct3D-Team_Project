#pragma once
#include "GameObject.h"
#include "Client_Defines.h"
#include "ComCollider.h"

NS_BEGIN(Engine)
class CComPxRigidBody;
class CComPxBoxCollider;
class CResPhysXSphereGeometry;
class CResPhysXMaterial;
NS_END
NS_BEGIN(Client)

class CTestPhysXBox final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CTestPhysXBox, CGameObject)

public:
	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		_float3 vInitialPos{};
		_float3 vInitialVelocity{};
		_float fPlayerCollisionDelay{ -1.f };
		PX_FILTER_DESC tFilter{
			.iLayer = ETOUI(COLLISION_LAYER::PLAYER_BODY),
			.iSimulationMask = PX_ALL_LAYERS,
			.iQueryMask = PX_ALL_LAYERS,
			.iNotifyFlags = PX_NOTIFY_ALL
		};
	};

private:
	CTestPhysXBox();
	~CTestPhysXBox() override;

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
	_float m_fPlayerCollisionDelay{ -1.f };

public:
	static E::UPtr<CTestPhysXBox> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
