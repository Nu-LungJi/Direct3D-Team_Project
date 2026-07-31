#pragma once
#include "GameObject.h"
#include "Client_Defines.h"

NS_BEGIN(Engine) 
class CComPxSphereCollider;
class CComPxRigidBody;
NS_END


NS_BEGIN(Client)

class CPlayer_Magic_Bullet : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_Magic_Bullet, CGameObject)

public:
	typedef struct tag_Magic_Bullet_desc : public CGameObject::GAMEOBJECT_DESC
	{
		_float3 vStartPosition{};
		_float3 vEndPosition{};
		_float  fSpeed{ 10.f };
		_float  fCurveHeight{ 3.f };
		uint32_t iSampleCount{ 48 };
		_float fRadius{ 0.5f };
		PX_FILTER_DESC tFilter{
		.iLayer = ETOUI(COLLISION_LAYER::PLAYER_PROJECTILE),
		.iSimulationMask = PX_ALL_LAYERS,
		.iQueryMask = PX_ALL_LAYERS
			};
	}MAGIC_BULLET_DESC;

private:
	CPlayer_Magic_Bullet();
	~CPlayer_Magic_Bullet() override;

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
	_float3 m_vStartPosition{};
	_float3 m_vEndPosition{};
	_float  m_fSpeed{};
	_float  m_fRadius{};
	_float  m_fDistanceOnSegment{};
	size_t  m_iSplineIndex{};

	std::vector<_float3> m_Splines;

private:
	void BuildSpline(_float fCurveHeight, uint32_t iSampleCount);


private:
	CComPxRigidBody* m_pComPxRigidBody{};
	CComPxSphereCollider* m_pComPxShpereCollider{};
public:
	static E::UPtr<CPlayer_Magic_Bullet> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
