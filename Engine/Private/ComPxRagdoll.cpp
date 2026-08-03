#include "pch.h"
#include "ComPxRagdoll.h"

#include "GameInstance.h"
#include "GameObject.h"
#include "DbgLineRender.h"
#include "PhysXManager.h"
#include "ResModel.h"
#include "ResModelBone.h"
#include "ResPhysXMaterial.h"

#pragma push_macro("new")
#undef new
#include "PxPhysicsAPI.h"
#pragma pop_macro("new")

NS_USING(Engine)
using namespace physx;

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

	_bool IsFiniteMatrix(FXMMATRIX Matrix)
	{
		_float4x4 StoredMatrix{};
		XMStoreFloat4x4(&StoredMatrix, Matrix);
		const _float* pValues =
			reinterpret_cast<const _float*>(
				&StoredMatrix);
		for (size_t i = 0; i < 16; ++i)
		{
			if (!IsFinite(pValues[i]))
				return false;
		}

		return true;
	}

	_bool IsValidQuaternion(const _float4& vQuaternion)
	{
		if (!IsFinite(vQuaternion.x) ||
			!IsFinite(vQuaternion.y) ||
			!IsFinite(vQuaternion.z) ||
			!IsFinite(vQuaternion.w))
		{
			return false;
		}

		const _float fMagnitudeSquared =
			vQuaternion.x * vQuaternion.x +
			vQuaternion.y * vQuaternion.y +
			vQuaternion.z * vQuaternion.z +
			vQuaternion.w * vQuaternion.w;
		return fMagnitudeSquared > FLT_EPSILON;
	}

	_bool IsValidShape(const PX_RAGDOLL_SHAPE_DESC& tShape)
	{
		if (tShape.sName.empty() ||
			!IsFinite(tShape.vLocalPosition) ||
			!IsValidQuaternion(tShape.vLocalRotation))
		{
			return false;
		}

		switch (tShape.eType)
		{
		case PX_RAGDOLL_SHAPE_TYPE::BOX:
			return IsFinite(tShape.vHalfExtents) &&
				tShape.vHalfExtents.x > 0.f &&
				tShape.vHalfExtents.y > 0.f &&
				tShape.vHalfExtents.z > 0.f;

		case PX_RAGDOLL_SHAPE_TYPE::SPHERE:
			return IsFinite(tShape.fRadius) &&
				tShape.fRadius > 0.f;

		case PX_RAGDOLL_SHAPE_TYPE::CAPSULE:
			return IsFinite(tShape.fRadius) &&
				IsFinite(tShape.fHalfHeight) &&
				tShape.fRadius > 0.f &&
				tShape.fHalfHeight >= 0.f;
		}

		return false;
	}

	_bool IsValidMotion(PX_RAGDOLL_D6_MOTION eMotion)
	{
		return eMotion == PX_RAGDOLL_D6_MOTION::LOCKED ||
			eMotion == PX_RAGDOLL_D6_MOTION::LIMITED ||
			eMotion == PX_RAGDOLL_D6_MOTION::FREE;
	}

	PxQuat ToPxQuaternion(const _float4& vQuaternion)
	{
		PxQuat tQuaternion{
			vQuaternion.x,
			vQuaternion.y,
			vQuaternion.z,
			vQuaternion.w
		};
		return tQuaternion.magnitudeSquared() > 0.f
			? tQuaternion.getNormalized()
			: PxQuat{ PxIdentity };
	}

	_matrix MakePoseMatrix(
		const _float3& vPosition,
		const _float4& vRotation)
	{
		return XMMatrixRotationQuaternion(
			XMLoadFloat4(&vRotation)) *
			XMMatrixTranslation(
				vPosition.x,
				vPosition.y,
				vPosition.z);
	}

	_bool ToPxTransform(
		FXMMATRIX Matrix,
		PxTransform& tOutTransform)
	{
		_vector vScale{};
		_vector vRotation{};
		_vector vTranslation{};
		if (!XMMatrixDecompose(
			&vScale,
			&vRotation,
			&vTranslation,
			Matrix))
		{
			return false;
		}

		_float3 vPosition{};
		_float4 vQuaternion{};
		XMStoreFloat3(&vPosition, vTranslation);
		XMStoreFloat4(
			&vQuaternion,
			XMQuaternionNormalize(vRotation));
		tOutTransform = PxTransform{
			PxVec3{
				vPosition.x,
				vPosition.y,
				vPosition.z },
			ToPxQuaternion(vQuaternion)
		};
		return tOutTransform.isValid();
	}

	_matrix ToMatrix(const PxTransform& tTransform)
	{
		const _float4 vRotation{
			tTransform.q.x,
			tTransform.q.y,
			tTransform.q.z,
			tTransform.q.w
		};
		return XMMatrixRotationQuaternion(
			XMLoadFloat4(&vRotation)) *
			XMMatrixTranslation(
				tTransform.p.x,
				tTransform.p.y,
				tTransform.p.z);
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
			std::abs(fDeterminant) > FLT_EPSILON &&
			IsFiniteMatrix(OutInverse);
	}

	PxTransform MakeShapeLocalPose(
		const PX_RAGDOLL_SHAPE_DESC& tShape)
	{
		PxQuat tRotation =
			ToPxQuaternion(tShape.vLocalRotation);
		if (tShape.eType ==
			PX_RAGDOLL_SHAPE_TYPE::CAPSULE)
		{
			// PhysX Capsule 기본축 X를 엔진/DebugLine의 Y축 규약으로 맞춘다.
			tRotation =
				tRotation *
				PxQuat{
					PxHalfPi,
					PxVec3{ 0.f, 0.f, 1.f }
				};
		}

		return PxTransform{
			PxVec3{
				tShape.vLocalPosition.x,
				tShape.vLocalPosition.y,
				tShape.vLocalPosition.z },
			tRotation
		};
	}

	PxTransform MakeJointLocalPose(
		const _float3& vPosition,
		const _float4& vRotation)
	{
		return PxTransform{
			PxVec3{
				vPosition.x,
				vPosition.y,
				vPosition.z },
			ToPxQuaternion(vRotation)
		};
	}

	PxD6Motion::Enum ToPxD6Motion(
		PX_RAGDOLL_D6_MOTION eMotion)
	{
		switch (eMotion)
		{
		case PX_RAGDOLL_D6_MOTION::LIMITED:
			return PxD6Motion::eLIMITED;
		case PX_RAGDOLL_D6_MOTION::FREE:
			return PxD6Motion::eFREE;
		case PX_RAGDOLL_D6_MOTION::LOCKED:
		default:
			return PxD6Motion::eLOCKED;
		}
	}

	template<typename T>
	void ApplyAngularLimitResponse(
		T& tLimit,
		const PX_RAGDOLL_D6_JOINT_DESC& tDesc)
	{
		tLimit.restitution = tDesc.fLimitRestitution;
		tLimit.bounceThreshold =
			XMConvertToRadians(
				tDesc.fLimitBounceThreshold);
		tLimit.stiffness = tDesc.fLimitStiffness;
		tLimit.damping = tDesc.fLimitDamping;
	}
}

