#pragma once

#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
class CComPxRigidBody;
class CComPxSphereCollider;
class CComStaticModelInstance;
class CResPhysXMaterial;
class CResPhysXSphereGeometry;
class CResPixelShader;
class CResVertexShader;
NS_END

NS_BEGIN(Client)

class CAccioBall final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CAccioBall, CGameObject)

	enum class COLOR
	{
		NONE,
		BLUE,
		RED,
		END
	};

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		StringID sResourceGroup{};
		StringID sModelResourceTag{};
		COLOR eColor{ COLOR::NONE };
		_float3 vInitialPosition{};
		_float3 vInitialRotation{};
		_float3 vInitialScale{ 1.f, 1.f, 1.f };
		_float fSphereRadius{ 0.5f };
		_float fMass{ 5.f };
		_float fStaticFriction{ 0.6f };
		_float fDynamicFriction{ 0.45f };
		_float fRestitution{ 0.35f };
		_float fLinearDamping{ 0.6f };
		_float fAngularDamping{ 0.7f };
		_float fAutoSleepLinearSpeed{ 0.15f };
		_float fAutoSleepAngularSpeed{ 0.2f };
		_float fAutoSleepDelay{ 0.3f };
		_float fRollingTorque{ 35.f };
		_float fMaxRollAngularSpeed{ 6.f };
		_float fMaxPullAcceleration{ 32.f };
		_float fMaxPullLinearSpeed{ 20.f };
		_float fPullSlowRadius{ 4.f };
		PX_FILTER_DESC tFilter{
			.iLayer = ETOUI(COLLISION_LAYER::WORLD_DYNAMIC),
			.iSimulationMask =
				ETOUI(COLLISION_LAYER::WORLD_STATIC) |
				ETOUI(COLLISION_LAYER::WORLD_DYNAMIC) |
				ETOUI(COLLISION_LAYER::PLAYER_BODY) |
				ETOUI(COLLISION_LAYER::ENEMY_BODY),
			.iQueryMask = PX_ALL_LAYERS
		};
	};

private:
	CAccioBall();
	CAccioBall(const CAccioBall& prototype);
	~CAccioBall() override = default;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void FixedUpdate(_float fTimeDelta) override;
	void LateUpdate(_float fTimeDelta) override;
	HRESULT Render_Instanced(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx, const MODEL_INSTANCE_BATCH& batch) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;
	void OnWake() override;
	void OnSleep() override;

	_bool ApplyImpulse(const _float3& vImpulse);
	_bool ApplyTorque(const _float3& vTorque);
	_bool ApplyRollingTorque(const _float3& vTorqueAxis);
	_bool ApplyPullMotion(const _float3& vToTarget);
	_bool TryAcquireControl(const CHandle& hController);
	_bool ReleaseControl(const CHandle& hController);
	_bool IsControlledBy(const CHandle& hController) const;
	void SetActivityHandle(const CHandle& hActivity) { m_hActivity = hActivity; }
	_bool SetMotionTuning(
		_float fMass,
		_float fLinearDamping,
		_float fAngularDamping);
	void SetRollingTuning(
		_float fRollingTorque,
		_float fMaxRollAngularSpeed);
	void SetPullTuning(
		_float fMaxPullAcceleration,
		_float fMaxPullLinearSpeed,
		_float fPullSlowRadius);
	_bool ResetToInitialPose();
	CComPxRigidBody* GetRigidBody() const { return m_pComPxRigidBody; }
	_float GetSphereRadius() const { return m_fSphereRadius; }
	COLOR GetBallColor() const { return m_eColor; }
	_bool IsSettled() const { return m_bSettled; }
	CHandle GetControllerHandle() const { return m_hController; }

public:
	static UPtr<CAccioBall> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	void SyncRenderPoseFromRigidBody();

private:
	CComStaticModelInstance* m_pComModelInstance{};
	CComConstantBuffer* m_pComCBufferPerObject{};
	CComPxRigidBody* m_pComPxRigidBody{};
	CComPxSphereCollider* m_pComPxSphereCollider{};
	SPtr<CResPhysXMaterial> m_pResPhysXMaterial{};
	SPtr<CResPhysXSphereGeometry> m_pResSphereGeometry{};
	SPtr<CResVertexShader> m_pResVertexShader{};
	SPtr<CResPixelShader> m_pResPixelShader{};
	_float m_fSphereRadius{ 0.5f };
	_float m_fRollingTorque{ 35.f };
	_float m_fMaxRollAngularSpeed{ 6.f };
	_float m_fMaxPullAcceleration{ 32.f };
	_float m_fMaxPullLinearSpeed{ 20.f };
	_float m_fPullSlowRadius{ 4.f };
	_float m_fAutoSleepLinearSpeed{ 0.15f };
	_float m_fAutoSleepAngularSpeed{ 0.2f };
	_float m_fAutoSleepDelay{ 0.3f };
	_float m_fAutoSleepElapsed{};
	_bool m_bSettled{};
	CHandle m_hController{};
	CHandle m_hActivity{};
	_float3 m_vInitialPosition{};
	_float4 m_vInitialRotation{ 0.f, 0.f, 0.f, 1.f };
	COLOR m_eColor{ COLOR::NONE };
};

NS_END
