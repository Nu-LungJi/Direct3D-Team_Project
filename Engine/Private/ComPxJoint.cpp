#include "pch.h"
#include "ComPxJoint.h"
#include "ComPxCharacterController.h"
#include "ComPxRigidBody.h"
#include "GameObject.h"

#pragma push_macro("new")
#undef new

#include "PxPhysicsAPI.h"

#pragma pop_macro("new")

using namespace physx;

NS_USING(Engine)

namespace
{
	PxQuat ToJointNormalizedQuat(const _float4& vRotation)
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

	PxTransform ToJointPxTransform(
		const CComPxJoint::FRAME& tFrame)
	{
		return PxTransform{
			PxVec3{
				tFrame.vPosition.x,
				tFrame.vPosition.y,
				tFrame.vPosition.z },
			ToJointNormalizedQuat(tFrame.vRotation) };
	}

	PxJointActorIndex::Enum ToJointPxActorIndex(
		CComPxJoint::ACTOR eActor)
	{
		return eActor == CComPxJoint::ACTOR::A
			? PxJointActorIndex::eACTOR0
			: PxJointActorIndex::eACTOR1;
	}
}

CComPxJoint::CComPxJoint()
{
}

CComPxJoint::CComPxJoint(const CComPxJoint& Prototype)
	: CComponent{ Prototype }
{
}

CComPxJoint::~CComPxJoint()
{
}

HRESULT CComPxJoint::Initialize(void* pArg)
{
	auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc)
		return E_FAIL;

	if (FAILED(CComponent::Initialize(pArg)))
		return E_FAIL;

	const _bool bHasMultipleEndpointsA =
		pDesc->pRigidBodyA &&
		pDesc->pCharacterControllerA;
	const _bool bHasMultipleEndpointsB =
		pDesc->pRigidBodyB &&
		pDesc->pCharacterControllerB;
	if (bHasMultipleEndpointsA || bHasMultipleEndpointsB)
		return E_FAIL;

	const _bool bHasEndpointA =
		pDesc->pRigidBodyA ||
		pDesc->pCharacterControllerA;
	const _bool bHasEndpointB =
		pDesc->pRigidBodyB ||
		pDesc->pCharacterControllerB;
	if (!bHasEndpointA && !bHasEndpointB)
		return E_FAIL;

	PxRigidActor* pActorA = pDesc->pRigidBodyA
		? pDesc->pRigidBodyA->GetActor()
		: (pDesc->pCharacterControllerA
			? pDesc->pCharacterControllerA->GetActor()
			: nullptr);
	PxRigidActor* pActorB = pDesc->pRigidBodyB
		? pDesc->pRigidBodyB->GetActor()
		: (pDesc->pCharacterControllerB
			? pDesc->pCharacterControllerB->GetActor()
			: nullptr);

	if ((bHasEndpointA && !pActorA) ||
		(bHasEndpointB && !pActorB) ||
		(pActorA && pActorB && pActorA == pActorB))
	{
		return E_FAIL;
	}

	const _bool bHasDynamicActor =
		(pDesc->pRigidBodyA && pDesc->pRigidBodyA->IsDynamic()) ||
		(pDesc->pRigidBodyB && pDesc->pRigidBodyB->IsDynamic()) ||
		pDesc->pCharacterControllerA ||
		pDesc->pCharacterControllerB;
	if (!bHasDynamicActor)
		return E_FAIL;

	if (pDesc->fBreakForce < 0.f || pDesc->fBreakTorque < 0.f ||
		pDesc->fInvMassScaleA < 0.f || pDesc->fInvMassScaleB < 0.f ||
		pDesc->fInvInertiaScaleA < 0.f || pDesc->fInvInertiaScaleB < 0.f)
	{
		return E_FAIL;
	}

	m_pRigidBodyA = pDesc->pRigidBodyA;
	m_pRigidBodyB = pDesc->pRigidBodyB;
	m_pCharacterControllerA =
		pDesc->pCharacterControllerA;
	m_pCharacterControllerB =
		pDesc->pCharacterControllerB;
	m_tLocalFrameA = pDesc->tLocalFrameA;
	m_tLocalFrameB = pDesc->tLocalFrameB;
	m_tSettings = *pDesc;

	return S_OK;
}

