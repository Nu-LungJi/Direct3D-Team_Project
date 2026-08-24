#include "pch.h"
#include "ComFootIK.h"

#include "ComModelInstance.h"
#include "DbgLineRender.h"
#include "GameInstance.h"
#include "GameObject.h"
#include "PhysXManager.h"
#include "ResModel.h"
#include "ResModelBone.h"

NS_USING(Engine)

namespace
{
	constexpr _float FOOT_IK_EPSILON = 0.0001f;

	void BuildCombinedPose(
		const CResModel& Model,
		const std::vector<_float4x4>& LocalPose,
		std::vector<_float4x4>& OutCombined)
	{
		const auto& Bones = Model.GetBones();
		OutCombined.resize(Bones.size());
		const _matrix matPreTransform =
			XMLoadFloat4x4(&Model.Get_PreTransformMatrix());
		for (size_t i = 0; i < Bones.size(); ++i)
		{
			const int32_t iParent = Bones[i]->GetParendBoneIndex();
			const _matrix matParent = iParent >= 0
				? XMLoadFloat4x4(&OutCombined[static_cast<size_t>(iParent)])
				: matPreTransform;
			XMStoreFloat4x4(
				&OutCombined[i], XMLoadFloat4x4(&LocalPose[i]) * matParent);
		}
	}

	_vector GetTranslation(const _float4x4& Matrix)
	{
		return XMLoadFloat4x4(&Matrix).r[3];
	}

	_vector RotationBetweenNormals(_vector vFrom, _vector vTo)
	{
		vFrom = XMVector3Normalize(vFrom);
		vTo = XMVector3Normalize(vTo);
		const _float fDot = std::clamp(
			XMVectorGetX(XMVector3Dot(vFrom, vTo)), -1.f, 1.f);
		if (fDot >= 1.f - FOOT_IK_EPSILON)
			return XMQuaternionIdentity();
		if (fDot <= -1.f + FOOT_IK_EPSILON)
		{
			_vector vAxis = XMVector3Cross(
				vFrom, XMVectorSet(1.f, 0.f, 0.f, 0.f));
			if (XMVectorGetX(XMVector3LengthSq(vAxis)) <= FOOT_IK_EPSILON)
			{
				vAxis = XMVector3Cross(
					vFrom, XMVectorSet(0.f, 1.f, 0.f, 0.f));
			}
			return XMQuaternionRotationAxis(vAxis, XM_PI);
		}

		const _vector vCross = XMVector3Cross(vFrom, vTo);
		return XMQuaternionNormalize(XMVectorSet(
			XMVectorGetX(vCross),
			XMVectorGetY(vCross),
			XMVectorGetZ(vCross),
			1.f + fDot));
	}

	_bool RotateBoneToward(
		const CResModel& Model,
		int32_t iBoneIndex,
		_vector vCurrentDirection,
		_vector vDesiredDirection,
		_float fWeight,
		std::vector<_float4x4>& LocalPose,
		const std::vector<_float4x4>& CombinedPose)
	{
		if (iBoneIndex < 0 || fWeight <= 0.f ||
			XMVectorGetX(XMVector3LengthSq(vCurrentDirection)) <= FOOT_IK_EPSILON ||
			XMVectorGetX(XMVector3LengthSq(vDesiredDirection)) <= FOOT_IK_EPSILON)
		{
			return false;
		}

		vCurrentDirection = XMVector3Normalize(vCurrentDirection);
		vDesiredDirection = XMVector3Normalize(vDesiredDirection);
		const _vector qDelta = RotationBetweenNormals(
			vCurrentDirection, vDesiredDirection);
		_matrix matDesiredCombined =
			XMLoadFloat4x4(&CombinedPose[static_cast<size_t>(iBoneIndex)]) *
			XMMatrixRotationQuaternion(qDelta);
		matDesiredCombined.r[3] =
			XMLoadFloat4x4(&CombinedPose[static_cast<size_t>(iBoneIndex)]).r[3];

		const auto& Bones = Model.GetBones();
		const int32_t iParent =
			Bones[static_cast<size_t>(iBoneIndex)]->GetParendBoneIndex();
		const _matrix matParent = iParent >= 0
			? XMLoadFloat4x4(&CombinedPose[static_cast<size_t>(iParent)])
			: XMLoadFloat4x4(&Model.Get_PreTransformMatrix());
		const _matrix matDesiredLocal =
			matDesiredCombined * XMMatrixInverse(nullptr, matParent);
		const _matrix matOriginalLocal =
			XMLoadFloat4x4(&LocalPose[static_cast<size_t>(iBoneIndex)]);

		_vector vOriginalScale{}, qOriginal{}, vOriginalTranslation{};
		_vector vDesiredScale{}, qDesired{}, vDesiredTranslation{};
		if (!XMMatrixDecompose(
			&vOriginalScale, &qOriginal, &vOriginalTranslation, matOriginalLocal) ||
			!XMMatrixDecompose(
			&vDesiredScale, &qDesired, &vDesiredTranslation, matDesiredLocal))
		{
			return false;
		}

		const _vector qBlended = XMQuaternionNormalize(XMQuaternionSlerp(
			qOriginal, qDesired, std::clamp(fWeight, 0.f, 1.f)));
		XMStoreFloat4x4(
			&LocalPose[static_cast<size_t>(iBoneIndex)],
			XMMatrixScalingFromVector(vOriginalScale) *
			XMMatrixRotationQuaternion(qBlended) *
			XMMatrixTranslationFromVector(vOriginalTranslation));
		return true;
	}
}

