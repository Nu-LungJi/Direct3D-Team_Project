#pragma once
#include "GameObject.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CComPxSphereCollider;
class CComPxRigidBody;
NS_END

NS_BEGIN(Client)
class CBoss_StarBurst : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CBoss_StarBurst, CGameObject)

public:
	typedef struct tag_StarBurst_desc : public CGameObject::GAMEOBJECT_DESC
	{
		_float3 vStartPosition{};
		_float3 vEndPosition{};
		_float  fSpeed{ 10.f };
		_float fRadius{ 0.5f };
		CHandle	pTargetHandle{};
		PX_FILTER_DESC tFilter{
		.iLayer = ETOUI(COLLISION_LAYER::PLAYER_PROJECTILE),
		.iSimulationMask = PX_ALL_LAYERS,
		.iQueryMask = PX_ALL_LAYERS
		};
	}STARBURST_DESC;

private:
	CBoss_StarBurst();
	~CBoss_StarBurst() override;

public:
	void UpdateGUI() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

	void	Translate_Casting(_float _Ratio);
	void	Translate_Attacking(_float _Ratio);

public:
	void OnCollisionEnter(CGameObject* pObj, const PX_ON_COLLISION_DATA& info) override;
	void OnCollisionExit(CGameObject* pObj, const PX_ON_COLLISION_DATA& info) override;
	void OnTriggerEnter(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info) override;
	void OnTriggerExit(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info) override;

private:
	CComPxRigidBody* m_pComPxRigidBody{};
	CComPxSphereCollider* m_pComPxShpereCollider{};

	_float m_fPrevEffectMovementValue{};
	_float m_fCurrEffectMovementValue{};
	_float	m_fEffectLifeTime{};
	_float	m_fEffectSpawnTimer{};

	CHandle	m_pTargetHandle{};

	EFFECT_INSTANCE_ID	m_pLightEffectID{};

public:
	static E::UPtr<CBoss_StarBurst> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
