#pragma once
#include "GameObject.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CComPxSphereCollider;
class CComPxRigidBody;
NS_END

NS_BEGIN(Client)
class CStarBurst : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CStarBurst, CGameObject)

public:
	typedef struct tag_StarBurst_desc : public CGameObject::GAMEOBJECT_DESC
	{
		_float3 vStartPosition{};
		_float3 vEndPosition{};
		_float  fSpeed{ 10.f };
		_float fRadius{ 0.5f };
		PX_FILTER_DESC tFilter{
		.iLayer = ETOUI(COLLISION_LAYER::PLAYER_PROJECTILE),
		.iSimulationMask = PX_ALL_LAYERS,
		.iQueryMask = PX_ALL_LAYERS
		};
	}STARBURST_DESC;

private:
	CStarBurst();
	~CStarBurst() override;

public:
	void UpdateGUI() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

public:
	void OnCollisionEnter(CGameObject* pObj, const PX_ON_COLLISION_DATA& info) override;
	void OnCollisionExit(CGameObject* pObj, const PX_ON_COLLISION_DATA& info) override;
	void OnTriggerEnter(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info) override;
	void OnTriggerExit(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info) override;

private:
	CComPxRigidBody* m_pComPxRigidBody{};
	CComPxSphereCollider* m_pComPxShpereCollider{};

public:
	static E::UPtr<CStarBurst> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
