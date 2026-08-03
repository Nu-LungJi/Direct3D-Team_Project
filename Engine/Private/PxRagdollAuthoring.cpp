#include "pch.h"
#include "PxRagdollAuthoring.h"

#include "ResModel.h"
#include "ResModelBone.h"

NS_USING(Engine)

namespace
{
	_bool IsFinite(_float fValue)
	{
		return std::isfinite(fValue);
	}

	_bool IsFinite(const _float3& vValue)
	{
		return IsFinite(vValue.x) &&
			IsFinite(vValue.y) &&
			IsFinite(vValue.z);
	}

	_bool TryInverse(
		FXMMATRIX Matrix,
		_matrix& OutInverse)
	{
		_vector vDeterminant{};
		OutInverse =
			XMMatrixInverse(&vDeterminant, Matrix);
		const _float fDeterminant =
			XMVectorGetX(vDeterminant);
		return IsFinite(fDeterminant) &&
			std::abs(fDeterminant) > FLT_EPSILON;
	}
}

_bool CPxRagdollAuthoring::Initialize(
	CResModel& Model)
{
	m_BindPoses.clear();
	m_BoneIndexByName.clear();

	const auto& Bones = Model.GetBones();
	if (Bones.empty())
		return false;

	m_BindPoses.resize(Bones.size());
	m_BoneIndexByName.reserve(Bones.size());
	std::vector<uint8_t> BindPoseStates(
		Bones.size(),
		0u);
	const _matrix ModelPreTransform =
		XMLoadFloat4x4(
			&Model.Get_PreTransformMatrix());

	for (size_t iBone = 0;
		iBone < Bones.size();
		++iBone)
	{
		if (!Bones[iBone] ||
			!m_BoneIndexByName.emplace(
				Bones[iBone]->GetBoneName(),
				iBone).second)
		{
			m_BindPoses.clear();
			m_BoneIndexByName.clear();
			return false;
		}
	}

	auto BuildBindPose =
		[&](auto&& Self, size_t iBone) -> _bool
		{
			if (iBone >= Bones.size() ||
				!Bones[iBone])
			{
				return false;
			}

			if (BindPoseStates[iBone] == 2u)
				return true;
			if (BindPoseStates[iBone] == 1u)
				return false;

			BindPoseStates[iBone] = 1u;
			const int32_t iParentBone =
				Bones[iBone]->
					GetParendBoneIndex();
			_matrix CombinedBindPose{};
			if (iParentBone < 0)
			{
				CombinedBindPose =
					Bones[iBone]->
						Get_TransformationMatrix() *
					ModelPreTransform;
			}
			else
			{
				const size_t iParentIndex =
					static_cast<size_t>(
						iParentBone);
				if (iParentIndex >= Bones.size() ||
					!Self(Self, iParentIndex))
				{
					return false;
				}

				CombinedBindPose =
					Bones[iBone]->
						Get_TransformationMatrix() *
					XMLoadFloat4x4(
						&m_BindPoses[
							iParentIndex]);
			}

			XMStoreFloat4x4(
				&m_BindPoses[iBone],
				CombinedBindPose);
			BindPoseStates[iBone] = 2u;
			return true;
		};

	for (size_t iBone = 0;
		iBone < Bones.size();
		++iBone)
	{
		if (!BuildBindPose(
			BuildBindPose,
			iBone))
		{
			m_BindPoses.clear();
			m_BoneIndexByName.clear();
			return false;
		}
	}

	return true;
}

_bool CPxRagdollAuthoring::GetBoneBindPose(
	size_t iBoneIndex,
	_float4x4& OutBindPose) const
{
	if (iBoneIndex >= m_BindPoses.size())
		return false;

	OutBindPose = m_BindPoses[iBoneIndex];
	return true;
}