CComFootIK::CComFootIK() = default;

CComFootIK::CComFootIK(const CComFootIK& Prototype)
	: CComponent{ Prototype }
{
}

CComFootIK::~CComFootIK() = default;

HRESULT CComFootIK::Initialize(void* pArg)
{
	if (!pArg || FAILED(CComponent::Initialize(pArg)))
		return E_FAIL;

	const auto* pDesc = static_cast<const DESC*>(pArg);
	m_tLeftLegNames = pDesc->tLeftLeg;
	m_tRightLegNames = pDesc->tRightLeg;
	m_sPelvisBoneName = pDesc->sPelvisBone;
	m_fTraceStartHeight = std::max(0.f, pDesc->fTraceStartHeight);
	m_fTraceDistance = std::max(0.f, pDesc->fTraceDistance);
	m_fFootHeight = std::max(0.f, pDesc->fFootHeight);
	m_fBlendSpeed = std::max(0.f, pDesc->fBlendSpeed);
	m_fMaxStepHeight = std::max(0.f, pDesc->fMaxStepHeight);
	m_fMaxExtensionRatio = std::clamp(
		pDesc->fMaxExtensionRatio, 0.5f, 0.9999f);
	m_fMaxFootSlopeDegrees = std::clamp(
		pDesc->fMaxFootSlopeDegrees, 0.f, 89.f);
	m_fMaxPelvisDrop = std::max(0.f, pDesc->fMaxPelvisDrop);
	m_fPelvisBlendSpeed = std::max(0.f, pDesc->fPelvisBlendSpeed);
	m_fLiftReleaseSpeed = std::max(0.f, pDesc->fLiftReleaseSpeed);
	m_iGroundQueryMask = pDesc->iGroundQueryMask;
	m_bEnabled = pDesc->bEnabled;

	return S_OK;
}

_bool CComFootIK::BindModel(const CComModelInstance& ModelInstance)
{
	const auto pModel = ModelInstance.GetModel();
	if (!pModel)
		return false;

	m_tLeftLegIndices = ResolveLegIndices(ModelInstance, m_tLeftLegNames);
	m_tRightLegIndices = ResolveLegIndices(ModelInstance, m_tRightLegNames);
	m_iLeftFootUpAxis = -1;
	m_iRightFootUpAxis = -1;
	m_fLeftFootUpSign = 1.f;
	m_fRightFootUpSign = 1.f;
	m_iPelvisBone = m_sPelvisBoneName.empty()
		? -1
		: pModel->Get_BoneIndex(m_sPelvisBoneName.c_str());
	m_CombinedPoseScratch.resize(pModel->GetBones().size());
	return HasValidSkeleton();
}

void CComFootIK::UpdateGroundSamples(
	_float fTimeDelta,
	const _float3& vLeftFootWorldPosition,
	const _float3& vRightFootWorldPosition)
{
	const auto UpdateLiftState = [this, fTimeDelta](
		FOOT_GROUND_STATE& State,
		const _float3& vAnimatedWorldPosition)
	{
		State.fAnimatedVerticalSpeed = 0.f;
		if (State.bPreviousPositionValid && fTimeDelta > FOOT_IK_EPSILON)
		{
			State.fAnimatedVerticalSpeed =
				(vAnimatedWorldPosition.y -
				 State.vPreviousAnimatedWorldPosition.y) / fTimeDelta;
		}
		State.bAnimationLifting =
			State.bPreviousPositionValid &&
			State.fAnimatedVerticalSpeed > m_fLiftReleaseSpeed;
		State.vPreviousAnimatedWorldPosition = vAnimatedWorldPosition;
		State.bPreviousPositionValid = true;
	};
	UpdateLiftState(m_tLeftFootState, vLeftFootWorldPosition);
	UpdateLiftState(m_tRightFootState, vRightFootWorldPosition);
	m_tLeftFootState.vAnimatedWorldPosition = vLeftFootWorldPosition;
	m_tRightFootState.vAnimatedWorldPosition = vRightFootWorldPosition;

	if (m_bEnabled)
	{
		if (!m_tLeftFootState.bAnimationLifting)
			SampleGround(vLeftFootWorldPosition, m_tLeftFootState);
		else
			m_tLeftFootState.bHit = false;
		if (!m_tRightFootState.bAnimationLifting)
			SampleGround(vRightFootWorldPosition, m_tRightFootState);
		else
			m_tRightFootState.bHit = false;
	}
	else
	{
		m_tLeftFootState.bHit = false;
		m_tRightFootState.bHit = false;
	}

	UpdateStateWeight(m_tLeftFootState, fTimeDelta);
	UpdateStateWeight(m_tRightFootState, fTimeDelta);
}

