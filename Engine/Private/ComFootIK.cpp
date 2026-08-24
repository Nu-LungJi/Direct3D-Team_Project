#include "pch.h"
#include "ComFootIK.h"

#include "ComModelInstance.h"
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
	m_tLeftFootState.vAnimatedWorldPosition = vLeftFootWorldPosition;
	m_tRightFootState.vAnimatedWorldPosition = vRightFootWorldPosition;

	if (m_bEnabled)
	{
		SampleGround(vLeftFootWorldPosition, m_tLeftFootState);
		SampleGround(vRightFootWorldPosition, m_tRightFootState);
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
	std::vector<_float4x4>& LocalBoneMatrices)
{
	if (!m_bEnabled || !HasValidSkeleton())
		return false;

	const auto pModel = ModelInstance.GetModel();
	if (!pModel || LocalBoneMatrices.size() != pModel->GetBones().size())
		return false;

	_bool bApplied{};
	bApplied |= SolveLeg(
		ModelInstance, m_tLeftLegIndices,
		m_tLeftFootState, LocalBoneMatrices);
	bApplied |= SolveLeg(
		ModelInstance, m_tRightLegIndices,
		m_tRightFootState, LocalBoneMatrices);
	return bApplied;
}

_bool CComFootIK::SolveLeg(
	const CComModelInstance& ModelInstance,
	const LEG_BONE_INDICES& Leg,
	const FOOT_GROUND_STATE& GroundState,
	std::vector<_float4x4>& LocalBoneMatrices)
{
	if (!Leg.IsValid() || !GroundState.bHit || GroundState.fWeight <= 0.f ||
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
	const _matrix matWorld =
		m_pGameObject->GetTransform().GetLoadedCombinedWorldMatrix();
	const _vector vCurrentUp = XMVector3Normalize(XMVector3TransformNormal(
		XMLoadFloat4x4(&CombinedPose[static_cast<size_t>(Leg.iFoot)]).r[1],
		matWorld));
	_vector vGroundNormal = XMVector3Normalize(
		XMLoadFloat3(&GroundState.vGroundNormal));
	const _float fAngle = std::acos(std::clamp(
		XMVectorGetX(XMVector3Dot(vCurrentUp, vGroundNormal)), -1.f, 1.f));
	const _float fMaxAngle = XMConvertToRadians(m_fMaxFootSlopeDegrees);
	if (fAngle > fMaxAngle && fAngle > FOOT_IK_EPSILON)
	{
		const _float fRatio = fMaxAngle / fAngle;
		vGroundNormal = XMVector3Normalize(XMVectorLerp(
			vCurrentUp, vGroundNormal, fRatio));
	}
	const _vector vGroundNormalModel = XMVector3TransformNormal(
		vGroundNormal, matInverseWorld);
	RotateBoneToward(
		*pModel, Leg.iFoot,
		XMLoadFloat4x4(&CombinedPose[static_cast<size_t>(Leg.iFoot)]).r[1],
		vGroundNormalModel,
		GroundState.fWeight, LocalBoneMatrices, CombinedPose);
	return true;
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
	ImGui::Checkbox("Enabled", &m_bEnabled);
	ImGui::DragFloat("Trace Start Height", &m_fTraceStartHeight, 0.01f, 0.f, 2.f);
	ImGui::DragFloat("Trace Distance", &m_fTraceDistance, 0.01f, 0.f, 3.f);
	ImGui::DragFloat("Foot Height", &m_fFootHeight, 0.005f, 0.f, 0.5f);
	ImGui::DragFloat("Blend Speed", &m_fBlendSpeed, 0.1f, 0.f, 50.f);
	ImGui::DragFloat("Max Step Height", &m_fMaxStepHeight, 0.01f, 0.f, 2.f);
	ImGui::SliderFloat("Max Extension Ratio", &m_fMaxExtensionRatio, 0.9f, 0.9999f, "%.4f");
	ImGui::SliderFloat("Max Foot Slope", &m_fMaxFootSlopeDegrees, 0.f, 89.f, "%.1f deg");
	ImGui::Text("Skeleton: %s", HasValidSkeleton() ? "ready" : "not bound");
	ImGui::Text("Pelvis / Left / Right: %d / %s / %s",
		m_iPelvisBone,
		m_tLeftLegIndices.IsValid() ? "ready" : "missing",
		m_tRightLegIndices.IsValid() ? "ready" : "missing");
	ImGui::Text("Left Ground: %s (weight %.3f)",
		m_tLeftFootState.bHit ? "hit" : "miss", m_tLeftFootState.fWeight);
	ImGui::Text("Right Ground: %s (weight %.3f)",
		m_tRightFootState.bHit ? "hit" : "miss", m_tRightFootState.fWeight);
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