_bool CPxRagdollAuthoring::GetRigidBindPose(
	const _char* pBoneName,
	_float4x4& OutRigidBindPose) const
{
	if (!pBoneName)
		return false;

	const auto BoneIter =
		m_BoneIndexByName.find(pBoneName);
	if (BoneIter == m_BoneIndexByName.end() ||
		BoneIter->second >= m_BindPoses.size())
	{
		return false;
	}

	_vector vScale{};
	_vector vRotation{};
	_vector vTranslation{};
	if (!XMMatrixDecompose(
		&vScale,
		&vRotation,
		&vTranslation,
		XMLoadFloat4x4(
			&m_BindPoses[BoneIter->second])))
	{
		return false;
	}

	XMStoreFloat4x4(
		&OutRigidBindPose,
		XMMatrixRotationQuaternion(
			XMQuaternionNormalize(vRotation)) *
		XMMatrixTranslationFromVector(
			vTranslation));
	return true;
}

_bool CPxRagdollAuthoring::
GetBonePositionInBodySpace(
	const _char* pBodyBoneName,
	const _char* pTargetBoneName,
	_float3& vOutPosition) const
{
	_float4x4 BodyRigidBindPose{};
	_float4x4 TargetRigidBindPose{};
	if (!GetRigidBindPose(
			pBodyBoneName,
			BodyRigidBindPose) ||
		!GetRigidBindPose(
			pTargetBoneName,
			TargetRigidBindPose))
	{
		return false;
	}

	_matrix InverseBodyBindPose{};
	if (!TryInverse(
		XMLoadFloat4x4(
			&BodyRigidBindPose),
		InverseBodyBindPose))
	{
		return false;
	}

	const _matrix TargetInBodySpace =
		XMLoadFloat4x4(
			&TargetRigidBindPose) *
		InverseBodyBindPose;
	_vector vRelativeScale{};
	_vector vRelativeRotation{};
	_vector vRelativePosition{};
	if (!XMMatrixDecompose(
		&vRelativeScale,
		&vRelativeRotation,
		&vRelativePosition,
		TargetInBodySpace))
	{
		return false;
	}

	XMStoreFloat3(
		&vOutPosition,
		vRelativePosition);
	return IsFinite(vOutPosition);
}

PX_RAGDOLL_SHAPE_DESC*
CPxRagdollAuthoring::FindSingleBodyShape(
	PX_RAGDOLL_DESC& tRagdoll,
	const _char* pBodyName) const
{
	if (!pBodyName)
		return nullptr;

	auto BodyIter = std::find_if(
		tRagdoll.Bodies.begin(),
		tRagdoll.Bodies.end(),
		[pBodyName](
			const PX_RAGDOLL_BODY_DESC& tBody)
		{
			return tBody.sBodyName == pBodyName;
		});
	if (BodyIter == tRagdoll.Bodies.end() ||
		BodyIter->Shapes.size() != 1)
	{
		return nullptr;
	}

	return &BodyIter->Shapes.front();
}

_bool CPxRagdollAuthoring::MakeFromToRotation(
	const _float3& vFromAxis,
	const _float3& vDirection,
	_float4& vOutRotation)
{
	_vector vFrom =
		XMLoadFloat3(&vFromAxis);
	_vector vTo =
		XMLoadFloat3(&vDirection);
	const _float fFromLength =
		XMVectorGetX(
			XMVector3Length(vFrom));
	const _float fDirectionLength =
		XMVectorGetX(
			XMVector3Length(vTo));
	if (!IsFinite(fFromLength) ||
		!IsFinite(fDirectionLength) ||
		fFromLength <= FLT_EPSILON ||
		fDirectionLength <= FLT_EPSILON)
	{
		return false;
	}

	vFrom = XMVector3Normalize(vFrom);
	vTo = XMVector3Normalize(vTo);
	const _float fAxisDot =
		std::clamp(
			XMVectorGetX(
				XMVector3Dot(vFrom, vTo)),
			-1.f,
			1.f);

	_vector vRotation{};
	if (fAxisDot >= 0.9999f)
	{
		vRotation = XMQuaternionIdentity();
	}
	else if (fAxisDot <= -0.9999f)
	{
		_vector vFallbackAxis =
			XMVector3Cross(
				vFrom,
				XMVectorSet(
					1.f,
					0.f,
					0.f,
					0.f));
		if (XMVectorGetX(
			XMVector3LengthSq(
				vFallbackAxis)) <=
			FLT_EPSILON)
		{
			vFallbackAxis =
				XMVector3Cross(
					vFrom,
					XMVectorSet(
						0.f,
						1.f,
						0.f,
						0.f));
		}

		vRotation =
			XMQuaternionRotationAxis(
				XMVector3Normalize(
					vFallbackAxis),
				XM_PI);
	}
	else
	{
		const _vector vRotationAxis =
			XMVector3Normalize(
				XMVector3Cross(
					vFrom,
					vTo));
		vRotation =
			XMQuaternionRotationAxis(
				vRotationAxis,
				std::acos(fAxisDot));
	}

	XMStoreFloat4(
		&vOutRotation,
		XMQuaternionNormalize(vRotation));
	return true;
}