CComPxRagdoll::CComPxRagdoll()
{
}

CComPxRagdoll::CComPxRagdoll(
	const CComPxRagdoll& Prototype)
	: CComponent{ Prototype }
{
}

CComPxRagdoll::~CComPxRagdoll()
{
}

HRESULT CComPxRagdoll::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<const DESC*>(pArg);
	if (!pDesc ||
		FAILED(CComponent::Initialize(pArg)) ||
		!ValidateDesc(pDesc->tRagdoll))
	{
		return E_FAIL;
	}

	m_tRagdoll = pDesc->tRagdoll;
	if (IsConfigured() &&
		FAILED(BuildRuntimeBodies()))
	{
		return E_FAIL;
	}

	return BuildRuntimeJoints();
}

HRESULT CComPxRagdoll::BuildRuntimeJoints()
{
	ReleaseRuntimeJoints();

	auto* pPhysics = CGameInstance::Get().PxGetPhysics();
	auto* pGameObject = GetGameObject();
	if (!pPhysics ||
		!pGameObject ||
		m_RuntimeBodies.size() !=
			m_tRagdoll.Bodies.size())
	{
		return E_FAIL;
	}

	std::unordered_map<std::string, size_t>
		BodyIndexByName{};
	BodyIndexByName.reserve(m_tRagdoll.Bodies.size());
	for (size_t iBody = 0;
		iBody < m_tRagdoll.Bodies.size();
		++iBody)
	{
		BodyIndexByName.emplace(
			m_tRagdoll.Bodies[iBody].sBodyName,
			iBody);
	}

	// PxJoint::userData가 아래 원소를 가리키므로 생성 중 재할당되지 않게 한다.
	m_RuntimeJoints.reserve(m_tRagdoll.Joints.size());
	for (size_t iJoint = 0;
		iJoint < m_tRagdoll.Joints.size();
		++iJoint)
	{
		const auto& tJointDesc =
			m_tRagdoll.Joints[iJoint];
		const auto ParentIter =
			BodyIndexByName.find(
				tJointDesc.sParentBodyName);
		const auto ChildIter =
			BodyIndexByName.find(
				tJointDesc.sChildBodyName);
		if (ParentIter == BodyIndexByName.end() ||
			ChildIter == BodyIndexByName.end())
		{
			ReleaseRuntimeJoints();
			return E_FAIL;
		}

		auto* pParentActor =
			m_RuntimeBodies[ParentIter->second].pActor;
		auto* pChildActor =
			m_RuntimeBodies[ChildIter->second].pActor;
		if (!pParentActor || !pChildActor)
		{
			ReleaseRuntimeJoints();
			return E_FAIL;
		}

		PxD6Joint* pJoint = PxD6JointCreate(
			*pPhysics,
			pParentActor,
			MakeJointLocalPose(
				tJointDesc.vParentLocalPosition,
				tJointDesc.vParentLocalRotation),
			pChildActor,
			MakeJointLocalPose(
				tJointDesc.vChildLocalPosition,
				tJointDesc.vChildLocalRotation));
		if (!pJoint)
		{
			ReleaseRuntimeJoints();
			return E_FAIL;
		}

		m_RuntimeJoints.emplace_back();
		auto& tRuntimeJoint =
			m_RuntimeJoints.back();
		tRuntimeJoint.pJoint = pJoint;

		pJoint->setMotion(
			PxD6Axis::eX,
			PxD6Motion::eLOCKED);
		pJoint->setMotion(
			PxD6Axis::eY,
			PxD6Motion::eLOCKED);
		pJoint->setMotion(
			PxD6Axis::eZ,
			PxD6Motion::eLOCKED);
		pJoint->setMotion(
			PxD6Axis::eTWIST,
			ToPxD6Motion(
				tJointDesc.eTwistMotion));
		pJoint->setMotion(
			PxD6Axis::eSWING1,
			ToPxD6Motion(
				tJointDesc.eSwingYMotion));
		pJoint->setMotion(
			PxD6Axis::eSWING2,
			ToPxD6Motion(
				tJointDesc.eSwingZMotion));

		PxJointAngularLimitPair tTwistLimit{
			XMConvertToRadians(
				tJointDesc.fTwistLowerDegrees),
			XMConvertToRadians(
				tJointDesc.fTwistUpperDegrees)
		};
		ApplyAngularLimitResponse(
			tTwistLimit,
			tJointDesc);
		pJoint->setTwistLimit(tTwistLimit);

		PxJointLimitCone tSwingLimit{
			XMConvertToRadians(
				tJointDesc.fSwingYDegrees),
			XMConvertToRadians(
				tJointDesc.fSwingZDegrees)
		};
		ApplyAngularLimitResponse(
			tSwingLimit,
			tJointDesc);
		pJoint->setSwingLimit(tSwingLimit);

		pJoint->setBreakForce(
			tJointDesc.fBreakForce,
			tJointDesc.fBreakTorque);
		pJoint->setInvMassScale0(
			tJointDesc.fInvMassScaleParent);
		pJoint->setInvMassScale1(
			tJointDesc.fInvMassScaleChild);
		pJoint->setInvInertiaScale0(
			tJointDesc.fInvInertiaScaleParent);
		pJoint->setInvInertiaScale1(
			tJointDesc.fInvInertiaScaleChild);
		pJoint->setConstraintFlag(
			PxConstraintFlag::eCOLLISION_ENABLED,
			tJointDesc.bCollisionEnabled);
		pJoint->setConstraintFlag(
			PxConstraintFlag::eVISUALIZATION,
			tJointDesc.bVisualizationEnabled);
		pJoint->setConstraintFlag(
			PxConstraintFlag::eDISABLE_CONSTRAINT,
			!tJointDesc.bEnabled);

		tRuntimeJoint.tUserData.hJointOwner =
			pGameObject->GetHandle();
		tRuntimeJoint.tUserData.hActorA =
			pGameObject->GetHandle();
		tRuntimeJoint.tUserData.hActorB =
			pGameObject->GetHandle();
		tRuntimeJoint.tUserData.iJointSubIndex =
			static_cast<uint32_t>(iJoint);
		pJoint->userData =
			&tRuntimeJoint.tUserData;
	}

	return S_OK;
}

