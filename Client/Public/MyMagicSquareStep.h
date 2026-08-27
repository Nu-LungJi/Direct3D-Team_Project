#pragma once

#include "GameObject.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
class CComPxBoxCollider;
class CComPxRigidBody;
class CComStaticModelInstance;
class CResPhysXBoxGeometry;
class CResPhysXMaterial;
NS_END

NS_BEGIN(Client)
class CMyMagicSquareStep final : public CGameObject
{
public:
	enum class STATE
	{
		IDLE,
		MOVE,
		BOUNCE_RISE,
		BOUNCE_SETTLE
	};
public:
	DECLARE_DERIVED_TYPE(CMyMagicSquareStep, CGameObject)

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		StringID ResMajorTag{};
		StringID ResMinorTag{};
		_float3 vInitialPosition{};
		_float3 vInitialRotation{};
		_float3 vInitialScale{ 1.f, 1.f, 1.f };
		//_bool bEnablePhysics{ true };
		PX_FILTER_DESC tFilter{
			.iLayer = ETOUI(COLLISION_LAYER::MOVING_PLATFORM),
			.iSimulationMask =
				ETOUI(COLLISION_LAYER::WORLD_DYNAMIC) |
				ETOUI(COLLISION_LAYER::PLAYER_BODY) |
				ETOUI(COLLISION_LAYER::ENEMY_BODY) |
				ETOUI(COLLISION_LAYER::NPC_BODY) |
				ETOUI(COLLISION_LAYER::DEBRIS),
			.iQueryMask = PX_ALL_LAYERS
		};
	};

public:
	static constexpr _float3 MAGIC_STEP_BOX_HALF_EXTENTS{ 0.5031f, 1.5048f, 0.5031f };
	static constexpr _float3 MAGIC_STEP_BOX_LOCAL_OFFSET{ 0.f, -1.4953f, 0.f };
private:
	CMyMagicSquareStep();
	CMyMagicSquareStep(const CMyMagicSquareStep& rhs);
	~CMyMagicSquareStep() override = default;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void FixedUpdate(_float fTimeDelta) override;
	void LateUpdate(_float fTimeDelta) override;
	HRESULT Render_Instanced(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx, const MODEL_INSTANCE_BATCH& batch) override;
	HRESULT Render(
		ID3D11DeviceContext* pContext,
		const RENDER_CTX& ctx) override;
	HRESULT Render_Shadow(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;
	bool GetShadowBounds(BoundingBox& outBounds) const override;
	void UpdateGUI() override;

private:
	SPtr<CResPhysXBoxGeometry> m_pResBoxGeometry{};
	SPtr<CResPhysXMaterial> m_pResPhysXMaterial{};

private:
	CComStaticModelInstance* m_pComModelInstance{};
	CComConstantBuffer* m_pComCBufferPerObject{};
	CComPxRigidBody* m_pComPxRigidBody{};
	CComPxBoxCollider* m_pComPxBoxCollider{};

public:
	void SetMoveTarget(_fvector vMoveTarget);
	void SetBounceMoveTarget(
		_fvector vFinalTarget,
		_float fRiseSpeed,
		_float fBounceHeight,
		_float fSettleSpeed);
	void SetKinematicPosition(
		const _float3& vPosition);
	_fvector GetMoveTarget() const { return XMLoadFloat3(&m_vMoveTarget); };
private:
	_float3 m_vMoveTarget{};
	_float3 m_vFinalMoveTarget{};
	_float m_fBounceSettleSpeed{ 1.f };

public:
	STATE GetState() const { return m_eState; }
private:
	STATE m_eState{ STATE::IDLE };

public:
	void SetSpeed(_float fSpeed) { m_fSpeed = fSpeed; }
	_float GetSpeed() const { return m_fSpeed; }
private:
	_float m_fSpeed{ 1.f };

public:
	static UPtr<CMyMagicSquareStep> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