_bool CPxRagdollAuthoring::
FitCapsuleBodyToChild(
	PX_RAGDOLL_DESC& tRagdoll,
	const _char* pBodyName,
	const _char* pBodyBoneName,
	const _char* pChildBoneName,
	_float fRadius) const
{
	auto* pShape =
		FindSingleBodyShape(
			tRagdoll,
			pBodyName);
	_float3 vChildPosition{};
	if (!pShape ||
		!IsFinite(fRadius) ||
		fRadius <= 0.f ||
		!GetBonePositionInBodySpace(
			pBodyBoneName,
			pChildBoneName,
			vChildPosition))
	{
		return false;
	}

	const _vector vRelativePosition =
		XMLoadFloat3(&vChildPosition);
	const _float fLength =
		XMVectorGetX(
			XMVector3Length(
				vRelativePosition));
	_float4 vCapsuleRotation{};
	const _float3 vCapsuleAxis{
		0.f,
		1.f,
		0.f
	};
	if (!IsFinite(fLength) ||
		fLength <= FLT_EPSILON ||
		!MakeFromToRotation(
			vCapsuleAxis,
			vChildPosition,
			vCapsuleRotation))
	{
		return false;
	}

	pShape->eType =
		PX_RAGDOLL_SHAPE_TYPE::CAPSULE;
	pShape->fRadius = fRadius;
	pShape->fHalfHeight = std::max(
		fLength * 0.5f - fRadius,
		0.01f);
	XMStoreFloat3(
		&pShape->vLocalPosition,
		vRelativePosition * 0.5f);
	pShape->vLocalRotation =
		vCapsuleRotation;
	return true;
}

_bool CPxRagdollAuthoring::FitPelvisBox(
	PX_RAGDOLL_DESC& tRagdoll,
	const _char* pBodyName,
	const _char* pBodyBoneName,
	const _char* pLeftLegBoneName,
	const _char* pRightLegBoneName,
	const _char* pSpineBoneName) const
{
	auto* pShape =
		FindSingleBodyShape(
			tRagdoll,
			pBodyName);
	_float3 vLeftHipPosition{};
	_float3 vRightHipPosition{};
	_float3 vSpinePosition{};
	if (!pShape ||
		!GetBonePositionInBodySpace(
			pBodyBoneName,
			pLeftLegBoneName,
			vLeftHipPosition) ||
		!GetBonePositionInBodySpace(
			pBodyBoneName,
			pRightLegBoneName,
			vRightHipPosition) ||
		!GetBonePositionInBodySpace(
			pBodyBoneName,
			pSpineBoneName,
			vSpinePosition))
	{
		return false;
	}

	const _vector vLeftHip =
		XMLoadFloat3(&vLeftHipPosition);
	const _vector vRightHip =
		XMLoadFloat3(&vRightHipPosition);
	const _float fHipSpan =
		XMVectorGetX(
			XMVector3Length(
				vRightHip - vLeftHip));
	const _float fSpineDistance =
		XMVectorGetX(
			XMVector3Length(
				XMLoadFloat3(
					&vSpinePosition)));
	if (!IsFinite(fHipSpan) ||
		!IsFinite(fSpineDistance) ||
		fHipSpan <= FLT_EPSILON)
	{
		return false;
	}

	pShape->sName =
		std::string{ pBodyName } +
		"PelvisBox";
	pShape->eType =
		PX_RAGDOLL_SHAPE_TYPE::BOX;
	pShape->vHalfExtents = {
		std::max(fHipSpan * 0.55f, 0.18f),
		std::max(fSpineDistance * 0.22f, 0.14f),
		std::max(fHipSpan * 0.28f, 0.12f)
	};
	XMStoreFloat3(
		&pShape->vLocalPosition,
		(vLeftHip + vRightHip) * 0.5f);
	pShape->vLocalRotation =
		{ 0.f, 0.f, 0.f, 1.f };
	return true;
}