_bool CComPxRagdoll::ValidateDesc(
	const PX_RAGDOLL_DESC& tDesc)
{
	if (tDesc.iVersion != PX_RAGDOLL_DATA_VERSION)
		return false;

	std::unordered_set<std::string> BodyNames{};
	std::unordered_set<std::string> BoneNames{};
	for (const auto& tBody : tDesc.Bodies)
	{
		if (tBody.sBodyName.empty() ||
			tBody.sBoneName.empty() ||
			!BodyNames.emplace(tBody.sBodyName).second ||
			!BoneNames.emplace(tBody.sBoneName).second ||
			!IsFinite(tBody.vBoneToActorPosition) ||
			!IsValidQuaternion(tBody.vBoneToActorRotation) ||
			!IsFinite(tBody.fMass) ||
			!IsFinite(tBody.fLinearDamping) ||
			!IsFinite(tBody.fAngularDamping) ||
			!IsFinite(tBody.fMaxDepenetrationVelocity) ||
			tBody.fMass <= 0.f ||
			tBody.fLinearDamping < 0.f ||
			tBody.fAngularDamping < 0.f ||
			tBody.fMaxDepenetrationVelocity < 0.f ||
			tBody.Shapes.empty())
		{
			return false;
		}

		std::unordered_set<std::string> ShapeNames{};
		for (const auto& tShape : tBody.Shapes)
		{
			if (!IsValidShape(tShape) ||
				!ShapeNames.emplace(tShape.sName).second)
			{
				return false;
			}
		}
	}

	std::unordered_set<std::string> JointNames{};
	std::unordered_map<std::string, std::string> ParentByChild{};
	for (const auto& tJoint : tDesc.Joints)
	{
		const _bool bInvalidTwistLimit =
			tJoint.eTwistMotion ==
				PX_RAGDOLL_D6_MOTION::LIMITED &&
			(tJoint.fTwistLowerDegrees >
				tJoint.fTwistUpperDegrees ||
				tJoint.fTwistLowerDegrees < -179.9f ||
				tJoint.fTwistUpperDegrees > 179.9f);
		const _bool bInvalidSwingYLimit =
			tJoint.eSwingYMotion ==
				PX_RAGDOLL_D6_MOTION::LIMITED &&
			(tJoint.fSwingYDegrees <= 0.f ||
				tJoint.fSwingYDegrees > 179.9f);
		const _bool bInvalidSwingZLimit =
			tJoint.eSwingZMotion ==
				PX_RAGDOLL_D6_MOTION::LIMITED &&
			(tJoint.fSwingZDegrees <= 0.f ||
				tJoint.fSwingZDegrees > 179.9f);

		if (tJoint.sJointName.empty() ||
			!JointNames.emplace(tJoint.sJointName).second ||
			tJoint.sParentBodyName == tJoint.sChildBodyName ||
			!BodyNames.contains(tJoint.sParentBodyName) ||
			!BodyNames.contains(tJoint.sChildBodyName) ||
			!ParentByChild.emplace(
				tJoint.sChildBodyName,
				tJoint.sParentBodyName).second ||
			!IsFinite(tJoint.vParentLocalPosition) ||
			!IsFinite(tJoint.vChildLocalPosition) ||
			!IsValidQuaternion(tJoint.vParentLocalRotation) ||
			!IsValidQuaternion(tJoint.vChildLocalRotation) ||
			!IsValidMotion(tJoint.eTwistMotion) ||
			!IsValidMotion(tJoint.eSwingYMotion) ||
			!IsValidMotion(tJoint.eSwingZMotion) ||
			!IsFinite(tJoint.fTwistLowerDegrees) ||
			!IsFinite(tJoint.fTwistUpperDegrees) ||
			!IsFinite(tJoint.fSwingYDegrees) ||
			!IsFinite(tJoint.fSwingZDegrees) ||
			!IsFinite(tJoint.fLimitStiffness) ||
			!IsFinite(tJoint.fLimitDamping) ||
			!IsFinite(tJoint.fLimitRestitution) ||
			!IsFinite(tJoint.fLimitBounceThreshold) ||
			!IsFinite(tJoint.fBreakForce) ||
			!IsFinite(tJoint.fBreakTorque) ||
			!IsFinite(tJoint.fInvMassScaleParent) ||
			!IsFinite(tJoint.fInvMassScaleChild) ||
			!IsFinite(tJoint.fInvInertiaScaleParent) ||
			!IsFinite(tJoint.fInvInertiaScaleChild) ||
			bInvalidTwistLimit ||
			bInvalidSwingYLimit ||
			bInvalidSwingZLimit ||
			tJoint.fLimitStiffness < 0.f ||
			tJoint.fLimitDamping < 0.f ||
			tJoint.fLimitRestitution < 0.f ||
			tJoint.fLimitRestitution > 1.f ||
			tJoint.fLimitBounceThreshold < 0.f ||
			tJoint.fBreakForce < 0.f ||
			tJoint.fBreakTorque < 0.f ||
			tJoint.fInvMassScaleParent < 0.f ||
			tJoint.fInvMassScaleChild < 0.f ||
			tJoint.fInvInertiaScaleParent < 0.f ||
			tJoint.fInvInertiaScaleChild < 0.f)
		{
			return false;
		}
	}

	for (const auto& tBody : tDesc.Bodies)
	{
		std::unordered_set<std::string> Visited{};
		std::string sCurrent = tBody.sBodyName;
		while (true)
		{
			const auto Iter = ParentByChild.find(sCurrent);
			if (Iter == ParentByChild.end())
				break;

			if (!Visited.emplace(sCurrent).second)
				return false;

			sCurrent = Iter->second;
		}
	}

	return true;
}

