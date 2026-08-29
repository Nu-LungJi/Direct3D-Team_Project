#pragma once

#include "Client_Defines.h"
#include "Engine_NvClothDefines.h"
#include "GameObject.h"
#include "NvClothCollisionRigData.h"

#include <array>

NS_BEGIN(Engine)
class CComConstantBuffer;
class CComModelInstance;
class CComNvCloth;
class CResNvClothMesh;
class CResPixelShader;
class CResVertexShader;
NS_END

NS_BEGIN(Client)

class CPlayer;

class CNvClothCape final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CNvClothCape, CGameObject)

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		CHandle hTarget{};
		StringID sResourceGroup{};
		StringID sModelResourceTag{};
		StringID sClothMeshResourceTag{};
		StringID sTargetModelComponentTag{};
		_string sAttachBoneName{};
		_float3 vLocalPosition{};
		_float fTeleportDistance{ 3.f };
		_float fTeleportAngleDegrees{ 60.f };
		_bool bUseBackstop{ true };
		_bool bFlipBackstopNormal{};
		_float fBackstopRadius{ 5.f };
		_float fBackstopOffset{};
		_float fBackstopFullRatio{ 0.35f };
		_float fBackstopFadeEndRatio{ 0.9f };
		_float fBackstopFadeDepth{ 0.15f };
		// [LSY] 수직 상승 중에도 몸 안쪽 관통을 막되 하단 끝은 자유롭게 둔다.
		_float fBroomBackstopOffset{};
		_float fBroomBackstopFullRatio{ 0.15f };
		_float fBroomBackstopFadeEndRatio{ 0.55f };
		_float fBroomBackstopFadeDepth{ 0.15f };
		_float fBroomBackstopFullInfluenceRatio{ 0.35f };
		_float fBroomBackstopMinimumInfluenceDepthRatio{ 0.7f };
		// [LSY] 빗자루 상태에서도 망토 하단의 Backstop을 완전히 제거하지 않는다.
		_float fBroomBackstopMinimumInfluence{ 0.12f };
		_bool bUseVirtualParticles{};
		// [LSY] 바람과 무관하게 망토 면끼리 통과하며 꼬이는 현상을 줄인다.
		_bool bUseSelfCollision{ true };
		// [LSY] 고속 이동 후 겹친 면을 좁은 범위에서 강하게 튕기지 않고
		// 조금 넓은 범위에서 부드럽게 분리해 망토가 말리는 현상을 줄인다.
		_float fSelfCollisionDistance{ 0.18f };
		_float fSelfCollisionStiffness{ 0.28f };
		// [LSY] 구조 제약은 형태를 유지하고 굽힘·전단만 완화해
		// 끝단이 아니라 망토 중·하단 전체가 큰 곡률로 움직이게 한다.
		_float fStructuralPhaseStiffness{ 0.9f };
		_float fShearingPhaseStiffness{ 0.7f };
		_float fBendingPhaseStiffness{ 0.45f };
		_bool bUseVelocityWind{ true };
		_float fVelocityWindScale{ 0.55f };
		// [LSY] 상승·하강 속도가 만드는 수직 상대풍만 별도로 감쇠한다.
		_float fVerticalVelocityWindScale{};
		// [LSY] 수직 급가속 시 이동 관성과 Solver 반복을 별도 프로필로 제어한다.
		_float fBroomVerticalInertia{ 0.3f };
		_float fBaseSolverFrequency{ 60.f };
		_float fHighLoadSolverFrequency{ 120.f };
		_float fHighLoadEnterVerticalSpeed{ 6.f };
		_float fHighLoadExitVerticalSpeed{ 3.f };
		_float fHighLoadEnterVerticalAcceleration{ 8.f };
		_float fHighLoadExitVerticalAcceleration{ 4.f };
		_float fMaxWindSpeed{ 20.f };
		_float fWindResponse{ 18.f };
		_float fWindDragCoefficient{ 0.24f };
		_float fWindLiftCoefficient{ 0.08f };
		_float fWindFluidDensity{ 1.f };
		// [LSY] 일정풍에 시간 변화가 있는 횡풍과 돌풍을 합성해
		// 고속 이동 중 망토가 뒤로 붙기만 하지 않고 계속 펄럭이게 한다.
		_bool bUseWindFlutter{ true };
		_float fWindFlutterStrength{ 0.35f };
		_float fWindFlutterFrequency{ 3.5f };
		_float fWindGustStrength{ 0.35f };
		_float fWindGustFrequency{ 0.9f };
		// [LSY] 저속 반응은 유지하고 빗자루 고속 구간에서만 펄럭임을 강조한다.
		_float fHighSpeedWindStart{ 6.f };
		_float fHighSpeedWindFull{ 16.f };
		_float fHighSpeedFlutterStrength{ 0.75f };
		_float fHighSpeedFlutterFrequency{ 5.f };
		_float fHighSpeedGustStrength{ 0.55f };
		_float fMotionConstraintScale{ 1.f };

		// 래그돌 Authoring 데이터를 망토 몸 충돌 리그의 공용 원본으로 사용한다.
		NVCLOTH_COLLISION_RIG_DESC
			tBodyCollisionRig{};
		// [LSY] 빗자루 자세에서 사용하는 별도 충돌 리그다.
		NVCLOTH_COLLISION_RIG_DESC
			tBroomBodyCollisionRig{};
		// [LSY] 별도 CPlayer_Broom 모델에 부착되는 충돌 리그다.
		NVCLOTH_COLLISION_RIG_DESC
			tBroomObjectCollisionRig{};
	};