_bool CPxRagdollAuthoring::FitChestBox(
	PX_RAGDOLL_DESC& tRagdoll,
	const _char* pBodyName,
	const _char* pBodyBoneName,
	const _char* pLeftArmBoneName,
	const _char* pRightArmBoneName,
	const _char* pHeadBoneName) const
{
	auto* pShape =
		FindSingleBodyShape(
			tRagdoll,
			pBodyName);
	_float3 vLeftShoulderPosition{};
	_float3 vRightShoulderPosition{};
	_float3 vHeadPosition{};
	if (!pShape ||
		!GetBonePositionInBodySpace(
			pBodyBoneName,
			pLeftArmBoneName,
			vLeftShoulderPosition) ||
		!GetBonePositionInBodySpace(
			pBodyBoneName,
			pRightArmBoneName,
			vRightShoulderPosition) ||
		!GetBonePositionInBodySpace(
			pBodyBoneName,
			pHeadBoneName,
			vHeadPosition))
	{
		return false;
	}

	const _vector vLeftShoulder =
		XMLoadFloat3(
			&vLeftShoulderPosition);
	const _vector vRightShoulder =
		XMLoadFloat3(
			&vRightShoulderPosition);
	const _float fShoulderSpan =
		XMVectorGetX(
			XMVector3Length(
				vRightShoulder -
				vLeftShoulder));
	const _float fHeadDistance =
		XMVectorGetX(
			XMVector3Length(
				XMLoadFloat3(
					&vHeadPosition)));
	if (!IsFinite(fShoulderSpan) ||
		!IsFinite(fHeadDistance) ||
		fShoulderSpan <= FLT_EPSILON)
	{
		return false;
	}

	pShape->sName =
		std::string{ pBodyName } +
		"ChestBox";
	pShape->eType =
		PX_RAGDOLL_SHAPE_TYPE::BOX;
	pShape->vHalfExtents = {
		std::max(
			fShoulderSpan * 0.5f,
			0.25f),
		std::max(
			fHeadDistance * 0.3f,
			0.2f),
		std::max(
			fShoulderSpan * 0.22f,
			0.15f)
	};
	XMStoreFloat3(
		&pShape->vLocalPosition,
		(vLeftShoulder +
			vRightShoulder) *
			0.25f);
	pShape->vLocalRotation =
		{ 0.f, 0.f, 0.f, 1.f };
	return true;
}

_bool CPxRagdollAuthoring::FitFootBox(
	PX_RAGDOLL_DESC& tRagdoll,
	const _char* pBodyName,
	const _char* pFootBoneName,
	const _char* pToeBoneName) const
{
	auto* pShape =
		FindSingleBodyShape(
			tRagdoll,
			pBodyName);
	_float3 vToePosition{};
	if (!pShape ||
		!GetBonePositionInBodySpace(
			pFootBoneName,
			pToeBoneName,
			vToePosition))
	{
		return false;
	}

	const _vector vToe =
		XMLoadFloat3(&vToePosition);
	const _float fFootLength =
		XMVectorGetX(
			XMVector3Length(vToe));
	const _float3 vBoxAxis{
		0.f,
		0.f,
		1.f
	};
	_float4 vFootRotation{};
	if (!IsFinite(fFootLength) ||
		fFootLength <= FLT_EPSILON ||
		!MakeFromToRotation(
			vBoxAxis,
			vToePosition,
			vFootRotation))
	{
		return false;
	}

	pShape->sName =
		std::string{ pBodyName } +
		"Box";
	pShape->eType =
		PX_RAGDOLL_SHAPE_TYPE::BOX;
	pShape->vHalfExtents = {
		0.12f,
		0.08f,
		std::max(
			fFootLength * 0.55f,
			0.16f)
	};
	XMStoreFloat3(
		&pShape->vLocalPosition,
		vToe * 0.5f);
	pShape->vLocalRotation =
		vFootRotation;
	return true;
}

