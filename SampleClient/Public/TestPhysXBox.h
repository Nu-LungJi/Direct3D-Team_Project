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
	void OnCollisionEnter(CGameObject* pObj, const PHYSIX_ON_COLLISION_DATA& info) override;
	void OnCollisionExit(CGameObject* pObj, const PHYSIX_ON_COLLISION_DATA& info) override;
	void OnTriggerEnter(CGameObject* pObj, const PHYSIX_ON_TRIGGER_DATA& info) override;
	void OnTriggerExit(CGameObject* pObj, const PHYSIX_ON_TRIGGER_DATA& info) override;

private:
	CComPxRigidBody* m_pComPxRigidBody{};
	CComPxBoxCollider* m_pComPxBoxCollider{};

public:
	static E::UPtr<CTestPhysXBox> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