_bool CComPxRagdoll::BindSkeleton(
	const CResModel& Model)
{
	UnbindSkeleton();

	if (!IsConfigured())
		return false;

	const auto& Bones = Model.GetBones();
	if (Bones.empty() ||
		Bones.size() >
			static_cast<size_t>(
				std::numeric_limits<int32_t>::max()))
	{
		return false;
	}

	std::vector<uint32_t> BoneIndices(
		m_tRagdoll.Bodies.size());
	std::vector<int32_t> BoneParentIndices(
		Bones.size(),
		-1);
	std::vector<int32_t> BodyIndexByBoneIndex(
		Bones.size(),
		-1);

	for (size_t iBone = 0;
		iBone < Bones.size();
		++iBone)
	{
		const auto& pBone = Bones[iBone];
		if (!pBone)
			return false;

		const int32_t iParent =
			pBone->GetParendBoneIndex();
		if (iParent < -1 ||
			iParent >= static_cast<int32_t>(iBone))
		{
			return false;
		}
		BoneParentIndices[iBone] = iParent;
	}

	for (size_t iBody = 0;
		iBody < m_tRagdoll.Bodies.size();
		++iBody)
	{
		const std::string& sBoneName =
			m_tRagdoll.Bodies[iBody].sBoneName;
		auto Iter = std::ranges::find_if(
			Bones,
			[&sBoneName](
				const SPtr<CResModelBone>& pBone)
			{
				return pBone &&
					pBone->GetBoneName() ==
						sBoneName;
			});
		if (Iter == Bones.end())
			return false;

		const size_t iBone =
			static_cast<size_t>(
				std::distance(
					Bones.begin(),
					Iter));
		if (BodyIndexByBoneIndex[iBone] >= 0)
			return false;

		BoneIndices[iBody] =
			static_cast<uint32_t>(iBone);
		BodyIndexByBoneIndex[iBone] =
			static_cast<int32_t>(iBody);
	}

	m_BoneIndices = std::move(BoneIndices);
	m_BoneParentIndices =
		std::move(BoneParentIndices);
	m_BodyIndexByBoneIndex =
		std::move(BodyIndexByBoneIndex);
	m_iBoundBoneCount = Bones.size();
	return true;
}

void CComPxRagdoll::UnbindSkeleton()
{
	m_BoneIndices.clear();
	m_BoneParentIndices.clear();
	m_BodyIndexByBoneIndex.clear();
	m_CachedActorWorldMatrices.clear();
	m_iBoundBoneCount = 0;
	m_bHasCachedAnimationPose = false;
}

_bool CComPxRagdoll::CacheAnimationPose(
	const std::vector<_float4x4>& CombinedBoneMatrices,
	_fmatrix ObjectWorld)
{
	if (!IsSkeletonBound() ||
		CombinedBoneMatrices.size() <
			m_iBoundBoneCount ||
		!IsFiniteMatrix(ObjectWorld))
	{
		return false;
	}

	std::vector<_float4x4>
		CachedActorWorldMatrices(
			m_tRagdoll.Bodies.size());
	for (size_t iBody = 0;
		iBody < m_tRagdoll.Bodies.size();
		++iBody)
	{
		const uint32_t iBone =
			m_BoneIndices[iBody];
		if (iBone >= CombinedBoneMatrices.size())
			return false;

		const auto& tBody =
			m_tRagdoll.Bodies[iBody];
		const _matrix BoneWorld =
			XMLoadFloat4x4(
				&CombinedBoneMatrices[iBone]) *
			ObjectWorld;
		const _matrix ActorWorld =
			MakePoseMatrix(
				tBody.vBoneToActorPosition,
				tBody.vBoneToActorRotation) *
			BoneWorld;

		PxTransform tActorPose{ PxIdentity };
		if (!ToPxTransform(
			ActorWorld,
			tActorPose))
		{
			return false;
		}

		XMStoreFloat4x4(
			&CachedActorWorldMatrices[iBody],
			ToMatrix(tActorPose));
	}

	m_CachedActorWorldMatrices =
		std::move(CachedActorWorldMatrices);
	m_bHasCachedAnimationPose = true;
	return true;
}

_bool CComPxRagdoll::ApplyCachedPoseToKinematicBodies()
{
	if (!IsSkeletonBound() ||
		!m_bHasCachedAnimationPose ||
		m_bRagdollActive ||
		!HasRuntimeBodies() ||
		m_CachedActorWorldMatrices.size() !=
			m_RuntimeBodies.size())
	{
		return false;
	}

	std::vector<PxTransform> ActorPoses(
		m_RuntimeBodies.size(),
		PxTransform{ PxIdentity });
	for (size_t iBody = 0;
		iBody < m_RuntimeBodies.size();
		++iBody)
	{
		auto* pActor =
			m_RuntimeBodies[iBody].pActor;
		if (!pActor ||
			!pActor->getRigidBodyFlags().isSet(
				PxRigidBodyFlag::eKINEMATIC) ||
			!ToPxTransform(
				XMLoadFloat4x4(
					&m_CachedActorWorldMatrices[
						iBody]),
				ActorPoses[iBody]))
		{
			return false;
		}
	}

	for (size_t iBody = 0;
		iBody < m_RuntimeBodies.size();
		++iBody)
	{
		m_RuntimeBodies[iBody].pActor
			->setKinematicTarget(
				ActorPoses[iBody]);
	}

	return true;
}

