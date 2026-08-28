#pragma once

#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
class CComPxBoxCollider;
class CComPxD6Joint;
class CComPxRigidBody;
class CComStaticModelInstance;
class CResPixelShader;
class CResVertexShader;
NS_END

NS_BEGIN(Client)

class CPhysicsDoor final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CPhysicsDoor, CGameObject)

	enum class HINGE_SIDE : uint8_t
	{
		LEFT,
		RIGHT
	};

	enum class SHAPE_SUB_INDEX : uint32_t
	{
		DOOR_LEAF,
		HINGE_BLOCKER,
		PASSAGE_BARRIER,
		PASSAGE_TRIGGER
	};

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		StringID sModelResourceGroup{ LEVEL::TERRAIN };
		StringID sModelResourceTag{ "Static_PhysicsDoor_Resource" };
		_float3 vInitialPosition{};
		_float3 vInitialRotation{};
		_float3 vInitialScale{ 1.f, 1.f, 1.f };
		_float3 vHalfExtents{ 1.875f, 3.735f, 0.354f };
		_float fMass{ 30.f };
		_float fAngularDamping{ 5.f };
		_float fTwistDriveStiffness{ 300.f };
		_float fTwistDriveDamping{ 80.f };
		_float fTwistDriveForceLimit{ 1500.f };
		_float fLowerLimitDegrees{ -110.f };
		_float fUpperLimitDegrees{ 110.f };
		_float fCCTPushForce{ 500.f };
		_float fHingeBlockerHalfWidth{ 0.45f };
		_float fHingeBlockerDepthPadding{ 0.3f };
		_float fPassageBarrierHalfDepth{ 0.12f };
		_float fPassageOpenAngleDegrees{ 35.f };
		_float3 vPassageTriggerHalfExtents{ 1.875f, 3.5f, 1.f };
		HINGE_SIDE eHingeSide{ HINGE_SIDE::LEFT };
		PX_FILTER_DESC tDoorFilter{
			.iLayer = ETOUI(COLLISION_LAYER::DOOR_DYNAMIC),
			// [LSY] CCT는 Query로 문에 막히고 OnCCTShapeHit에서 제한된 토크를
			// 전달한다. 직접 Simulation 접촉은 D6 문을 요동시킬 수 있어 제외한다.
			.iSimulationMask =
				PX_ALL_LAYERS &
				~ETOUI(COLLISION_LAYER::PLAYER_BODY) &
				~ETOUI(COLLISION_LAYER::WORLD_STATIC),
			.iQueryMask = PX_ALL_LAYERS
		};
		PX_FILTER_DESC tCCTBlockerFilter{
			.iLayer = ETOUI(COLLISION_LAYER::DOOR_HINGE_BLOCKER),
			// [LSY] 힌지와 출입구 고정 가드는 CCT Move Query만 차단한다.
			// Simulation Pair를 만들지 않아 문짝과 물리력을 주고받지 않는다.
			.iSimulationMask = 0u,
			.iQueryMask = PX_ALL_LAYERS
		};
		PX_FILTER_DESC tPassageTriggerFilter{
			.iLayer = ETOUI(COLLISION_LAYER::TRIGGER),
			.iSimulationMask = ETOUI(COLLISION_LAYER::PLAYER_HURTBOX),
			.iQueryMask = 0u,
			.iNotifyFlags = PX_NOTIFY_TOUCH_FOUND | PX_NOTIFY_TOUCH_LOST
		};
	};