_bool CPxRagdollAuthoring::
AddBindPoseD6Joint(
	PX_RAGDOLL_DESC& tRagdoll,
	const _char* pJointName,
	const _char* pParentBodyName,
	const _char* pParentBoneName,
	const _char* pChildBodyName,
	const _char* pChildBoneName,
	_float fTwistDegrees,
	_float fSwingYDegrees,
	_float fSwingZDegrees) const
{
	if (!pJointName ||
		!pParentBodyName ||
		!pChildBodyName ||
		!IsFinite(fTwistDegrees) ||
		!IsFinite(fSwingYDegrees) ||
		!IsFinite(fSwingZDegrees))
	{
		return false;
	}

	_float4x4 ParentRigidBindPose{};
	_float4x4 ChildRigidBindPose{};
	if (!GetRigidBindPose(
			pParentBoneName,
			ParentRigidBindPose) ||
		!GetRigidBindPose(
			pChildBoneName,
			ChildRigidBindPose))
	{
		return false;
	}

	_matrix InverseParentBindPose{};
	if (!TryInverse(
		XMLoadFloat4x4(
			&ParentRigidBindPose),
		InverseParentBindPose))
	{
		return false;
	}

	const _matrix ParentJointLocal =
		XMLoadFloat4x4(
			&ChildRigidBindPose) *
		InverseParentBindPose;
	_vector vJointScale{};
	_vector vJointRotation{};
	_vector vJointTranslation{};
	if (!XMMatrixDecompose(
		&vJointScale,
		&vJointRotation,
		&vJointTranslation,
		ParentJointLocal))
	{
		return false;
	}

	PX_RAGDOLL_D6_JOINT_DESC tJoint{};
	tJoint.sJointName = pJointName;
	tJoint.sParentBodyName =
		pParentBodyName;
	tJoint.sChildBodyName =
		pChildBodyName;
	XMStoreFloat3(
		&tJoint.vParentLocalPosition,
		vJointTranslation);
	XMStoreFloat4(
		&tJoint.vParentLocalRotation,
		XMQuaternionNormalize(
			vJointRotation));
	tJoint.fTwistLowerDegrees =
		-fTwistDegrees;
	tJoint.fTwistUpperDegrees =
		fTwistDegrees;
	tJoint.fSwingYDegrees =
		fSwingYDegrees;
	tJoint.fSwingZDegrees =
		fSwingZDegrees;
	tJoint.bCollisionEnabled = false;
	tJoint.bVisualizationEnabled = true;
	tRagdoll.Joints.emplace_back(
		std::move(tJoint));
	return true;
}