_bool CComPxRagdoll::WritePhysicsPoseToBones(
	std::vector<_float4x4>&
		InOutCombinedBoneMatrices,
	_fmatrix ObjectWorld) const
{
	if (!IsSkeletonBound() ||
		!m_bRagdollActive ||
		!HasRuntimeBodies() ||
		InOutCombinedBoneMatrices.size() <
			m_iBoundBoneCount ||
		m_BoneParentIndices.size() !=
			m_iBoundBoneCount ||
		m_BodyIndexByBoneIndex.size() !=
			m_iBoundBoneCount)
	{
		return false;
	}

	_matrix InverseObjectWorld{};
	if (!TryInverse(
		ObjectWorld,
		InverseObjectWorld))
	{
		return false;
	}

	const std::vector<_float4x4>
		OriginalCombinedBoneMatrices =
			InOutCombinedBoneMatrices;
	std::vector<_float4x4>
		UpdatedCombinedBoneMatrices =
			OriginalCombinedBoneMatrices;

	for (size_t iBone = 0;
		iBone < m_iBoundBoneCount;
		++iBone)
	{
		const int32_t iBody =
			m_BodyIndexByBoneIndex[iBone];
		if (iBody >= 0)
		{
			const size_t iBodyIndex =
				static_cast<size_t>(iBody);
			_float4x4 ActorWorldFloat4x4{};
			if (!GetBodyWorldMatrix(
				iBodyIndex,
				ActorWorldFloat4x4))
			{
				return false;
			}

			const auto& tBody =
				m_tRagdoll.Bodies[iBodyIndex];
			_matrix InverseBoneToActor{};
			if (!TryInverse(
				MakePoseMatrix(
					tBody.vBoneToActorPosition,
					tBody.vBoneToActorRotation),
				InverseBoneToActor))
			{
				return false;
			}

			const _matrix PhysicsBoneModel =
				InverseBoneToActor *
				XMLoadFloat4x4(
					&ActorWorldFloat4x4) *
				InverseObjectWorld;
			if (!IsFiniteMatrix(PhysicsBoneModel))
				return false;

			_vector vOriginalScale{};
			_vector vOriginalRotation{};
			_vector vOriginalTranslation{};
			_vector vPhysicsScale{};
			_vector vPhysicsRotation{};
			_vector vPhysicsTranslation{};
			if (!XMMatrixDecompose(
					&vOriginalScale,
					&vOriginalRotation,
					&vOriginalTranslation,
					XMLoadFloat4x4(
						&OriginalCombinedBoneMatrices[
							iBone])) ||
				!XMMatrixDecompose(
					&vPhysicsScale,
					&vPhysicsRotation,
					&vPhysicsTranslation,
					PhysicsBoneModel))
			{
				return false;
			}

			// PhysX pose contains only translation and rotation.
			// Preserve the animated combined-bone scale so the
			// model pre-transform scale is not lost in ragdoll mode.
			const _matrix ScaledPhysicsBoneModel =
				XMMatrixScalingFromVector(
					vOriginalScale) *
				XMMatrixRotationQuaternion(
					XMQuaternionNormalize(
						vPhysicsRotation)) *
				XMMatrixTranslationFromVector(
					vPhysicsTranslation);
			if (!IsFiniteMatrix(
				ScaledPhysicsBoneModel))
			{
				return false;
			}

			XMStoreFloat4x4(
				&UpdatedCombinedBoneMatrices[
					iBone],
				ScaledPhysicsBoneModel);
			continue;
		}

		const int32_t iParent =
			m_BoneParentIndices[iBone];
		if (iParent < 0)
			continue;

		_matrix InverseOriginalParent{};
		if (!TryInverse(
			XMLoadFloat4x4(
				&OriginalCombinedBoneMatrices[
					static_cast<size_t>(iParent)]),
			InverseOriginalParent))
		{
			return false;
		}

		const _matrix LocalBone =
			XMLoadFloat4x4(
				&OriginalCombinedBoneMatrices[
					iBone]) *
			InverseOriginalParent;
		const _matrix UpdatedBone =
			LocalBone *
			XMLoadFloat4x4(
				&UpdatedCombinedBoneMatrices[
					static_cast<size_t>(iParent)]);
		if (!IsFiniteMatrix(UpdatedBone))
			return false;

		XMStoreFloat4x4(
			&UpdatedCombinedBoneMatrices[iBone],
			UpdatedBone);
	}

	InOutCombinedBoneMatrices =
		std::move(UpdatedCombinedBoneMatrices);
	return true;
}

HRESULT CComPxRagdoll::BuildRuntimeBodies()
{
	ReleaseRuntimeBodies();

	auto* pGameObject = GetGameObject();
	auto* pPhysics = CGameInstance::Get().PxGetPhysics();
	auto* pScene = CGameInstance::Get().PxGetScene();
	auto* pPhysXManager =
		CGameInstance::Get().GetPhysXManager();
	if (!pGameObject ||
		!pPhysics ||
		!pScene ||
		!pPhysXManager)
	{
		return E_FAIL;
	}

	m_pMaterial =
		CResPhysXMaterial::CreateAndLoad({});
	if (!m_pMaterial ||
		!m_pMaterial->GetMaterial())
	{
		ReleaseRuntimeBodies();
		return E_FAIL;
	}

	const _matrix OwnerWorld =
		pGameObject->GetTransform()
			.GetLoadedCombinedWorldMatrix();
	m_RuntimeBodies.reserve(m_tRagdoll.Bodies.size());

	for (size_t iBody = 0;
		iBody < m_tRagdoll.Bodies.size();
		++iBody)
	{
		const auto& tBody = m_tRagdoll.Bodies[iBody];
		PxTransform tActorPose{ PxIdentity };
		if (!ToPxTransform(
			MakePoseMatrix(
				tBody.vBoneToActorPosition,
				tBody.vBoneToActorRotation) *
			OwnerWorld,
			tActorPose))
		{
			ReleaseRuntimeBodies();
			return E_FAIL;
		}

		RUNTIME_BODY tRuntimeBody{};
		tRuntimeBody.pActor =
			pPhysics->createRigidDynamic(tActorPose);
		if (!tRuntimeBody.pActor)
		{
			ReleaseRuntimeBodies();
			return E_FAIL;
		}

		m_RuntimeBodies.emplace_back(
			std::move(tRuntimeBody));
		auto& tRuntime = m_RuntimeBodies.back();
		tRuntime.pActor->setRigidBodyFlag(
			PxRigidBodyFlag::eKINEMATIC,
			true);
		tRuntime.pActor->setLinearDamping(
			tBody.fLinearDamping);
		tRuntime.pActor->setAngularDamping(
			tBody.fAngularDamping);
		// Ragdoll은 여러 동적 바디가 관절 체인으로 연결되므로 기본
		// 반복 횟수보다 높여 관절 꺾임과 순간적인 벌어짐을 줄인다.
		tRuntime.pActor->setSolverIterationCounts(8, 2);
		// 강한 충돌이나 한계각 보정에서 발생하는 비현실적인 고속
		// 회전을 제한한다. 20 rad/s는 빠른 낙하 반응은 유지한다.
		tRuntime.pActor->setMaxAngularVelocity(20.f);
		tRuntime.pActor->setMaxDepenetrationVelocity(
			tBody.fMaxDepenetrationVelocity);
		tRuntime.pActor->setActorFlag(
			PxActorFlag::eDISABLE_GRAVITY,
			!tBody.bGravityEnabled);
		tRuntime.pActor->userData = nullptr;

		PX_ACTOR_USER_DATA tActorUserData{};
		tActorUserData.hGameObject =
			pGameObject->GetHandle();
		tActorUserData.eType =
			PX_ACTOR_TYPE::RAGDOLL_BONE;
		tActorUserData.iSubIndex =
			static_cast<uint32_t>(iBody);
		if (!pPhysXManager->RegisterActor(
			tRuntime.pActor,
			tActorUserData))
		{
			ReleaseRuntimeBodies();
			return E_FAIL;
		}

		tRuntime.Shapes.reserve(tBody.Shapes.size());
		for (const auto& tShapeDesc : tBody.Shapes)
		{
			PxShape* pShape{};
			switch (tShapeDesc.eType)
			{
			case PX_RAGDOLL_SHAPE_TYPE::BOX:
				pShape = pPhysics->createShape(
					PxBoxGeometry{
						tShapeDesc.vHalfExtents.x,
						tShapeDesc.vHalfExtents.y,
						tShapeDesc.vHalfExtents.z },
					*m_pMaterial->GetMaterial(),
					true);
				break;

			case PX_RAGDOLL_SHAPE_TYPE::SPHERE:
				pShape = pPhysics->createShape(
					PxSphereGeometry{
						tShapeDesc.fRadius },
					*m_pMaterial->GetMaterial(),
					true);
				break;

			case PX_RAGDOLL_SHAPE_TYPE::CAPSULE:
				pShape = pPhysics->createShape(
					PxCapsuleGeometry{
						tShapeDesc.fRadius,
						tShapeDesc.fHalfHeight },
					*m_pMaterial->GetMaterial(),
					true);
				break;
			}

			if (!pShape)
			{
				ReleaseRuntimeBodies();
				return E_FAIL;
			}

			tRuntime.Shapes.emplace_back(pShape);
			pShape->setLocalPose(
				MakeShapeLocalPose(tShapeDesc));
			pShape->setFlag(
				PxShapeFlag::eSIMULATION_SHAPE,
				tShapeDesc.bSimulationEnabled);
			pShape->setFlag(
				PxShapeFlag::eSCENE_QUERY_SHAPE,
				tShapeDesc.bQueryEnabled);
			pShape->setFlag(
				PxShapeFlag::eTRIGGER_SHAPE,
				false);

			PxFilterData tSimulationFilter{};
			tSimulationFilter.word0 =
				tShapeDesc.iLayer;
			tSimulationFilter.word1 =
				tShapeDesc.iSimulationMask;
			pShape->setSimulationFilterData(
				tSimulationFilter);

			PxFilterData tQueryFilter{};
			tQueryFilter.word0 =
				tShapeDesc.iLayer;
			tQueryFilter.word1 =
				tShapeDesc.iQueryMask;
			pShape->setQueryFilterData(tQueryFilter);
			pShape->userData = nullptr;

			PX_SHAPE_USER_DATA tShapeUserData{};
			tShapeUserData.hGameObject =
				pGameObject->GetHandle();
			tShapeUserData.eType =
				PX_SHAPE_TYPE::RAGDOLL;
			tShapeUserData.iSubIndex =
				static_cast<uint32_t>(iBody);
			if (!pPhysXManager->RegisterShape(
				pShape,
				tShapeUserData) ||
				!tRuntime.pActor->attachShape(*pShape))
			{
				ReleaseRuntimeBodies();
				return E_FAIL;
			}
		}

		if (!PxRigidBodyExt::setMassAndUpdateInertia(
			*tRuntime.pActor,
			tBody.fMass))
		{
			ReleaseRuntimeBodies();
			return E_FAIL;
		}

		pScene->addActor(*tRuntime.pActor);
	}

	return S_OK;
}

