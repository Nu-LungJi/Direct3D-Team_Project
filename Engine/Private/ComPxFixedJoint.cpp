#include "pch.h"
#include "ComPxFixedJoint.h"
#include "ComPxCharacterController.h"
#include "ComPxRigidBody.h"

#pragma push_macro("new")
#undef new

#include "PxPhysicsAPI.h"

#pragma pop_macro("new")

using namespace physx;

NS_USING(Engine)

namespace
{
	PxQuat ToFixedJointNormalizedQuat(
		const _float4& vRotation)
	{
		const PxQuat tRotation{
			vRotation.x,
			vRotation.y,
			vRotation.z,
			vRotation.w };

		return tRotation.magnitudeSquared() > 0.f
			? tRotation.getNormalized()
			: PxQuat{ PxIdentity };
	}

	PxTransform ToFixedJointPxTransform(
		const CComPxJoint::FRAME& tFrame)
	{
		return PxTransform{
			PxVec3{
				tFrame.vPosition.x,
				tFrame.vPosition.y,
				tFrame.vPosition.z },
			ToFixedJointNormalizedQuat(tFrame.vRotation) };
	}

	CComPxJoint::FRAME ToFixedJointFrame(
		const PxTransform& tTransform)
	{
		return {
			.vPosition = {
				tTransform.p.x,
				tTransform.p.y,
				tTransform.p.z },
			.vRotation = {
				tTransform.q.x,
				tTransform.q.y,
				tTransform.q.z,
				tTransform.q.w }
		};
	}

	void PreserveFixedJointCurrentPose(
		CComPxJoint::DESC& tDesc)
	{
		PxRigidActor* pActorA =
			tDesc.pRigidBodyA
			? tDesc.pRigidBodyA->GetActor()
			: (tDesc.pCharacterControllerA
				? tDesc.pCharacterControllerA->GetActor()
				: nullptr);
		PxRigidActor* pActorB =
			tDesc.pRigidBodyB
			? tDesc.pRigidBodyB->GetActor()
			: (tDesc.pCharacterControllerB
				? tDesc.pCharacterControllerB->GetActor()
				: nullptr);

		if (pActorA && pActorB)
		{
			const PxTransform tPoseA = pActorA->getGlobalPose();
			const PxTransform tPoseB = pActorB->getGlobalPose();

			tDesc.tLocalFrameA = {};
			tDesc.tLocalFrameB =
				ToFixedJointFrame(tPoseB.getInverse() * tPoseA);
		}
		else if (pActorA)
		{
			tDesc.tLocalFrameA = {};
			tDesc.tLocalFrameB =
				ToFixedJointFrame(pActorA->getGlobalPose());
		}
		else if (pActorB)
		{
			tDesc.tLocalFrameA =
				ToFixedJointFrame(pActorB->getGlobalPose());
			tDesc.tLocalFrameB = {};
		}
	}
}

CComPxFixedJoint::CComPxFixedJoint()
{
}

CComPxFixedJoint::CComPxFixedJoint(
	const CComPxFixedJoint& Prototype)
	: CComPxJoint{ Prototype }
{
}

CComPxFixedJoint::~CComPxFixedJoint()
{
}

HRESULT CComPxFixedJoint::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc)
		return E_FAIL;

	CComPxJoint::DESC tJointDesc = *pDesc;
	if (pDesc->bPreserveCurrentPose)
		PreserveFixedJointCurrentPose(tJointDesc);

	if (FAILED(CComPxJoint::Initialize(&tJointDesc)))
		return E_FAIL;

	auto* pPhysics = CGameInstance::Get().PxGetPhysics();
	if (!pPhysics)
		return E_FAIL;

	PxFixedJoint* pJoint = PxFixedJointCreate(
		*pPhysics,
		GetActorA(),
		ToFixedJointPxTransform(GetLocalFrameA()),
		GetActorB(),
		ToFixedJointPxTransform(GetLocalFrameB()));
	if (!pJoint)
		return E_FAIL;

	if (!AttachJoint(pJoint))
	{
		pJoint->release();
		return E_FAIL;
	}

	return S_OK;
}

UPtr<CComPxFixedJoint> CComPxFixedJoint::Create()
{
	auto pInstance = ToUPtr(new CComPxFixedJoint{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CComPxFixedJoint");
		return nullptr;
	}

	return pInstance;
}

UPtr<CPrototype> CComPxFixedJoint::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CComPxFixedJoint{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CComPxFixedJoint");
		return nullptr;
	}

	return pInstance;
}

void CComPxFixedJoint::Free()
{
	CComPxJoint::Free();
}