void CComFootIK::ClearGroundSamples(_float fTimeDelta)
{
	m_tLeftFootState.bHit = false;
	m_tRightFootState.bHit = false;
	UpdateStateWeight(m_tLeftFootState, fTimeDelta);
	UpdateStateWeight(m_tRightFootState, fTimeDelta);
}

_bool CComFootIK::ApplyToLocalPose(
	const CComModelInstance& ModelInstance,
	std::vector<_float4x4>& LocalBoneMatrices,
	_float fTimeDelta)
{
	if (!m_bEnabled || !HasValidSkeleton())
		return false;

	const auto pModel = ModelInstance.GetModel();
	if (!pModel || LocalBoneMatrices.size() != pModel->GetBones().size())
		return false;

	// Sample from this frame's animated pose. A target stored by LateUpdate is
	// one frame old in world space and makes the legs trail behind the bones.
	auto& AnimatedCombinedPose = m_CombinedPoseScratch;
	BuildCombinedPose(*pModel, LocalBoneMatrices, AnimatedCombinedPose);
	const _matrix matOwnerWorld =
		m_pGameObject->GetTransform().GetLoadedCombinedWorldMatrix();
	_float3 vLeftFootWorld{};
	_float3 vRightFootWorld{};
	XMStoreFloat3(&vLeftFootWorld, XMVector3TransformCoord(
		GetTranslation(AnimatedCombinedPose[
			static_cast<size_t>(m_tLeftLegIndices.iFoot)]), matOwnerWorld));
	XMStoreFloat3(&vRightFootWorld, XMVector3TransformCoord(
		GetTranslation(AnimatedCombinedPose[
			static_cast<size_t>(m_tRightLegIndices.iFoot)]), matOwnerWorld));
	UpdateGroundSamples(fTimeDelta, vLeftFootWorld, vRightFootWorld);
	m_tLeftDebugState.bHasSolveTarget = false;
	m_tRightDebugState.bHasSolveTarget = false;
	m_tLeftDebugState.fSolveError = 0.f;
	m_tRightDebugState.fSolveError = 0.f;
	ApplyPelvisOffset(ModelInstance, LocalBoneMatrices, fTimeDelta);

	_bool bApplied{};
	bApplied |= SolveLeg(
		ModelInstance, m_tLeftLegIndices,
		m_tLeftFootState, LocalBoneMatrices);
	bApplied |= SolveLeg(
		ModelInstance, m_tRightLegIndices,
		m_tRightFootState, LocalBoneMatrices);
	return bApplied;
}

void CComFootIK::ApplyPelvisOffset(
	const CComModelInstance& ModelInstance,
	std::vector<_float4x4>& LocalBoneMatrices,
	_float fTimeDelta)
{
	const auto pModel = ModelInstance.GetModel();
	if (!pModel || m_iPelvisBone < 0 || !m_pGameObject)
		return;

	auto& CombinedPose = m_CombinedPoseScratch;
	BuildCombinedPose(*pModel, LocalBoneMatrices, CombinedPose);
	const _float fLeftExcess = CalculateLegReachExcess(
		ModelInstance, m_tLeftLegIndices, m_tLeftFootState, CombinedPose);
	const _float fRightExcess = CalculateLegReachExcess(
		ModelInstance, m_tRightLegIndices, m_tRightFootState, CombinedPose);
	const _float fDesiredOffsetY = -std::clamp(
		std::max(fLeftExcess, fRightExcess), 0.f, m_fMaxPelvisDrop);

	if (fTimeDelta > 0.f && m_fPelvisBlendSpeed > 0.f)
	{
		const _float fAlpha =
			1.f - std::exp(-m_fPelvisBlendSpeed * fTimeDelta);
		m_fCurrentPelvisOffsetY = std::lerp(
			m_fCurrentPelvisOffsetY, fDesiredOffsetY, fAlpha);
	}
	else
	{
		m_fCurrentPelvisOffsetY = fDesiredOffsetY;
	}

	if (std::abs(m_fCurrentPelvisOffsetY) <= FOOT_IK_EPSILON)
		return;

	const _matrix matInverseWorld = XMMatrixInverse(
		nullptr, m_pGameObject->GetTransform().GetLoadedCombinedWorldMatrix());
	const _vector vModelDisplacement = XMVector3TransformNormal(
		XMVectorSet(0.f, m_fCurrentPelvisOffsetY, 0.f, 0.f),
		matInverseWorld);
	const auto& Bones = pModel->GetBones();
	const int32_t iParent =
		Bones[static_cast<size_t>(m_iPelvisBone)]->GetParendBoneIndex();
	const _matrix matParent = iParent >= 0
		? XMLoadFloat4x4(&CombinedPose[static_cast<size_t>(iParent)])
		: XMLoadFloat4x4(&pModel->Get_PreTransformMatrix());
	const _vector vLocalDisplacement = XMVector3TransformNormal(
		vModelDisplacement, XMMatrixInverse(nullptr, matParent));
	_matrix matPelvisLocal = XMLoadFloat4x4(
		&LocalBoneMatrices[static_cast<size_t>(m_iPelvisBone)]);
	matPelvisLocal.r[3] = XMVectorSetW(
		matPelvisLocal.r[3] + vLocalDisplacement, 1.f);
	XMStoreFloat4x4(
		&LocalBoneMatrices[static_cast<size_t>(m_iPelvisBone)],
		matPelvisLocal);
}

