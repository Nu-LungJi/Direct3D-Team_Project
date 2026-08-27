#pragma once

#include "Component.h"
#include "Engine_PhysxDefines.h"

NS_BEGIN(Engine)

class CComModelInstance;

class ENGINE_DLL CComFootIK final : public CComponent
{
public:
	DECLARE_DERIVED_TYPE(CComFootIK, CComponent)

	struct LEG_BONE_NAMES
	{
		std::string sUpperLeg{};
		std::string sLowerLeg{};
		std::string sFoot{};
		std::string sToe{};
	};

	struct DESC : public CComponent::DESC
	{
		LEG_BONE_NAMES tLeftLeg{};
		LEG_BONE_NAMES tRightLeg{};
		std::string sPelvisBone{};
		_float fTraceStartHeight{ 0.35f };
		_float fTraceDistance{ 0.8f };
		_float fFootHeight{ 0.04f };
		_float fBlendSpeed{ 12.f };
		_float fMaxStepHeight{ 0.45f };
		_float fMaxExtensionRatio{ 0.995f };
		_float fMaxFootSlopeDegrees{ 45.f };
		_float fMaxPelvisDrop{ 0.4f };
		_float fPelvisBlendSpeed{ 8.f };
		_float fLiftReleaseSpeed{ 0.15f };
		uint32_t iGroundQueryMask{ PX_ALL_LAYERS };
		_bool bEnabled{ true };
	};

	struct LEG_BONE_INDICES
	{
		int32_t iUpperLeg{ -1 };
		int32_t iLowerLeg{ -1 };
		int32_t iFoot{ -1 };
		int32_t iToe{ -1 };

		_bool IsValid() const
		{
			return iUpperLeg >= 0 && iLowerLeg >= 0 && iFoot >= 0;
		}
	};

	struct FOOT_GROUND_STATE
	{
		_bool bHit{};
		_bool bHasTarget{};
		_bool bAnimationLifting{};
		_bool bPreviousPositionValid{};
		_float3 vAnimatedWorldPosition{};
		_float3 vPreviousAnimatedWorldPosition{};
		_float3 vTargetWorldPosition{};
		_float3 vGroundNormal{ 0.f, 1.f, 0.f };
		_float fDistance{};
		_float fWeight{};
		_float fAnimatedVerticalSpeed{};
		CHandle hGroundObject{};
	};

	struct DEBUG_LEG_STATE
	{
		_bool bValid{};
		_float3 vHipWorld{};
		_float3 vKneeWorld{};
		_float3 vFootWorld{};
		_float3 vSolveTargetWorld{};
		_float fSolveError{};
		_bool bHasSolveTarget{};
	};

private:
	explicit CComFootIK();
	explicit CComFootIK(const CComFootIK& Prototype);
	~CComFootIK() override;

private:
	HRESULT Initialize(void* pArg) override;

public:
	_bool BindModel(const CComModelInstance& ModelInstance);
	void UpdateGroundSamples(
		_float fTimeDelta,
		const _float3& vLeftFootWorldPosition,
		const _float3& vRightFootWorldPosition);
	void ClearGroundSamples(_float fTimeDelta = 0.f);
	_bool ApplyToLocalPose(
		const CComModelInstance& ModelInstance,
		std::vector<_float4x4>& LocalBoneMatrices,
		_float fTimeDelta);
	void FinalizeDebugPose(const CComModelInstance& ModelInstance);

	void SetEnabled(_bool bEnabled) { m_bEnabled = bEnabled; }
	_bool IsEnabled() const { return m_bEnabled; }
	_bool HasValidSkeleton() const
	{
		return m_iPelvisBone >= 0 &&
			m_tLeftLegIndices.IsValid() &&
			m_tRightLegIndices.IsValid();
	}