_bool CComPxRagdoll::SyncKinematicPoseFromOwner()
{
	if (!HasRuntimeBodies() ||
		m_bRagdollActive ||
		IsSkeletonBound() ||
		!GetGameObject())
	{
		return false;
	}

	const _matrix OwnerWorld =
		GetGameObject()->GetTransform()
			.GetLoadedCombinedWorldMatrix();
	for (size_t iBody = 0;
		iBody < m_RuntimeBodies.size();
		++iBody)
	{
		auto* pActor =
			m_RuntimeBodies[iBody].pActor;
		const auto& tBody =
			m_tRagdoll.Bodies[iBody];
		if (!pActor ||
			!pActor->getRigidBodyFlags().isSet(
				PxRigidBodyFlag::eKINEMATIC))
		{
			return false;
		}

		PxTransform tActorPose{ PxIdentity };
		if (!ToPxTransform(
			MakePoseMatrix(
				tBody.vBoneToActorPosition,
				tBody.vBoneToActorRotation) *
			OwnerWorld,
			tActorPose))
		{
			return false;
		}

		pActor->setKinematicTarget(tActorPose);
	}

	return true;
}

_bool CComPxRagdoll::ActivateRagdoll(
	const _float3& vLinearVelocity,
	const _float3& vAngularVelocityRadians)
{
	if (!HasRuntimeBodies() ||
		!GetGameObject() ||
		!IsFinite(vLinearVelocity) ||
		!IsFinite(vAngularVelocityRadians))
	{
		return false;
	}

	if (m_bRagdollActive)
		return true;

	const _bool bUseCachedAnimationPose =
		IsSkeletonBound();
	if (bUseCachedAnimationPose &&
		(!m_bHasCachedAnimationPose ||
			m_CachedActorWorldMatrices.size() !=
				m_RuntimeBodies.size()))
	{
		return false;
	}

	const _matrix OwnerWorld =
		GetGameObject()->GetTransform()
			.GetLoadedCombinedWorldMatrix();

	// 활성화 직전 모든 바디를 같은 애니메이션/오브젝트 포즈에 스냅한다.
	for (size_t iBody = 0;
		iBody < m_RuntimeBodies.size();
		++iBody)
	{
		auto* pActor =
			m_RuntimeBodies[iBody].pActor;
		const auto& tBody =
			m_tRagdoll.Bodies[iBody];
		if (!pActor)
			return false;

		const _matrix ActorWorld =
			bUseCachedAnimationPose
			? XMLoadFloat4x4(
				&m_CachedActorWorldMatrices[
					iBody])
			: MakePoseMatrix(
				tBody.vBoneToActorPosition,
				tBody.vBoneToActorRotation) *
				OwnerWorld;
		PxTransform tActorPose{ PxIdentity };
		if (!ToPxTransform(
			ActorWorld,
			tActorPose))
		{
			return false;
		}

		pActor->setGlobalPose(tActorPose, false);
	}

	for (auto& tRuntime : m_RuntimeBodies)
	{
		auto* pActor = tRuntime.pActor;
		pActor->setRigidBodyFlag(
			PxRigidBodyFlag::eKINEMATIC,
			false);
		pActor->setLinearVelocity(
			PxVec3{
				vLinearVelocity.x,
				vLinearVelocity.y,
				vLinearVelocity.z },
			false);
		pActor->setAngularVelocity(
			PxVec3{
				vAngularVelocityRadians.x,
				vAngularVelocityRadians.y,
				vAngularVelocityRadians.z },
			false);
		pActor->wakeUp();
	}

	m_bRagdollActive = true;
	return true;
}