_bool CPxRagdollAuthoring::
ConfigureAnatomicalHinge(
	PX_RAGDOLL_DESC& tRagdoll,
	const _char* pJointName,
	const _char* pChildBoneName,
	const _char* pEndBoneName,
	_float3 vObjectBendDirection,
	_float fTwistLowerDegrees,
	_float fTwistUpperDegrees) const
{
	if (!pJointName ||
		!IsFinite(vObjectBendDirection) ||
		!IsFinite(fTwistLowerDegrees) ||
		!IsFinite(fTwistUpperDegrees) ||
		fTwistLowerDegrees >
			fTwistUpperDegrees)
	{
		return false;
	}

	auto JointIter = std::find_if(
		tRagdoll.Joints.begin(),
		tRagdoll.Joints.end(),
		[pJointName](
			const PX_RAGDOLL_D6_JOINT_DESC& tJoint)
		{
			return tJoint.sJointName ==
				pJointName;
		});
	if (JointIter == tRagdoll.Joints.end())
		return false;

	_float3 vSegmentDirectionFloat3{};
	if (!GetBonePositionInBodySpace(
		pChildBoneName,
		pEndBoneName,
		vSegmentDirectionFloat3))
	{
		return false;
	}

	_vector vSegmentDirection =
		XMLoadFloat3(
			&vSegmentDirectionFloat3);
	if (XMVectorGetX(
		XMVector3LengthSq(
			vSegmentDirection)) <=
		FLT_EPSILON)
	{
		return false;
	}
	vSegmentDirection =
		XMVector3Normalize(
			vSegmentDirection);

	_float4x4 ChildRigidBindPose{};
	if (!GetRigidBindPose(
		pChildBoneName,
		ChildRigidBindPose))
	{
		return false;
	}

	_vector vChildScale{};
	_vector vChildRotation{};
	_vector vChildTranslation{};
	if (!XMMatrixDecompose(
		&vChildScale,
		&vChildRotation,
		&vChildTranslation,
		XMLoadFloat4x4(
			&ChildRigidBindPose)))
	{
		return false;
	}

	const _matrix ChildRotation =
		XMMatrixRotationQuaternion(
			XMQuaternionNormalize(
				vChildRotation));
	const _matrix InverseChildRotation =
		XMMatrixTranspose(ChildRotation);
	_vector vBendDirection =
		XMVector3TransformNormal(
			XMLoadFloat3(
				&vObjectBendDirection),
			InverseChildRotation);
	vBendDirection -=
		vSegmentDirection *
		XMVectorGetX(
			XMVector3Dot(
				vBendDirection,
				vSegmentDirection));
	if (XMVectorGetX(
		XMVector3LengthSq(
			vBendDirection)) <=
		FLT_EPSILON)
	{
		return false;
	}
	vBendDirection =
		XMVector3Normalize(
			vBendDirection);

	_vector vHingeAxis =
		XMVector3Cross(
			vSegmentDirection,
			vBendDirection);
	if (XMVectorGetX(
		XMVector3LengthSq(
			vHingeAxis)) <=
		FLT_EPSILON)
	{
		return false;
	}
	vHingeAxis =
		XMVector3Normalize(vHingeAxis);
	const _vector vFrameZ =
		XMVector3Normalize(
			XMVector3Cross(
				vHingeAxis,
				vSegmentDirection));

	const _matrix ChildJointFrame =
		XMMatrixSet(
			XMVectorGetX(vHingeAxis),
			XMVectorGetY(vHingeAxis),
			XMVectorGetZ(vHingeAxis),
			0.f,
			XMVectorGetX(vSegmentDirection),
			XMVectorGetY(vSegmentDirection),
			XMVectorGetZ(vSegmentDirection),
			0.f,
			XMVectorGetX(vFrameZ),
			XMVectorGetY(vFrameZ),
			XMVectorGetZ(vFrameZ),
			0.f,
			0.f,
			0.f,
			0.f,
			1.f);
	const _matrix ParentJointRotation =
		ChildJointFrame *
		XMMatrixRotationQuaternion(
			XMLoadFloat4(
				&JointIter->
					vParentLocalRotation));

	XMStoreFloat4(
		&JointIter->vChildLocalRotation,
		XMQuaternionNormalize(
			XMQuaternionRotationMatrix(
				ChildJointFrame)));
	XMStoreFloat4(
		&JointIter->vParentLocalRotation,
		XMQuaternionNormalize(
			XMQuaternionRotationMatrix(
				ParentJointRotation)));
	JointIter->eTwistMotion =
		PX_RAGDOLL_D6_MOTION::LIMITED;
	JointIter->eSwingYMotion =
		PX_RAGDOLL_D6_MOTION::LOCKED;
	JointIter->eSwingZMotion =
		PX_RAGDOLL_D6_MOTION::LOCKED;
	JointIter->fTwistLowerDegrees =
		fTwistLowerDegrees;
	JointIter->fTwistUpperDegrees =
		fTwistUpperDegrees;
	JointIter->fSwingYDegrees = 0.f;
	JointIter->fSwingZDegrees = 0.f;
	// 팔꿈치와 무릎은 단단한 한계각을 사용하는 단일 축 힌지다.
	JointIter->fLimitStiffness = 0.f;
	JointIter->fLimitDamping = 0.f;
	return true;
}