_float CComFootIK::CalculateLegReachExcess(
	const CComModelInstance& ModelInstance,
	const LEG_BONE_INDICES& Leg,
	const FOOT_GROUND_STATE& GroundState,
	const std::vector<_float4x4>& CombinedPose) const
{
	if (!Leg.IsValid() || !GroundState.bHit || !m_pGameObject)
		return 0.f;
	const auto pModel = ModelInstance.GetModel();
	if (!pModel)
		return 0.f;

	const _vector vHip = GetTranslation(
		CombinedPose[static_cast<size_t>(Leg.iUpperLeg)]);
	const _vector vKnee = GetTranslation(
		CombinedPose[static_cast<size_t>(Leg.iLowerLeg)]);
	const _vector vFoot = GetTranslation(
		CombinedPose[static_cast<size_t>(Leg.iFoot)]);
	const _float fMaxReach =
		(XMVectorGetX(XMVector3Length(vKnee - vHip)) +
		 XMVectorGetX(XMVector3Length(vFoot - vKnee))) *
		m_fMaxExtensionRatio;
	const _matrix matInverseWorld = XMMatrixInverse(
		nullptr, m_pGameObject->GetTransform().GetLoadedCombinedWorldMatrix());
	const _vector vTarget = XMVector3TransformCoord(
		XMLoadFloat3(&GroundState.vTargetWorldPosition), matInverseWorld);
	return std::max(
		0.f, XMVectorGetX(XMVector3Length(vTarget - vHip)) - fMaxReach);
}

void CComFootIK::FinalizeDebugPose(
	const CComModelInstance& ModelInstance)
{
	const auto& CombinedBoneMatrices =
		ModelInstance.Get_CombinedBoneMatrices();
	UpdateDebugLegState(
		ModelInstance, m_tLeftLegIndices,
		CombinedBoneMatrices, m_tLeftDebugState);
	UpdateDebugLegState(
		ModelInstance, m_tRightLegIndices,
		CombinedBoneMatrices, m_tRightDebugState);
	// Foot IK 디버그 라인은 필요할 때만 다시 활성화한다.
	// if (m_bDebugDraw)
	// 	DrawDebugVisualization();
}