_bool CComPxRagdoll::ResetToKinematicPose()
{
	if (!HasRuntimeBodies() ||
		!GetGameObject())
	{
		return false;
	}

	const _bool bUseCachedAnimationPose =
		IsSkeletonBound();
	if (bUseCachedAnimationPose &&
		(!m_bHasCachedAnimationPose ||
			m_CachedActorWorldMatrices.size() !=
				m_RuntimeBodies.size()))
	{
		return false;
	}

	for (auto& tRuntimeJoint : m_RuntimeJoints)
	{
		if (tRuntimeJoint.pJoint)
		{
			tRuntimeJoint.pJoint->setConstraintFlag(
				PxConstraintFlag::eDISABLE_CONSTRAINT,
				true);
		}
	}

	for (auto& tRuntime : m_RuntimeBodies)
	{
		auto* pActor = tRuntime.pActor;
		if (!pActor)
			return false;

		if (!pActor->getRigidBodyFlags().isSet(
			PxRigidBodyFlag::eKINEMATIC))
		{
			pActor->setLinearVelocity(
				PxVec3{ 0.f },
				false);
			pActor->setAngularVelocity(
				PxVec3{ 0.f },
				false);
			pActor->clearForce(
				PxForceMode::eFORCE);
			pActor->clearTorque(
				PxForceMode::eFORCE);
			pActor->setRigidBodyFlag(
				PxRigidBodyFlag::eKINEMATIC,
				true);
		}
	}

	const _matrix OwnerWorld =
		GetGameObject()->GetTransform()
			.GetLoadedCombinedWorldMatrix();
	for (size_t iBody = 0;
		iBody < m_RuntimeBodies.size();
		++iBody)
	{
		PxTransform tActorPose{ PxIdentity };
		const auto& tBody =
			m_tRagdoll.Bodies[iBody];
		const _matrix ActorWorld =
			bUseCachedAnimationPose
			? XMLoadFloat4x4(
				&m_CachedActorWorldMatrices[
					iBody])
			: MakePoseMatrix(
				tBody.vBoneToActorPosition,
				tBody.vBoneToActorRotation) *
				OwnerWorld;
		if (!ToPxTransform(
			ActorWorld,
			tActorPose))
		{
			return false;
		}

		m_RuntimeBodies[iBody].pActor
			->setGlobalPose(tActorPose, false);
	}

	for (size_t iJoint = 0;
		iJoint < m_RuntimeJoints.size();
		++iJoint)
	{
		auto* pJoint =
			m_RuntimeJoints[iJoint].pJoint;
		if (pJoint)
		{
			pJoint->setConstraintFlag(
				PxConstraintFlag::eDISABLE_CONSTRAINT,
				!m_tRagdoll.Joints[iJoint].bEnabled);
		}
	}

	m_bRagdollActive = false;
	return true;
}

_bool CComPxRagdoll::GetBodyWorldMatrix(
	size_t iBodyIndex,
	_float4x4& OutWorldMatrix) const
{
	if (iBodyIndex >= m_RuntimeBodies.size() ||
		!m_RuntimeBodies[iBodyIndex].pActor)
	{
		return false;
	}

	const PxTransform tPose =
		m_RuntimeBodies[iBodyIndex].pActor
			->getGlobalPose();
	const _float4 vRotation{
		tPose.q.x,
		tPose.q.y,
		tPose.q.z,
		tPose.q.w
	};
	XMStoreFloat4x4(
		&OutWorldMatrix,
		XMMatrixRotationQuaternion(
			XMLoadFloat4(&vRotation)) *
		XMMatrixTranslation(
			tPose.p.x,
			tPose.p.y,
			tPose.p.z));
	return true;
}

void CComPxRagdoll::DebugDraw() const
{
	if (!m_bDebugDraw ||
		m_RuntimeBodies.size() !=
			m_tRagdoll.Bodies.size())
	{
		return;
	}

	auto* pDebug =
		CGameInstance::Get().GetDbgLineRender();
	if (!pDebug)
		return;

	const _float4 vPreviousColor =
		pDebug->GetColor();
	const auto ePreviousDepthMode =
		pDebug->GetDepthMode();
	pDebug->SetDepthTest(
		m_bDebugDrawDepthTest);

	std::vector<_float4x4> BodyWorldMatrices(
		m_RuntimeBodies.size());
	std::vector<uint8_t> BodyWorldValid(
		m_RuntimeBodies.size(),
		0);

	for (size_t iBody = 0;
		iBody < m_RuntimeBodies.size();
		++iBody)
	{
		if (!GetBodyWorldMatrix(
			iBody,
			BodyWorldMatrices[iBody]))
		{
			continue;
		}

		BodyWorldValid[iBody] = 1;
		const _matrix BodyWorld =
			XMLoadFloat4x4(
				&BodyWorldMatrices[iBody]);
		const auto& tBody =
			m_tRagdoll.Bodies[iBody];

		pDebug->SetColor(
			{ 0.f, 0.85f, 1.f, 1.f });
		for (const auto& tShape :
			tBody.Shapes)
		{
			const _matrix ShapeWorld =
				MakePoseMatrix(
					tShape.vLocalPosition,
					tShape.vLocalRotation) *
				BodyWorld;

			switch (tShape.eType)
			{
			case PX_RAGDOLL_SHAPE_TYPE::BOX:
				pDebug->AddBox(
					tShape.vHalfExtents,
					ShapeWorld);
				break;

			case PX_RAGDOLL_SHAPE_TYPE::SPHERE:
				pDebug->AddSphere(
					tShape.fRadius,
					ShapeWorld);
				break;

			case PX_RAGDOLL_SHAPE_TYPE::CAPSULE:
				pDebug->AddCapsule(
					tShape.fRadius,
					tShape.fHalfHeight,
					ShapeWorld);
				break;
			}
		}

		pDebug->SetColor(
			{ 1.f, 0.2f, 0.8f, 1.f });
		pDebug->AddCross(
			{
				BodyWorldMatrices[iBody]._41,
				BodyWorldMatrices[iBody]._42,
				BodyWorldMatrices[iBody]._43
			},
			0.04f);
	}

	auto FindBodyIndex =
		[this](std::string_view sBodyName)
		-> size_t
		{
			const auto Iter =
				std::ranges::find_if(
					m_tRagdoll.Bodies,
					[sBodyName](
						const auto& tBody)
					{
						return tBody.sBodyName ==
							sBodyName;
					});
			return Iter ==
				m_tRagdoll.Bodies.end()
				? std::numeric_limits<
					size_t>::max()
				: static_cast<size_t>(
					std::distance(
						m_tRagdoll.Bodies.begin(),
						Iter));
		};

	for (const auto& tJoint :
		m_tRagdoll.Joints)
	{
		const size_t iParent =
			FindBodyIndex(
				tJoint.sParentBodyName);
		const size_t iChild =
			FindBodyIndex(
				tJoint.sChildBodyName);
		if (iParent >= BodyWorldMatrices.size() ||
			iChild >= BodyWorldMatrices.size() ||
			!BodyWorldValid[iParent] ||
			!BodyWorldValid[iChild])
		{
			continue;
		}

		const _matrix ParentJointWorld =
			MakePoseMatrix(
				tJoint.vParentLocalPosition,
				tJoint.vParentLocalRotation) *
			XMLoadFloat4x4(
				&BodyWorldMatrices[iParent]);
		const _matrix ChildJointWorld =
			MakePoseMatrix(
				tJoint.vChildLocalPosition,
				tJoint.vChildLocalRotation) *
			XMLoadFloat4x4(
				&BodyWorldMatrices[iChild]);

		_float4x4 ParentJointMatrix{};
		_float4x4 ChildJointMatrix{};
		XMStoreFloat4x4(
			&ParentJointMatrix,
			ParentJointWorld);
		XMStoreFloat4x4(
			&ChildJointMatrix,
			ChildJointWorld);

		const _float3 vParentPosition{
			ParentJointMatrix._41,
			ParentJointMatrix._42,
			ParentJointMatrix._43
		};
		const _float3 vChildPosition{
			ChildJointMatrix._41,
			ChildJointMatrix._42,
			ChildJointMatrix._43
		};
		const _float3 vParentBodyPosition{
			BodyWorldMatrices[iParent]._41,
			BodyWorldMatrices[iParent]._42,
			BodyWorldMatrices[iParent]._43
		};
		const _float3 vChildBodyPosition{
			BodyWorldMatrices[iChild]._41,
			BodyWorldMatrices[iChild]._42,
			BodyWorldMatrices[iChild]._43
		};

		pDebug->SetColor(
			{ 1.f, 0.85f, 0.f, 1.f });
		pDebug->AddLine(
			vParentBodyPosition,
			vParentPosition);
		pDebug->AddLine(
			vParentPosition,
			vChildPosition);
		pDebug->AddLine(
			vChildPosition,
			vChildBodyPosition);
		pDebug->AddLine(
			vParentBodyPosition,
			vChildBodyPosition);
		pDebug->AddCross(
			vParentPosition,
			0.06f);
		pDebug->AddCross(
			vChildPosition,
			0.04f);
	}

	pDebug->SetColor(vPreviousColor);
	pDebug->SetDepthMode(
		ePreviousDepthMode);
}

