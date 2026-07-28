#pragma once
#include "PhysXRagdollData.h"

NS_BEGIN(Engine)

class CResModel;

class ENGINE_DLL CPxRagdollAuthoring final
{
public:
	_bool Initialize(CResModel& Model);

	_bool IsReady() const
	{
		return !m_BindPoses.empty() &&
			!m_BoneIndexByName.empty();
	}

	_bool GetBoneBindPose(
		size_t iBoneIndex,
		_float4x4& OutBindPose) const;

	_bool GetRigidBindPose(
		const _char* pBoneName,
		_float4x4& OutRigidBindPose) const;

	_bool FitCapsuleBodyToChild(
		PX_RAGDOLL_DESC& tRagdoll,
		const _char* pBodyName,
		const _char* pBodyBoneName,
		const _char* pChildBoneName,
		_float fRadius) const;

	_bool FitPelvisBox(
		PX_RAGDOLL_DESC& tRagdoll,
		const _char* pBodyName,
		const _char* pBodyBoneName,
		const _char* pLeftLegBoneName,
		const _char* pRightLegBoneName,
		const _char* pSpineBoneName) const;

	_bool FitChestBox(
		PX_RAGDOLL_DESC& tRagdoll,
		const _char* pBodyName,
		const _char* pBodyBoneName,
		const _char* pLeftArmBoneName,
		const _char* pRightArmBoneName,
		const _char* pHeadBoneName) const;

	_bool FitFootBox(
		PX_RAGDOLL_DESC& tRagdoll,
		const _char* pBodyName,
		const _char* pFootBoneName,
		const _char* pToeBoneName) const;

	_bool AddBindPoseD6Joint(
		PX_RAGDOLL_DESC& tRagdoll,
		const _char* pJointName,
		const _char* pParentBodyName,
		const _char* pParentBoneName,
		const _char* pChildBodyName,
		const _char* pChildBoneName,
		_float fTwistDegrees,
		_float fSwingYDegrees,
		_float fSwingZDegrees) const;

	_bool ConfigureAnatomicalHinge(
		PX_RAGDOLL_DESC& tRagdoll,
		const _char* pJointName,
		const _char* pChildBoneName,
		const _char* pEndBoneName,
		_float3 vObjectBendDirection,
		_float fTwistLowerDegrees,
		_float fTwistUpperDegrees) const;

private:
	_bool GetBonePositionInBodySpace(
		const _char* pBodyBoneName,
		const _char* pTargetBoneName,
		_float3& vOutPosition) const;
	PX_RAGDOLL_SHAPE_DESC* FindSingleBodyShape(
		PX_RAGDOLL_DESC& tRagdoll,
		const _char* pBodyName) const;
	static _bool MakeFromToRotation(
		const _float3& vFromAxis,
		const _float3& vDirection,
		_float4& vOutRotation);

private:
	std::vector<_float4x4> m_BindPoses{};
	std::unordered_map<std::string, size_t>
		m_BoneIndexByName{};
};

NS_END