private:
	CPhysicsDoor();
	CPhysicsDoor(const CPhysicsDoor& prototype);
	~CPhysicsDoor() override = default;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void LateUpdate(_float fTimeDelta) override;
	HRESULT Render_Instanced(
		ID3D11DeviceContext* pContext,
		const RENDER_CTX& ctx,
		const MODEL_INSTANCE_BATCH& batch) override;
	HRESULT Render(
		ID3D11DeviceContext* pContext,
		const RENDER_CTX& ctx) override;
	HRESULT Render_Shadow(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;
	bool GetShadowBounds(BoundingBox& outBounds) const override;
	void UpdateGUI() override;
	void OnTriggerEnter(
		CGameObject* pGameObject,
		const PX_ON_TRIGGER_DATA& tData) override;
	void OnTriggerExit(
		CGameObject* pGameObject,
		const PX_ON_TRIGGER_DATA& tData) override;

	_bool ApplyOpeningTorque(_float fTorque);
	// [LSY] CCT Hit의 접촉점과 이동 방향을 힌지 축 토크로 변환한다.
	_bool ApplyCCTPush(const PX_CCT_HIT_DATA& tHit);
	// [LSY] 문짝, 월드 힌지 앵커와 출입구용 고정 Actor를 함께 옮긴다.
	_bool SetPlacement(
		const _float3& vPosition,
		const _float3& vRotationEulerDegrees,
		const _float3& vScale);
	_bool ResetDoor();
	_bool IsHingeReady() const { return m_pComPxD6Joint != nullptr; }
	_float GetOpeningAngleDegrees() const;

protected:
	void OnRegisteredToManager() override;

private:
	_float3 CalculateHingeWorldPosition(
		const _float3& vDoorPosition,
		const _float4& vDoorRotation) const;
	_bool IsPlayerPassageTriggerEvent(
		CGameObject* pGameObject,
		const PX_ON_TRIGGER_DATA& tData) const;
	_bool CreateHingeBlocker(const DESC& desc);
	_bool CreatePassageBarrier(const DESC& desc);
	_bool CreatePassageTrigger(const DESC& desc);
	_bool CreateHingeJoint();
	_bool SetPlacementScale(const _float3& vScale);
	_bool SetReturnDrivePaused(_bool bPaused);
	_bool SetPassageBarrierEnabled(_bool bEnabled);
	void UpdatePassageState();
	void DrawDebugDoor();

public:
	static UPtr<CPhysicsDoor> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	CComStaticModelInstance* m_pComModelInstance{};
	CComConstantBuffer* m_pComCBufferPerObject{};
	CComPxRigidBody* m_pComPxDoorRigidBody{};
	CComPxBoxCollider* m_pComPxDoorCollider{};
	CComPxRigidBody* m_pComPxHingeBlockerRigidBody{};
	CComPxBoxCollider* m_pComPxHingeBlockerCollider{};
	CComPxBoxCollider* m_pComPxPassageBarrierCollider{};
	CComPxRigidBody* m_pComPxPassageTriggerRigidBody{};
	CComPxBoxCollider* m_pComPxPassageTriggerCollider{};
	CComPxD6Joint* m_pComPxD6Joint{};
	SPtr<CResVertexShader> m_pResVertexShader{};
	SPtr<CResPixelShader> m_pResPixelShader{};

	_float3 m_vHalfExtents{ 1.875f, 3.735f, 0.354f };
	_float3 m_vHingeBlockerHalfExtents{};
	_float3 m_vPassageBarrierHalfExtents{};
	_float3 m_vHingeWorldPosition{};
	_float3 m_vPassageTriggerHalfExtents{};
	_float3 m_vInitialPosition{};
	_float4 m_vInitialRotation{ 0.f, 0.f, 0.f, 1.f };
	// [LSY] 물리 동기화에 덮어쓰이는 Transform과 분리한 GUI 배치 입력값이다.
	_float3 m_vPlacementEditorPosition{};
	_float3 m_vPlacementEditorRotationEuler{};
	_float3 m_vPlacementEditorScale{ 1.f, 1.f, 1.f };
	_float m_fLowerLimitDegrees{ -110.f };
	_float m_fUpperLimitDegrees{ 110.f };
	_float m_fTwistDriveStiffness{ 300.f };
	_float m_fTwistDriveDamping{ 80.f };
	_float m_fTwistDriveForceLimit{ 1500.f };
	_float m_fCCTPushForce{ 500.f };
	_float m_fPassageOpenAngleDegrees{ 35.f };
	_float3 m_vPlacementScale{ 1.f, 1.f, 1.f };
	_float m_fTestTorque{ 800.f };
	HINGE_SIDE m_eHingeSide{ HINGE_SIDE::LEFT };
	_bool m_bPlayerInsidePassageTrigger{};
	_bool m_bReturnDrivePaused{};
	_bool m_bPassageBarrierEnabled{ true };
	_bool m_bDebugDraw{ true };
};

NS_END