_bool CComFootIK::SolveLeg(
	const CComModelInstance& ModelInstance,
	const LEG_BONE_INDICES& Leg,
	const FOOT_GROUND_STATE& GroundState,
	std::vector<_float4x4>& LocalBoneMatrices)
{
	if (!Leg.IsValid() || !GroundState.bHasTarget ||
		GroundState.fWeight <= FOOT_IK_EPSILON ||
		!m_pGameObject)
	{
		return false;
	}

	const auto pModel = ModelInstance.GetModel();
	if (!pModel)
		return false;

	auto& CombinedPose = m_CombinedPoseScratch;
	BuildCombinedPose(*pModel, LocalBoneMatrices, CombinedPose);
	const _vector vHip = GetTranslation(
		CombinedPose[static_cast<size_t>(Leg.iUpperLeg)]);
	const _vector vKnee = GetTranslation(
		CombinedPose[static_cast<size_t>(Leg.iLowerLeg)]);
	const _vector vAnkle = GetTranslation(
		CombinedPose[static_cast<size_t>(Leg.iFoot)]);
	const _float fUpperLength = XMVectorGetX(XMVector3Length(vKnee - vHip));
	const _float fLowerLength = XMVectorGetX(XMVector3Length(vAnkle - vKnee));
	if (fUpperLength <= FOOT_IK_EPSILON || fLowerLength <= FOOT_IK_EPSILON)
		return false;

	const _matrix matInverseWorld = XMMatrixInverse(
		nullptr, m_pGameObject->GetTransform().GetLoadedCombinedWorldMatrix());
	_vector vTarget = XMVector3TransformCoord(
		XMLoadFloat3(&GroundState.vTargetWorldPosition), matInverseWorld);
	_vector vHipToTarget = vTarget - vHip;
	_float fTargetDistance = XMVectorGetX(XMVector3Length(vHipToTarget));
	if (fTargetDistance <= FOOT_IK_EPSILON)
		return false;

	const _vector vTargetDirection = XMVector3Normalize(vHipToTarget);
	const _float fMinReach =
		std::abs(fUpperLength - fLowerLength) + FOOT_IK_EPSILON;
	const _float fMaxReach =
		(fUpperLength + fLowerLength) * m_fMaxExtensionRatio;
	fTargetDistance = std::clamp(fTargetDistance, fMinReach, fMaxReach);
	vTarget = vHip + vTargetDirection * fTargetDistance;
	DEBUG_LEG_STATE& DebugState =
		Leg.iFoot == m_tLeftLegIndices.iFoot
		? m_tLeftDebugState
		: m_tRightDebugState;
	const _matrix matWorld =
		m_pGameObject->GetTransform().GetLoadedCombinedWorldMatrix();
	XMStoreFloat3(&DebugState.vSolveTargetWorld,
		XMVector3TransformCoord(vTarget, matWorld));
	DebugState.bHasSolveTarget = true;

	_vector vBendDirection = vKnee - vHip;
	vBendDirection -= vTargetDirection *
		XMVectorGetX(XMVector3Dot(vBendDirection, vTargetDirection));
	if (XMVectorGetX(XMVector3LengthSq(vBendDirection)) <= FOOT_IK_EPSILON)
	{
		vBendDirection = XMVectorSet(0.f, 0.f, 1.f, 0.f);
		vBendDirection -= vTargetDirection *
			XMVectorGetX(XMVector3Dot(vBendDirection, vTargetDirection));
	}
	if (XMVectorGetX(XMVector3LengthSq(vBendDirection)) <= FOOT_IK_EPSILON)
		vBendDirection = XMVectorSet(1.f, 0.f, 0.f, 0.f);
	vBendDirection = XMVector3Normalize(vBendDirection);

	const _float fAlong =
		(fUpperLength * fUpperLength - fLowerLength * fLowerLength +
			fTargetDistance * fTargetDistance) /
		(2.f * fTargetDistance);
	const _float fBendHeight = std::sqrt(std::max(
		0.f, fUpperLength * fUpperLength - fAlong * fAlong));
	const _vector vDesiredKnee =
		vHip + vTargetDirection * fAlong + vBendDirection * fBendHeight;

	if (!RotateBoneToward(
		*pModel, Leg.iUpperLeg,
		vKnee - vHip, vDesiredKnee - vHip,
		GroundState.fWeight, LocalBoneMatrices, CombinedPose))
	{
		return false;
	}

	BuildCombinedPose(*pModel, LocalBoneMatrices, CombinedPose);
	const _vector vUpdatedKnee = GetTranslation(
		CombinedPose[static_cast<size_t>(Leg.iLowerLeg)]);
	const _vector vUpdatedAnkle = GetTranslation(
		CombinedPose[static_cast<size_t>(Leg.iFoot)]);
	RotateBoneToward(
		*pModel, Leg.iLowerLeg,
		vUpdatedAnkle - vUpdatedKnee, vTarget - vUpdatedKnee,
		GroundState.fWeight, LocalBoneMatrices, CombinedPose);

	BuildCombinedPose(*pModel, LocalBoneMatrices, CombinedPose);
	const _vector vWorldUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	const _vector vCharacterUpModel = XMVector3Normalize(
		XMVector3TransformNormal(vWorldUp, matInverseWorld));
	const _vector vGroundNormalModel = XMVector3Normalize(
		XMVector3TransformNormal(
			XMLoadFloat3(&GroundState.vGroundNormal), matInverseWorld));
	int32_t& iFootUpAxis = Leg.iFoot == m_tLeftLegIndices.iFoot
		? m_iLeftFootUpAxis : m_iRightFootUpAxis;
	_float& fFootUpSign = Leg.iFoot == m_tLeftLegIndices.iFoot
		? m_fLeftFootUpSign : m_fRightFootUpSign;
	const _matrix matFootCombined = XMLoadFloat4x4(
		&CombinedPose[static_cast<size_t>(Leg.iFoot)]);
	if (iFootUpAxis < 0)
	{
		_float fBestAlignment = -1.f;
		for (int32_t iAxis = 0; iAxis < 3; ++iAxis)
		{
			const _vector vAxis = XMVector3Normalize(matFootCombined.r[iAxis]);
			const _float fDot = XMVectorGetX(
				XMVector3Dot(vAxis, vCharacterUpModel));
			const _float fAlignment = std::abs(fDot);
			if (fAlignment > fBestAlignment)
			{
				fBestAlignment = fAlignment;
				iFootUpAxis = iAxis;
				fFootUpSign = fDot >= 0.f ? 1.f : -1.f;
			}
		}
	}
	const _vector vCurrentFootUpModel = XMVector3Normalize(
		matFootCombined.r[iFootUpAxis] * fFootUpSign);
	_vector vLimitedGroundNormalModel = vGroundNormalModel;
	const _float fAngle = std::acos(std::clamp(
		XMVectorGetX(XMVector3Dot(
			vCurrentFootUpModel, vGroundNormalModel)), -1.f, 1.f));
	const _float fMaxAngle = XMConvertToRadians(m_fMaxFootSlopeDegrees);
	if (fAngle > fMaxAngle && fAngle > FOOT_IK_EPSILON)
	{
		vLimitedGroundNormalModel = XMVector3Normalize(XMVectorLerp(
			vCurrentFootUpModel, vGroundNormalModel, fMaxAngle / fAngle));
	}
	RotateBoneToward(
		*pModel, Leg.iFoot,
		vCurrentFootUpModel,
		vLimitedGroundNormalModel,
		GroundState.fWeight, LocalBoneMatrices, CombinedPose);
	return true;
}