private:
	CNvClothCape();
	CNvClothCape(const CNvClothCape& Prototype);
	~CNvClothCape() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(_float fTimeDelta) override;
	void FixedUpdate(_float fTimeDelta) override;
	void Update(_float fTimeDelta) override;
	void LateUpdate(_float fTimeDelta) override;
	void UpdateGUI() override;
	HRESULT Render(
		ID3D11DeviceContext* pContext,
		const RENDER_CTX& Context) override;
	HRESULT Render_Shadow(
		ID3D11DeviceContext* pContext,
		const RENDER_CTX& Context) override;

	static UPtr<CNvClothCape> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

	/*----------- 광윤 추가 -----------*/
	bool	GetShadowBounds(BoundingBox& OutBounds) const override;
	/*---------------------------------*/

private:
	struct BODY_COLLISION_BONE
	{
		_string sBoneName{};
		int32_t iBoneIndex{ -1 };
		_float fRadius{};
	};

	struct DEBUG_BODY_COLLISION_SHAPE
	{
		NVCLOTH_COLLISION_SHAPE_TYPE eType{
			NVCLOTH_COLLISION_SHAPE_TYPE::SPHERE };
		_float4x4 SimulationPose{};
		_float3 vHalfExtents{};
		_float fRadius{};
		_float fHalfHeight{};
	};

	struct COLLISION_RIG_PROFILE_CANDIDATE
	{
		_bool bChanged{};
		_bool bUseBroomRig{};
		NVCLOTH_COLLISION_RIG_DESC Rig{};
		std::vector<int32_t> BoneIndices{};
	};

private:
	_bool UpdateAttachment(
		_bool bUpdateSimulation,
		_bool bForceTeleport = false);
	_bool ResolveAttachment();
	_bool UpdateBodyCollisions();
	_bool UpdateBodyCollisions(
		const NVCLOTH_COLLISION_RIG_DESC& BodyRig,
		const std::vector<int32_t>& BodyRigBoneIndices);
	_bool PrepareCollisionRigProfile(
		CPlayer* pPlayer,
		COLLISION_RIG_PROFILE_CANDIDATE& OutCandidate);
	void CommitCollisionRigProfile(
		COLLISION_RIG_PROFILE_CANDIDATE&& Candidate);
	_bool ResolveCollisionRigBones(
		const NVCLOTH_COLLISION_RIG_DESC& Rig,
		CComModelInstance& ModelInstance,
		std::vector<int32_t>& OutBoneIndices,
		const char* szProfileName);
	_bool UpdateVirtualWind(
		_float fTimeDelta,
		CPlayer* pPlayer,
		_bool bSuppressed);
	_bool UpdateRuntimeSimulationProfile(
		_float fTimeDelta,
		CPlayer* pPlayer);
	_bool ValidateAndRecoverSimulation();
	_bool RecoverSimulation(
		const char* szReason,
		size_t iParticleIndex,
		_bool bRunaway);
	_bool ResetSimulationToAnimationPose();
	_bool GetValidatedRenderParticleView(
		NVCLOTH_RENDER_PARTICLE_VIEW& OutView);
	_bool AppendCollisionsFromRig(
		const NVCLOTH_COLLISION_RIG_DESC& Rig,
		const std::vector<int32_t>& BoneIndices,
		CComModelInstance& ModelInstance,
		_fmatrix TargetWorld,
		_fmatrix InverseSimulationWorld,
		NVCLOTH_COLLISION_DESC& OutDesc,
		std::vector<DEBUG_BODY_COLLISION_SHAPE>&
			OutDebugShapes);
	void DebugDrawBodyCollisions();
	_bool UpdateAnimationConstraints(
		CComModelInstance& ModelInstance,
		_fmatrix AttachmentWorld,
		_bool bResetPreviousParticles);
	_bool GetTargetBoneMatrix(
		CComModelInstance& ModelInstance,
		int32_t iBoneIndex,
		_matrix& OutBoneMatrix) const;