void CComPxRagdoll::ReleaseRuntimeBodies()
{
	ReleaseRuntimeJoints();
	m_bRagdollActive = false;

	auto* pPhysXManager =
		CGameInstance::Get().GetPhysXManager();

	for (auto& tRuntime : m_RuntimeBodies)
	{
		for (auto*& pShape : tRuntime.Shapes)
		{
			if (!pShape)
				continue;

			if (pPhysXManager)
				pPhysXManager->UnregisterShape(pShape);

			pShape->userData = nullptr;
			if (auto* pActor = pShape->getActor())
				pActor->detachShape(*pShape);

			pShape->release();
			pShape = nullptr;
		}
		tRuntime.Shapes.clear();

		if (!tRuntime.pActor)
			continue;

		if (pPhysXManager)
			pPhysXManager->UnregisterActor(
				tRuntime.pActor);

		tRuntime.pActor->userData = nullptr;
		if (auto* pScene =
			tRuntime.pActor->getScene())
		{
			pScene->removeActor(*tRuntime.pActor);
		}

		tRuntime.pActor->release();
		tRuntime.pActor = nullptr;
	}

	m_RuntimeBodies.clear();
	m_pMaterial.reset();
}

void CComPxRagdoll::ReleaseRuntimeJoints()
{
	for (auto& tRuntimeJoint : m_RuntimeJoints)
	{
		if (!tRuntimeJoint.pJoint)
			continue;

		tRuntimeJoint.pJoint->userData = nullptr;
		tRuntimeJoint.pJoint->release();
		tRuntimeJoint.pJoint = nullptr;
	}

	m_RuntimeJoints.clear();
}

void CComPxRagdoll::UpdateGUI()
{
	CComponent::UpdateGUI();
	ImGui::PushID(this);

	ImGui::Text(
		"Configured: %s",
		IsConfigured() ? "true" : "false");
	ImGui::Text("Data Version: %u", m_tRagdoll.iVersion);
	ImGui::Text(
		"Skeleton: %s",
		m_tRagdoll.sSkeletonTag.empty()
			? "<not assigned>"
			: m_tRagdoll.sSkeletonTag.c_str());
	ImGui::Text("Bodies: %zu", GetBodyCount());
	ImGui::Text("D6 Joints: %zu", GetJointCount());
	ImGui::Text(
		"Runtime Bodies: %zu",
		m_RuntimeBodies.size());
	ImGui::Text(
		"Runtime D6 Joints: %zu",
		m_RuntimeJoints.size());
	ImGui::Text(
		"Ragdoll Active: %s",
		m_bRagdollActive ? "true" : "false");
	ImGui::Text(
		"Skeleton Bound: %s",
		IsSkeletonBound() ? "true" : "false");
	ImGui::Text(
		"Cached Animation Pose: %s",
		m_bHasCachedAnimationPose
			? "true"
			: "false");
	ImGui::TextDisabled(
		"Animation and physics pose bridge ready.");
	ImGui::Separator();
	ImGui::Checkbox(
		"Debug Draw",
		&m_bDebugDraw);
	if (m_bDebugDraw)
	{
		ImGui::Checkbox(
			"Debug Depth Test",
			&m_bDebugDrawDepthTest);
		DebugDraw();
	}

	ImGui::PopID();
}

UPtr<CComPxRagdoll> CComPxRagdoll::Create()
{
	auto pInstance = ToUPtr(new CComPxRagdoll{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Create: CComPxRagdoll");
		return nullptr;
	}

	return pInstance;
}

UPtr<CPrototype> CComPxRagdoll::Clone(void* pArg)
{
	auto pInstance =
		ToUPtr(new CComPxRagdoll{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Clone: CComPxRagdoll");
		return nullptr;
	}

	return pInstance;
}

void CComPxRagdoll::Free()
{
	UnbindSkeleton();
	ReleaseRuntimeBodies();
	m_tRagdoll = {};
	CComponent::Free();
}