void CComFootIK::UpdateDebugLegState(
	const CComModelInstance& ModelInstance,
	const LEG_BONE_INDICES& Leg,
	const std::vector<_float4x4>& CombinedBoneMatrices,
	DEBUG_LEG_STATE& OutState)
{
	OutState.bValid = false;
	const auto pModel = ModelInstance.GetModel();
	if (!pModel || !Leg.IsValid() || !m_pGameObject ||
		CombinedBoneMatrices.size() != pModel->GetBones().size())
	{
		return;
	}

	const _matrix matOwnerWorld =
		m_pGameObject->GetTransform().GetLoadedCombinedWorldMatrix();
	XMStoreFloat3(&OutState.vHipWorld, XMVector3TransformCoord(
		GetTranslation(CombinedBoneMatrices[static_cast<size_t>(Leg.iUpperLeg)]),
		matOwnerWorld));
	XMStoreFloat3(&OutState.vKneeWorld, XMVector3TransformCoord(
		GetTranslation(CombinedBoneMatrices[static_cast<size_t>(Leg.iLowerLeg)]),
		matOwnerWorld));
	XMStoreFloat3(&OutState.vFootWorld, XMVector3TransformCoord(
		GetTranslation(CombinedBoneMatrices[static_cast<size_t>(Leg.iFoot)]),
		matOwnerWorld));
	if (OutState.bHasSolveTarget)
	{
		OutState.fSolveError = XMVectorGetX(XMVector3Length(
			XMLoadFloat3(&OutState.vFootWorld) -
			XMLoadFloat3(&OutState.vSolveTargetWorld)));
	}
	OutState.bValid = true;
}

_bool CComFootIK::SampleGround(
	const _float3& vFootWorldPosition,
	FOOT_GROUND_STATE& OutState) const
{
	OutState.bHit = false;
	OutState.hGroundObject = {};
	OutState.vGroundNormal = { 0.f, 1.f, 0.f };
	OutState.fDistance = 0.f;

	auto* pPhysXManager = CGameInstance::Get().GetPhysXManager();
	if (!pPhysXManager || m_fTraceDistance <= 0.f)
		return false;

	PX_RAYCAST_DESC RayDesc{};
	RayDesc.vOrigin = {
		vFootWorldPosition.x,
		vFootWorldPosition.y + m_fTraceStartHeight,
		vFootWorldPosition.z };
	RayDesc.vDirection = { 0.f, -1.f, 0.f };
	RayDesc.fMaxDistance = m_fTraceStartHeight + m_fTraceDistance;
	RayDesc.tFilter.iQueryMask = m_iGroundQueryMask;
	RayDesc.tFilter.bQueryStatic = true;
	RayDesc.tFilter.bQueryDynamic = true;
	RayDesc.tFilter.bIncludeTrigger = false;
	if (m_pGameObject)
		RayDesc.tFilter.hIgnoreGameObject = m_pGameObject->GetHandle();

	PX_RAYCAST_RESULT Hit{};
	if (!pPhysXManager->RayCast(RayDesc, Hit) || !Hit.bHit)
		return false;

	const _float fVerticalOffset = Hit.vHitpos.y - vFootWorldPosition.y;
	if (std::abs(fVerticalOffset) > m_fMaxStepHeight)
		return false;

	OutState.bHit = true;
	OutState.bHasTarget = true;
	OutState.vTargetWorldPosition = {
		Hit.vHitpos.x,
		Hit.vHitpos.y + m_fFootHeight,
		Hit.vHitpos.z };
	OutState.vGroundNormal = Hit.vHitNormal;
	OutState.fDistance = Hit.fDistance;
	OutState.hGroundObject = Hit.hGameObject;
	return true;
}

void CComFootIK::UpdateStateWeight(
	FOOT_GROUND_STATE& State,
	_float fTimeDelta) const
{
	const _float fTargetWeight = State.bHit && m_bEnabled ? 1.f : 0.f;
	if (fTimeDelta <= 0.f || m_fBlendSpeed <= 0.f)
	{
		State.fWeight = fTargetWeight;
		return;
	}

	const _float fBlendAlpha = 1.f - std::exp(-m_fBlendSpeed * fTimeDelta);
	State.fWeight = std::lerp(State.fWeight, fTargetWeight, fBlendAlpha);
	if (std::abs(State.fWeight - fTargetWeight) <= 0.001f)
		State.fWeight = fTargetWeight;
}

CComFootIK::LEG_BONE_INDICES CComFootIK::ResolveLegIndices(
	const CComModelInstance& ModelInstance,
	const LEG_BONE_NAMES& Names)
{
	LEG_BONE_INDICES Result{};
	const auto pModel = ModelInstance.GetModel();
	if (!pModel)
		return Result;

	const auto FindBone = [&pModel](const std::string& sName)
	{
		return sName.empty() ? -1 : pModel->Get_BoneIndex(sName.c_str());
	};
	Result.iUpperLeg = FindBone(Names.sUpperLeg);
	Result.iLowerLeg = FindBone(Names.sLowerLeg);
	Result.iFoot = FindBone(Names.sFoot);
	Result.iToe = FindBone(Names.sToe);
	return Result;
}