private:
	CHandle m_hTarget{};
	_float4x4 m_ParentWorld{};
	StringID m_sTargetModelComponentTag{};
	_string m_sAttachBoneName{};
	int32_t m_iAttachBoneIndex{ -1 };
	_float m_fTeleportDistance{ 3.f };
	_float m_fTeleportAngleDegrees{ 60.f };
	_float3 m_vPreviousAttachPosition{};
	_float4 m_vPreviousAttachRotation{ 0.f, 0.f, 0.f, 1.f };
	_bool m_bAttachmentInitialized{};
	_bool m_bSimulationTransformInitialized{};
	_bool m_bAnimationConstraintInitialized{};
	_bool m_bOwnerRenderSuppressed{};
	std::vector<int32_t>
		m_ResolvedSkinBoneIndices{};
	std::vector<_float4x4>
		m_SkinBoneToSimulationMatrices{};
	NVCLOTH_ANIMATION_CONSTRAINT_DESC
		m_AnimationConstraintDesc{};
	std::array<BODY_COLLISION_BONE, 5>
		m_BodyCollisionBones{ {
			{ "Spine2", -1, 0.30f },
			{ "Spine3", -1, 0.32f },
			{ "Neck", -1, 0.18f },
			{ "LeftShoulder", -1, 0.22f },
			{ "RightShoulder", -1, 0.22f }
		} };
	_bool m_bContinuousBodyCollision{ true };
	_float m_fCollisionMassScale{ 1.f };
	_float m_fCollisionFriction{};
	_bool m_bUseBackstop{ true };
	_bool m_bFlipBackstopNormal{};
	_float m_fBackstopRadius{ 5.f };
	_float m_fBackstopOffset{};
	_float m_fBackstopFullRatio{ 0.35f };
	_float m_fBackstopFadeEndRatio{ 0.9f };
	_float m_fBackstopFadeDepth{ 0.15f };
	_float m_fBroomBackstopOffset{};
	_float m_fBroomBackstopFullRatio{ 0.15f };
	_float m_fBroomBackstopFadeEndRatio{ 0.55f };
	_float m_fBroomBackstopFadeDepth{ 0.15f };
	_float m_fBroomBackstopFullInfluenceRatio{ 0.35f };
	_float m_fBroomBackstopMinimumInfluenceDepthRatio{ 0.7f };
	_float m_fBroomBackstopMinimumInfluence{ 0.12f };
	_bool m_bUseVirtualParticles{};
	_bool m_bUseSelfCollision{ true };
	_float m_fSelfCollisionDistance{ 0.18f };
	_float m_fSelfCollisionStiffness{ 0.28f };
	_bool m_bUseVelocityWind{ true };
	_float m_fVelocityWindScale{ 0.55f };
	_float m_fVerticalVelocityWindScale{};
	_float m_fBroomVerticalInertia{ 0.3f };
	_float m_fBaseSolverFrequency{ 60.f };
	_float m_fHighLoadSolverFrequency{ 120.f };
	_float m_fHighLoadEnterVerticalSpeed{ 6.f };
	_float m_fHighLoadExitVerticalSpeed{ 3.f };
	_float m_fHighLoadEnterVerticalAcceleration{ 8.f };
	_float m_fHighLoadExitVerticalAcceleration{ 4.f };
	_float m_fPreviousVerticalVelocity{};
	_float m_fCurrentVerticalVelocity{};
	_float m_fFilteredVerticalAcceleration{};
	_bool m_bVerticalVelocityInitialized{};
	_bool m_bRuntimeSimulationProfileInitialized{};
	_bool m_bHighLoadSolverEnabled{};
	_float3 m_vAppliedLinearInertia{};
	_float m_fAppliedSolverFrequency{};
	_float m_fMaxWindSpeed{ 20.f };
	_float m_fWindResponse{ 18.f };
	_float m_fWindDragCoefficient{ 0.24f };
	_float m_fWindLiftCoefficient{ 0.08f };
	_float m_fWindFluidDensity{ 1.f };
	_bool m_bUseWindFlutter{ true };
	_float m_fWindFlutterStrength{ 0.35f };
	_float m_fWindFlutterFrequency{ 3.5f };
	_float m_fWindGustStrength{ 0.35f };
	_float m_fWindGustFrequency{ 0.9f };
	_float m_fHighSpeedWindStart{ 6.f };
	_float m_fHighSpeedWindFull{ 16.f };
	_float m_fHighSpeedFlutterStrength{ 0.75f };
	_float m_fHighSpeedFlutterFrequency{ 5.f };
	_float m_fHighSpeedGustStrength{ 0.55f };
	_float m_fHighSpeedWindBlend{};
	_float m_fWindFlutterPhase{};
	_float m_fWindTime{};
	_float3 m_vCurrentWindVelocity{};
	_float m_fMotionConstraintScale{ 1.f };
	std::vector<_float3> m_SimulationValidationParticles{};
	uint32_t m_iSimulationValidationTick{};
	uint32_t m_iSimulationRecoveryCount{};
	uint32_t m_iSimulationRunawayRecoveryCount{};
	uint32_t m_iSimulationDistanceWarningCount{};
	uint32_t m_iConsecutiveDistanceWarningSamples{};
	size_t m_iLastDistanceWarningParticleCount{};
	_float m_fLastMaximumParticleDistance{};
	_bool m_bSimulationValidationFailureLogged{};
	_bool m_bSimulationRecoveryLogged{};
	_bool m_bSimulationDistanceWarningLogged{};
	_bool m_bCollisionUpdateFailureLogged{};
	_bool m_bRenderParticleViewFailureLogged{};
	NVCLOTH_COLLISION_RIG_DESC
		m_BodyCollisionRig{};
	NVCLOTH_COLLISION_RIG_DESC
		m_GroundBodyCollisionRig{};
	NVCLOTH_COLLISION_RIG_DESC
		m_BroomBodyCollisionRig{};
	NVCLOTH_COLLISION_RIG_DESC
		m_BroomObjectCollisionRig{};
	std::vector<int32_t>
		m_CollisionRigBoneIndices{};
	std::vector<int32_t>
		m_BroomObjectCollisionRigBoneIndices{};
	_bool m_bUsingBroomCollisionRig{};
	_bool m_bCollisionRigProfileChanged{};
	_bool m_bBroomObjectCollisionRequested{};
	_bool m_bBroomObjectCollisionApplied{};
	size_t m_iResolvedBroomObjectShapeCount{};
	size_t m_iBroomDebugShapeStart{
		std::numeric_limits<size_t>::max() };
	_bool m_bDebugBodyCollisions{};
	NVCLOTH_COLLISION_DESC
		m_LastBodyCollisionDesc{};
	std::vector<DEBUG_BODY_COLLISION_SHAPE>
		m_DebugBodyCollisionShapes{};
	_bool m_bRenderCape{ true };
	SPtr<CResNvClothMesh> m_pClothMesh{};
	CComModelInstance* m_pComModelInstance{};
	CComNvCloth* m_pComNvCloth{};
	CComConstantBuffer* m_pComCBufferPerObject{};
	SPtr<CResVertexShader> m_pVertexShader{};
	SPtr<CResVertexShader> m_pShadowVertexShader{};
	SPtr<CResVertexShader> m_pPointShadowVertexShader{};
	SPtr<CResPixelShader> m_pPixelShader{};

public:
	void RequestClothWindImpulse(
		const _float3& vVelocity,
		_float fDuration);
	const _float3& GetClothWindImpulseVelocity() const
	{
		return m_vClothWindImpulseVelocity;
	}
private:
	_float3 m_vClothWindImpulseVelocity{};
	_float m_fClothWindImpulseRemaining{};

};

NS_END
