#pragma once

#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)
class CComPxConvexCollider;
class CComPxRigidBody;
class CComSound;
class CComStaticModelInstance;
class CComConstantBuffer;
class CResPhysXConvexGeometry;
class CResPhysXMaterial;
class CResPixelShader;
class CResVertexShader;
NS_END

NS_BEGIN(Client)

class CPropBarrel final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CPropBarrel, CGameObject)

	enum class BARREL_STATE : uint8_t
	{
		CREATED,
		DESTROYED
	};

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		StringID sResourceGroup{};
		_float3 vInitialPosition{};
		_float3 vInitialRotation{};
		_float3 vInitialScale{ 3.f, 3.f, 3.f };
		_float3 vConvexScale{ 3.f, 3.f, 3.f };
		_float3 vInitialImpulse{};
		_float3 vInitialAngularVelocityRadians{};
		_float fAngularDamping{ 0.05f };
		_float fMass{ 0.15f };
		_float fCollisionDestroyImpulse{ 2.25f };
		_float fCollisionDestroyGraceTime{ 0.3f };
		PX_FILTER_DESC tFilter{
			.iLayer = ETOUI(COLLISION_LAYER::WORLD_DYNAMIC),
			.iSimulationMask =
				ETOUI(COLLISION_LAYER::WORLD_STATIC) |
				ETOUI(COLLISION_LAYER::MOVING_PLATFORM) |
				ETOUI(COLLISION_LAYER::PLAYER_BODY) |
				ETOUI(COLLISION_LAYER::ENEMY_BODY) |
				ETOUI(COLLISION_LAYER::WORLD_DYNAMIC),
			.iQueryMask = PX_ALL_LAYERS,
			.iNotifyFlags =
				PX_NOTIFY_TOUCH_FOUND |
				PX_NOTIFY_CONTACT_POINTS
		};
	};

private:
	CPropBarrel();
	CPropBarrel(const CPropBarrel& prototype);
	~CPropBarrel() override = default;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void FixedUpdate(_float fTimeDelta) override;
	void Update(_float fTimeDelta) override;
	void LateUpdate(_float fTimeDelta) override;
	HRESULT Render_Instanced(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx, const MODEL_INSTANCE_BATCH& batch) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;
	HRESULT Render_Shadow(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;
	bool GetShadowBounds(BoundingBox& outBounds) const override;
	void UpdateGUI() override;
	void OnCollisionEnter(
		CGameObject* pObj,
		const PX_ON_COLLISION_DATA& info) override;

	// 완성된 통을 현재 자세 그대로 파편으로 교체한다.
	_bool DestroyBarrel();

	// AncientThrow가 소유하는 동안 배럴을 키네마틱으로 이동/발사한다.
	_bool BeginAncientThrowControl();
	_float3 GetVisualCenterPosition() const;
	_bool SetAncientThrowVisualPose(
		const _float3& vCenterPosition,
		const _float4& vQuaternion);
	void CancelAncientThrowControl();
	_bool Launch(
		const _float3& vLinearVelocity,
		const _float3& vAngularVelocityRadians);
	BARREL_STATE GetBarrelState() const { return m_eState; }

public:
	static UPtr<CPropBarrel> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	// AncientThrow 충돌 처리와 단계별 사운드 수명 관리
	void HandleAncientThrowImpact(
		CGameObject* pHitObject,
		const _float3& vImpactPosition,
		const _float3& vImpactNormal);
	void PlayAncientThrowPullSounds();
	void PlayAncientThrowLaunchSounds();
	void PlayAncientThrowImpactSounds(const _float3& vImpactPosition) const;
	void PlayBarrelCollisionSound(
		const _float3& vImpactPosition,
		_float fImpulse) const;
	void PlayBarrelDestroySounds(const _float3& vPosition) const;
	void UpdateAncientThrowSoundPosition(
		const _float3& vPosition,
		const _float3& vVelocity = {});
	void StopAncientThrowHoldSound();
	void StopAncientThrowFlightSound();

	// 공유 리소스와 오브젝트 소유 컴포넌트
	SPtr<CResPhysXConvexGeometry> m_pResConvexGeometry{};
	SPtr<CResPhysXMaterial> m_pResPhysXMaterial{};
	CComStaticModelInstance* m_pComModelInstance{};
	CComConstantBuffer* m_pComCBufferPerObject{};
	CComPxRigidBody* m_pComPxRigidBody{};
	CComPxConvexCollider* m_pComPxConvexCollider{};
	CComSound* m_pComSound{};
	SPtr<CResVertexShader> m_pResVertexShader{};
	SPtr<CResPixelShader> m_pResPixelShader{};

	// 배럴 외형/물리 파괴 설정 및 런타임 상태
	StringID m_sResourceGroup{};
	_float3 m_vModelScale{ 1.f, 1.f, 1.f };
	_float3 m_vDebrisConvexScale{ 1.f, 1.f, 1.f };
	_float3 m_vVisualCenterLocalOffset{};
	_float m_fAncientThrowSweepRadius{ 0.5f };
	_float m_fCollisionDestroyImpulse{ 2.25f };
	_float m_fCollisionDestroyGraceTime{ 0.3f };
	_float m_fCollisionDestroyElapsed{};
	_float m_fCollisionSoundCooldown{};
	BARREL_STATE m_eState{ BARREL_STATE::CREATED };
	_bool m_bDestroyRequested{};
	_bool m_bDestroyOriginalNextFrame{};

	// AncientThrow 수동 제어 및 충돌 파괴 상태
	_bool m_bAncientThrowControlled{};
	_bool m_bAncientThrowFlying{};
	_float3 m_vAncientThrowKinematicLinearVelocity{};
	_float3 m_vAncientThrowKinematicAngularVelocity{};
	_bool m_bDestroyFromAncientThrow{};
	_bool m_bHasDestroyImpactPosition{};
	_float3 m_vDestroyImpactPosition{};
	static constexpr _float ANCIENT_DEBRIS_INHERITED_VELOCITY_RATIO = 0.01f;
};

NS_END