void CComFootIK::UpdateGUI()
{
	CComponent::UpdateGUI();
	ImGui::Checkbox("Enable Foot IK", &m_bEnabled);
	ImGui::SameLine();
	const _bool bSkeletonReady = HasValidSkeleton();
	ImGui::TextColored(
		bSkeletonReady ? ImVec4{ 0.2f, 1.f, 0.3f, 1.f }
			: ImVec4{ 1.f, 0.25f, 0.2f, 1.f },
		bSkeletonReady ? "SKELETON READY" : "SKELETON MISSING");

	if (ImGui::CollapsingHeader("1. Live Status", ImGuiTreeNodeFlags_DefaultOpen))
	{
		const auto DrawFootStatus = [](const char* pLabel,
			const FOOT_GROUND_STATE& State)
		{
			ImGui::TextColored(
				State.bAnimationLifting ? ImVec4{ 0.25f, 0.7f, 1.f, 1.f }
				: State.bHit ? ImVec4{ 0.2f, 1.f, 0.3f, 1.f }
					: ImVec4{ 1.f, 0.35f, 0.2f, 1.f },
				"%s: %s", pLabel,
				State.bAnimationLifting ? "ANIMATION LIFT" :
				State.bHit ? "GROUND HIT" : "MISS");
			ImGui::SameLine();
			ImGui::Text("Weight %.3f | Distance %.3f | Vy %.3f",
				State.fWeight, State.fDistance,
				State.fAnimatedVerticalSpeed);
		};
		DrawFootStatus("LEFT ", m_tLeftFootState);
		DrawFootStatus("RIGHT", m_tRightFootState);
		ImGui::Text("Final Foot Error  L %.4f m | R %.4f m",
			m_tLeftDebugState.fSolveError,
			m_tRightDebugState.fSolveError);
		ImGui::TextDisabled(
			"Yellow=actual solve target, Orange=raw ground target, Green=final foot");
	}

	if (ImGui::CollapsingHeader("2. Ground Trace", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat("Trace Start Height", &m_fTraceStartHeight,
			0.01f, 0.f, 2.f, "%.2f m");
		ImGui::TextDisabled("Raise this when stairs or edges are missed. Start: 0.35");
		ImGui::DragFloat("Trace Down Distance", &m_fTraceDistance,
			0.01f, 0.f, 3.f, "%.2f m");
		ImGui::TextDisabled("Lower it if the ray catches a floor below. Start: 0.80");
		ImGui::DragFloat("Max Step Height", &m_fMaxStepHeight,
			0.01f, 0.f, 2.f, "%.2f m");
		ImGui::TextDisabled("Rejects ground farther from the animated foot. Start: 0.45");
	}

	if (ImGui::CollapsingHeader("3. Foot Placement", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat("Sole Height Offset", &m_fFootHeight,
			0.0025f, 0.f, 0.5f, "%.3f m");
		ImGui::TextDisabled("Foot buried: increase. Foot floating: decrease. Start: 0.040");
		ImGui::SliderFloat("Max Foot Slope", &m_fMaxFootSlopeDegrees,
			0.f, 89.f, "%.1f deg");
		ImGui::TextDisabled("Use 25-35 degrees if the ankle bends too much.");
		ImGui::SliderFloat("Max Leg Extension", &m_fMaxExtensionRatio,
			0.9f, 0.9999f, "%.4f");
		ImGui::TextDisabled("Lower to 0.98-0.99 if the knee becomes too straight.");
		ImGui::DragFloat("Blend Speed", &m_fBlendSpeed,
			0.1f, 0.f, 50.f, "%.1f");
		ImGui::TextDisabled("Foot jitter: lower to 6-10. Slow response: raise.");
		ImGui::DragFloat("Max Pelvis Drop", &m_fMaxPelvisDrop,
			0.01f, 0.f, 1.5f, "%.2f m");
		ImGui::DragFloat("Pelvis Blend Speed", &m_fPelvisBlendSpeed,
			0.1f, 0.f, 50.f, "%.1f");
		ImGui::Text("Current Pelvis Offset Y: %.3f m",
			m_fCurrentPelvisOffsetY);
		ImGui::Text("Detected Foot Up Axis  L %d (%+.0f) | R %d (%+.0f)",
			m_iLeftFootUpAxis, m_fLeftFootUpSign,
			m_iRightFootUpAxis, m_fRightFootUpSign);
		ImGui::DragFloat("Lift Release Speed", &m_fLiftReleaseSpeed,
			0.01f, 0.f, 3.f, "%.2f m/s");
		ImGui::TextDisabled("Rising faster than this releases IK back to animation.");
	}

	if (ImGui::CollapsingHeader("4. Debug View", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Checkbox("Draw IK Debug", &m_bDebugDraw);
		ImGui::Checkbox("Draw Leg Chains", &m_bDebugDrawSkeleton);
		ImGui::Checkbox("Depth Test", &m_bDebugDepthTest);
		ImGui::TextDisabled("Yellow=solve target | Orange=raw target | Green=final foot");
	}

	if (ImGui::Button("Preset: Flat Ground"))
	{
		m_fTraceStartHeight = 0.3f;
		m_fTraceDistance = 0.65f;
		m_fMaxStepHeight = 0.35f;
		m_fFootHeight = 0.04f;
		m_fBlendSpeed = 10.f;
		m_fMaxExtensionRatio = 0.99f;
		m_fMaxFootSlopeDegrees = 30.f;
	}
	ImGui::SameLine();
	if (ImGui::Button("Preset: Stairs"))
	{
		m_fTraceStartHeight = 0.5f;
		m_fTraceDistance = 1.f;
		m_fMaxStepHeight = 0.6f;
		m_fFootHeight = 0.04f;
		m_fBlendSpeed = 8.f;
		m_fMaxExtensionRatio = 0.985f;
		m_fMaxFootSlopeDegrees = 40.f;
	}
}

void CComFootIK::DrawDebugVisualization() const
{
	auto* pDebug = CGameInstance::Get().GetDbgLineRender();
	if (!pDebug)
		return;

	const _float4 PreviousColor = pDebug->GetColor();
	const DBG_LINE_DEPTH_MODE PreviousDepth = pDebug->GetDepthMode();
	pDebug->SetDepthTest(m_bDebugDepthTest);

	const auto DrawFoot = [this, pDebug](
		const FOOT_GROUND_STATE& State,
		const DEBUG_LEG_STATE& LegDebug,
		const _float4& LegColor)
	{
		const _float3 vRayStart{
			State.vAnimatedWorldPosition.x,
			State.vAnimatedWorldPosition.y + m_fTraceStartHeight,
			State.vAnimatedWorldPosition.z };
		const _float3 vRayEnd{
			vRayStart.x,
			vRayStart.y - (m_fTraceStartHeight + m_fTraceDistance),
			vRayStart.z };
		pDebug->AddLine(vRayStart, vRayEnd,
			State.bHit ? _float4{ 0.2f, 1.f, 0.2f, 1.f }
				: _float4{ 1.f, 0.15f, 0.1f, 1.f });
		pDebug->SetColor({ 1.f, 1.f, 1.f, 1.f });
		pDebug->AddCross(State.vAnimatedWorldPosition, 0.035f);
		if (State.bHit)
		{
			pDebug->SetColor({ 1.f, 0.4f, 0.05f, 1.f });
			pDebug->AddCross(State.vTargetWorldPosition, 0.075f);
			const _float3 vNormalEnd{
				State.vTargetWorldPosition.x + State.vGroundNormal.x * 0.3f,
				State.vTargetWorldPosition.y + State.vGroundNormal.y * 0.3f,
				State.vTargetWorldPosition.z + State.vGroundNormal.z * 0.3f };
			pDebug->AddLine(State.vTargetWorldPosition, vNormalEnd,
				{ 0.1f, 0.9f, 1.f, 1.f });
		}
		if (m_bDebugDrawSkeleton && LegDebug.bValid)
		{
			if (LegDebug.bHasSolveTarget)
			{
				pDebug->SetColor({ 1.f, 0.85f, 0.1f, 1.f });
				pDebug->AddCross(LegDebug.vSolveTargetWorld, 0.09f);
				pDebug->AddLine(
					LegDebug.vFootWorld,
					LegDebug.vSolveTargetWorld,
					{ 1.f, 0.85f, 0.1f, 1.f });
			}
			pDebug->SetColor({ 0.15f, 1.f, 0.25f, 1.f });
			pDebug->AddCross(LegDebug.vFootWorld, 0.065f);
			pDebug->AddLine(LegDebug.vHipWorld, LegDebug.vKneeWorld, LegColor);
			pDebug->AddLine(LegDebug.vKneeWorld, LegDebug.vFootWorld, LegColor);
			pDebug->SetColor(LegColor);
			pDebug->AddCross(LegDebug.vKneeWorld, 0.045f);
		}
	};

	DrawFoot(m_tLeftFootState, m_tLeftDebugState,
		{ 0.2f, 0.45f, 1.f, 1.f });
	DrawFoot(m_tRightFootState, m_tRightDebugState,
		{ 1.f, 0.2f, 0.85f, 1.f });
	pDebug->SetColor(PreviousColor);
	pDebug->SetDepthMode(PreviousDepth);
}

UPtr<CComFootIK> CComFootIK::Create()
{
	auto pInstance = ToUPtr(new CComFootIK{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CComFootIK");
		return nullptr;
	}
	return pInstance;
}

UPtr<CPrototype> CComFootIK::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CComFootIK{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CComFootIK");
		return nullptr;
	}
	return pInstance;
}

void CComFootIK::Free()
{
	CComponent::Free();
}