_bool CComPxJoint::AttachJoint(PxJoint* pJoint)
{
	if (!pJoint || m_pJoint)
		return false;

	m_pJoint = pJoint;
	m_tUserData = {};
	m_tUserData.hJointOwner = m_pGameObject->GetHandle();
	m_tUserData.iJointSubIndex = m_tSettings.iJointSubIndex;

	if (m_pRigidBodyA && m_pRigidBodyA->GetGameObject())
	{
		m_tUserData.hActorA =
			m_pRigidBodyA->GetGameObject()->GetHandle();
	}
	else if (m_pCharacterControllerA &&
		m_pCharacterControllerA->GetGameObject())
	{
		m_tUserData.hActorA =
			m_pCharacterControllerA->GetGameObject()->GetHandle();
	}

	if (m_pRigidBodyB && m_pRigidBodyB->GetGameObject())
	{
		m_tUserData.hActorB =
			m_pRigidBodyB->GetGameObject()->GetHandle();
	}
	else if (m_pCharacterControllerB &&
		m_pCharacterControllerB->GetGameObject())
	{
		m_tUserData.hActorB =
			m_pCharacterControllerB->GetGameObject()->GetHandle();
	}

	m_pJoint->userData = &m_tUserData;

	m_pJoint->setBreakForce(
		m_tSettings.fBreakForce,
		m_tSettings.fBreakTorque);
	m_pJoint->setInvMassScale0(m_tSettings.fInvMassScaleA);
	m_pJoint->setInvMassScale1(m_tSettings.fInvMassScaleB);
	m_pJoint->setInvInertiaScale0(m_tSettings.fInvInertiaScaleA);
	m_pJoint->setInvInertiaScale1(m_tSettings.fInvInertiaScaleB);
	m_pJoint->setConstraintFlag(
		PxConstraintFlag::eCOLLISION_ENABLED,
		m_tSettings.bCollisionEnabled);
	m_pJoint->setConstraintFlag(
		PxConstraintFlag::eVISUALIZATION,
		m_tSettings.bVisualizationEnabled);
	m_pJoint->setConstraintFlag(
		PxConstraintFlag::eDISABLE_CONSTRAINT,
		!m_tSettings.bEnabled);

	if (m_pRigidBodyA)
		m_pRigidBodyA->RegisterJoint(this);
	if (m_pRigidBodyB)
		m_pRigidBodyB->RegisterJoint(this);
	if (m_pCharacterControllerA)
		m_pCharacterControllerA->RegisterJoint(this);
	if (m_pCharacterControllerB)
		m_pCharacterControllerB->RegisterJoint(this);

	return true;
}

PxRigidActor* CComPxJoint::GetActorA() const
{
	if (m_pRigidBodyA)
		return m_pRigidBodyA->GetActor();
	return m_pCharacterControllerA
		? m_pCharacterControllerA->GetActor()
		: nullptr;
}

PxRigidActor* CComPxJoint::GetActorB() const
{
	if (m_pRigidBodyB)
		return m_pRigidBodyB->GetActor();
	return m_pCharacterControllerB
		? m_pCharacterControllerB->GetActor()
		: nullptr;
}

_bool CComPxJoint::IsBroken() const
{
	return m_pJoint &&
		m_pJoint->getConstraintFlags().isSet(PxConstraintFlag::eBROKEN);
}

_bool CComPxJoint::SetEnabled(_bool bEnabled)
{
	if (!m_pJoint)
		return false;

	m_pJoint->setConstraintFlag(
		PxConstraintFlag::eDISABLE_CONSTRAINT,
		!bEnabled);
	m_tSettings.bEnabled = bEnabled;
	return true;
}

_bool CComPxJoint::IsEnabled() const
{
	return m_pJoint &&
		!m_pJoint->getConstraintFlags().isSet(
			PxConstraintFlag::eDISABLE_CONSTRAINT);
}

_bool CComPxJoint::SetCollisionEnabled(_bool bEnabled)
{
	if (!m_pJoint)
		return false;

	m_pJoint->setConstraintFlag(
		PxConstraintFlag::eCOLLISION_ENABLED,
		bEnabled);
	m_tSettings.bCollisionEnabled = bEnabled;
	return true;
}

_bool CComPxJoint::IsCollisionEnabled() const
{
	return m_pJoint &&
		m_pJoint->getConstraintFlags().isSet(
			PxConstraintFlag::eCOLLISION_ENABLED);
}

_bool CComPxJoint::SetVisualizationEnabled(_bool bEnabled)
{
	if (!m_pJoint)
		return false;

	m_pJoint->setConstraintFlag(
		PxConstraintFlag::eVISUALIZATION,
		bEnabled);
	m_tSettings.bVisualizationEnabled = bEnabled;
	return true;
}

_bool CComPxJoint::IsVisualizationEnabled() const
{
	return m_pJoint &&
		m_pJoint->getConstraintFlags().isSet(
			PxConstraintFlag::eVISUALIZATION);
}

_bool CComPxJoint::SetBreakForce(_float fForce, _float fTorque)
{
	if (!m_pJoint || fForce < 0.f || fTorque < 0.f)
		return false;

	m_pJoint->setBreakForce(fForce, fTorque);
	m_tSettings.fBreakForce = fForce;
	m_tSettings.fBreakTorque = fTorque;
	return true;
}

