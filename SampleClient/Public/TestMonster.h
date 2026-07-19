#pragma once
#include "GameObject.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CComPxCharacterController;
class CComLocomotion;
class CComCharacterMotor;
NS_END

NS_BEGIN(Client)

class CTestMonster final : public CGameObject
{
public:
	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		_float3 vInitialPos{};
		CHandle hTarget{};
		PX_FILTER_DESC tFilter{
			.iLayer = ETOUI(COLLISION_LAYER::ENEMY_BODY),
			.iSimulationMask = PX_ALL_LAYERS,
			.iQueryMask = PX_ALL_LAYERS
		};
	};

public:
	DECLARE_DERIVED_TYPE(CTestMonster, CGameObject)

private:
	CTestMonster();
	~CTestMonster() override;

public:
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(_float fTimeDelta) override;
	void FixedUpdate(_float fTimeDelta) override;
	void LateUpdate(_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;

public:
	void OnCollisionEnter(CGameObject* pObj, const PX_ON_COLLISION_DATA& info) override;
	void OnCollisionExit(CGameObject* pObj, const PX_ON_COLLISION_DATA& info) override;
	void OnCCTShapeHit(const PX_CCT_HIT_DATA& tHit) override;

private:
	void TryDestroyByProjectile(CGameObject* pObject);
	void TryFireAtTarget(_float fTimeDelta);

	CComLocomotion* m_pLocomotion{};
	CComCharacterMotor* m_pCharacterMotor{};
	CComPxCharacterController* m_pCharacterController{};
	CHandle m_hTarget{};
	_float m_fDirectionChangeTimer{};
	_float m_fAttackCooldown{};
	_float3 m_vMoveDirection{ 0.f, 0.f, 1.f };

public:
	static UPtr<CTestMonster> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
