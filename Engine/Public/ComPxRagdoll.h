#pragma once
#include "Component.h"
#include "PhysXRagdollData.h"

NS_BEGIN(physx)
class PxRigidDynamic;
class PxShape;
class PxD6Joint;
NS_END

NS_BEGIN(Engine)

class CResPhysXMaterial;
class CResModel;

class ENGINE_DLL CComPxRagdoll final : public CComponent
{
public:
	struct DESC : public CComponent::DESC
	{
		PX_RAGDOLL_DESC tRagdoll{};
	};

public:
	DECLARE_DERIVED_TYPE(CComPxRagdoll, CComponent)

private:
	explicit CComPxRagdoll();
	explicit CComPxRagdoll(const CComPxRagdoll& Prototype);
	~CComPxRagdoll() override;

public:
	const PX_RAGDOLL_DESC& GetRagdollDesc() const
	{
		return m_tRagdoll;
	}

	size_t GetBodyCount() const
	{
		return m_tRagdoll.Bodies.size();
	}

	size_t GetJointCount() const
	{
		return m_tRagdoll.Joints.size();
	}

	_bool IsConfigured() const
	{
		return !m_tRagdoll.Bodies.empty();
	}

	_bool HasRuntimeBodies() const
	{
		return m_RuntimeBodies.size() ==
			m_tRagdoll.Bodies.size() &&
			!m_RuntimeBodies.empty();
	}

	_bool HasRuntimeJoints() const
	{
		return m_RuntimeJoints.size() ==
			m_tRagdoll.Joints.size();
	}

	_bool IsRagdollActive() const
	{
		return m_bRagdollActive;
	}

	_bool BindSkeleton(const CResModel& Model);
	void UnbindSkeleton();
	_bool IsSkeletonBound() const
	{
		return m_iBoundBoneCount > 0 &&
			m_BoneIndices.size() ==
				m_tRagdoll.Bodies.size();
	}

	_bool CacheAnimationPose(
		const std::vector<_float4x4>& CombinedBoneMatrices,
		_fmatrix ObjectWorld);
	_bool ApplyCachedPoseToKinematicBodies();
	_bool WritePhysicsPoseToBones(
		std::vector<_float4x4>& InOutCombinedBoneMatrices,
		_fmatrix ObjectWorld) const;

	_bool ActivateRagdoll(
		const _float3& vLinearVelocity = {},
		const _float3& vAngularVelocityRadians = {});
	_bool ResetToKinematicPose();
	_bool SyncKinematicPoseFromOwner();
	_bool GetBodyWorldMatrix(
		size_t iBodyIndex,
		_float4x4& OutWorldMatrix) const;
	void DebugDraw() const;

	void UpdateGUI() override;

private:
	HRESULT Initialize(void* pArg) override;
	static _bool ValidateDesc(const PX_RAGDOLL_DESC& tDesc);
	HRESULT BuildRuntimeBodies();
	HRESULT BuildRuntimeJoints();
	void ReleaseRuntimeJoints();
	void ReleaseRuntimeBodies();

private:
	struct RUNTIME_BODY
	{
		physx::PxRigidDynamic* pActor{};
		std::vector<physx::PxShape*> Shapes{};
	};

	struct RUNTIME_JOINT
	{
		physx::PxD6Joint* pJoint{};
		PX_JOINT_USER_DATA tUserData{};
	};

private:
	PX_RAGDOLL_DESC m_tRagdoll{};
	std::vector<RUNTIME_BODY> m_RuntimeBodies{};
	std::vector<RUNTIME_JOINT> m_RuntimeJoints{};
	SPtr<CResPhysXMaterial> m_pMaterial{};
	_bool m_bRagdollActive{};

	std::vector<uint32_t> m_BoneIndices{};
	std::vector<int32_t> m_BoneParentIndices{};
	std::vector<int32_t> m_BodyIndexByBoneIndex{};
	std::vector<_float4x4> m_CachedActorWorldMatrices{};
	size_t m_iBoundBoneCount{};
	_bool m_bHasCachedAnimationPose{};
	_bool m_bDebugDraw{};
	_bool m_bDebugDrawDepthTest{};

public:
	static UPtr<CComPxRagdoll> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	void Free() override;
};

NS_END