_bool CComPxJoint::GetBreakForce(
	_float& fOutForce,
	_float& fOutTorque) const
{
	if (!m_pJoint)
		return false;

	m_pJoint->getBreakForce(fOutForce, fOutTorque);
	return true;
}

_bool CComPxJoint::SetLocalFrame(
	ACTOR eActor,
	const FRAME& tFrame)
{
	if (!m_pJoint)
		return false;

	m_pJoint->setLocalPose(
		ToJointPxActorIndex(eActor),
		ToJointPxTransform(tFrame));

	if (eActor == ACTOR::A)
		m_tLocalFrameA = tFrame;
	else
		m_tLocalFrameB = tFrame;

	return true;
}

_bool CComPxJoint::SetInverseMassScale(
	ACTOR eActor,
	_float fScale)
{
	if (!m_pJoint || fScale < 0.f)
		return false;

	if (eActor == ACTOR::A)
	{
		m_pJoint->setInvMassScale0(fScale);
		m_tSettings.fInvMassScaleA = fScale;
	}
	else
	{
		m_pJoint->setInvMassScale1(fScale);
		m_tSettings.fInvMassScaleB = fScale;
	}

	return true;
}

_bool CComPxJoint::SetInverseInertiaScale(
	ACTOR eActor,
	_float fScale)
{
	if (!m_pJoint || fScale < 0.f)
		return false;

	if (eActor == ACTOR::A)
	{
		m_pJoint->setInvInertiaScale0(fScale);
		m_tSettings.fInvInertiaScaleA = fScale;
	}
	else
	{
		m_pJoint->setInvInertiaScale1(fScale);
		m_tSettings.fInvInertiaScaleB = fScale;
	}

	return true;
}

void CComPxJoint::UpdateGUI()
{
	CComponent::UpdateGUI();

	ImGui::Text("Valid: %s", IsValid() ? "True" : "False");
	ImGui::Text("Broken: %s", IsBroken() ? "True" : "False");

	if (!m_pJoint)
		return;

	bool bEnabled = IsEnabled();
	if (ImGui::Checkbox("Enabled", &bEnabled))
		SetEnabled(bEnabled);

	bool bCollisionEnabled = IsCollisionEnabled();
	if (ImGui::Checkbox(
		"Connected Actor Collision",
		&bCollisionEnabled))
	{
		SetCollisionEnabled(bCollisionEnabled);
	}

	bool bVisualizationEnabled = IsVisualizationEnabled();
	if (ImGui::Checkbox(
		"Joint Visualization",
		&bVisualizationEnabled))
	{
		SetVisualizationEnabled(bVisualizationEnabled);
	}

	_float fBreakForce{};
	_float fBreakTorque{};
	if (GetBreakForce(fBreakForce, fBreakTorque))
	{
		bool bChanged{};
		bChanged |= ImGui::DragFloat(
			"Break Force",
			&fBreakForce,
			1.f,
			0.f,
			std::numeric_limits<_float>::max());
		bChanged |= ImGui::DragFloat(
			"Break Torque",
			&fBreakTorque,
			1.f,
			0.f,
			std::numeric_limits<_float>::max());

		if (bChanged)
			SetBreakForce(fBreakForce, fBreakTorque);
	}
}

void CComPxJoint::ReleaseJoint()
{
	CComPxRigidBody* pRigidBodyA = m_pRigidBodyA;
	CComPxRigidBody* pRigidBodyB = m_pRigidBodyB;
	CComPxCharacterController* pCharacterControllerA =
		m_pCharacterControllerA;
	CComPxCharacterController* pCharacterControllerB =
		m_pCharacterControllerB;
	m_pRigidBodyA = nullptr;
	m_pRigidBodyB = nullptr;
	m_pCharacterControllerA = nullptr;
	m_pCharacterControllerB = nullptr;

	if (pRigidBodyA)
		pRigidBodyA->UnregisterJoint(this);
	if (pRigidBodyB)
		pRigidBodyB->UnregisterJoint(this);
	if (pCharacterControllerA)
		pCharacterControllerA->UnregisterJoint(this);
	if (pCharacterControllerB)
		pCharacterControllerB->UnregisterJoint(this);

	if (m_pJoint)
	{
		m_pJoint->userData = nullptr;
		m_pJoint->release();
		m_pJoint = nullptr;
	}
}

void CComPxJoint::OnRigidBodyReleased(
	CComPxRigidBody* pRigidBody)
{
	if (pRigidBody != m_pRigidBodyA &&
		pRigidBody != m_pRigidBodyB)
	{
		return;
	}

	ReleaseJoint();
}

void CComPxJoint::OnCharacterControllerReleased(
	CComPxCharacterController* pCharacterController)
{
	if (pCharacterController != m_pCharacterControllerA &&
		pCharacterController != m_pCharacterControllerB)
	{
		return;
	}

	ReleaseJoint();
}

void CComPxJoint::Free()
{
	ReleaseJoint();
	CComponent::Free();
}