	const LEG_BONE_INDICES& GetLeftLegIndices() const
	{
		return m_tLeftLegIndices;
	}
	const LEG_BONE_INDICES& GetRightLegIndices() const
	{
		return m_tRightLegIndices;
	}
	int32_t GetPelvisBoneIndex() const { return m_iPelvisBone; }
	const FOOT_GROUND_STATE& GetLeftFootState() const { return m_tLeftFootState; }
	const FOOT_GROUND_STATE& GetRightFootState() const { return m_tRightFootState; }

	void UpdateGUI() override;

private:
	_bool SampleGround(const _float3& vFootWorldPosition, FOOT_GROUND_STATE& OutState) const;
	_bool SolveLeg(
		const CComModelInstance& ModelInstance,
		const LEG_BONE_INDICES& Leg,
		const FOOT_GROUND_STATE& GroundState,
		std::vector<_float4x4>& LocalBoneMatrices);
	void UpdateDebugLegState(
		const CComModelInstance& ModelInstance,
		const LEG_BONE_INDICES& Leg,
		const std::vector<_float4x4>& CombinedBoneMatrices,
		DEBUG_LEG_STATE& OutState);
	void ApplyPelvisOffset(
		const CComModelInstance& ModelInstance,
		std::vector<_float4x4>& LocalBoneMatrices,
		_float fTimeDelta);
	_float CalculateLegReachExcess(
		const CComModelInstance& ModelInstance,
		const LEG_BONE_INDICES& Leg,
		const FOOT_GROUND_STATE& GroundState,
		const std::vector<_float4x4>& CombinedPose) const;
	void UpdateStateWeight(FOOT_GROUND_STATE& State, _float fTimeDelta) const;
	void DrawDebugVisualization() const;
	static LEG_BONE_INDICES ResolveLegIndices(
		const CComModelInstance& ModelInstance,
		const LEG_BONE_NAMES& Names);

private:
	LEG_BONE_NAMES m_tLeftLegNames{};
	LEG_BONE_NAMES m_tRightLegNames{};
	std::string m_sPelvisBoneName{};
	LEG_BONE_INDICES m_tLeftLegIndices{};
	LEG_BONE_INDICES m_tRightLegIndices{};
	int32_t m_iPelvisBone{ -1 };

	FOOT_GROUND_STATE m_tLeftFootState{};
	FOOT_GROUND_STATE m_tRightFootState{};
	_float m_fTraceStartHeight{ 0.35f };
	_float m_fTraceDistance{ 0.8f };
	_float m_fFootHeight{ 0.04f };
	_float m_fBlendSpeed{ 12.f };
	_float m_fMaxStepHeight{ 0.45f };
	_float m_fMaxExtensionRatio{ 0.995f };
	_float m_fMaxFootSlopeDegrees{ 45.f };
	_float m_fMaxPelvisDrop{ 0.4f };
	_float m_fPelvisBlendSpeed{ 8.f };
	_float m_fLiftReleaseSpeed{ 0.15f };
	_float m_fCurrentPelvisOffsetY{};
	int32_t m_iLeftFootUpAxis{ -1 };
	int32_t m_iRightFootUpAxis{ -1 };
	_float m_fLeftFootUpSign{ 1.f };
	_float m_fRightFootUpSign{ 1.f };
	uint32_t m_iGroundQueryMask{ PX_ALL_LAYERS };
	_bool m_bEnabled{ true };
	_bool m_bDebugDraw{ true };
	_bool m_bDebugDrawSkeleton{ true };
	_bool m_bDebugDepthTest{ false };
	mutable DEBUG_LEG_STATE m_tLeftDebugState{};
	mutable DEBUG_LEG_STATE m_tRightDebugState{};
	// Reused by the per-frame IK solve. Keeping this storage on the component
	// avoids allocating a full bone-matrix array for each foot every frame.
	std::vector<_float4x4> m_CombinedPoseScratch{};

public:
	static UPtr<CComFootIK> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	void Free() override;
};

NS_END
