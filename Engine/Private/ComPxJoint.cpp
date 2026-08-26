#include "pch.h"
#include "ComPxJoint.h"
#include "ComPxCharacterController.h"
#include "ComPxRigidBody.h"
#include "DbgLineRender.h"
#include "GameInstance.h"
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

	CComPxJoint::FRAME FromJointPxTransform(
		const PxTransform& tTransform)
	{
		CComPxJoint::FRAME tFrame{};
		tFrame.vPosition = {
			tTransform.p.x,
			tTransform.p.y,
			tTransform.p.z
		};
		tFrame.vRotation = {
			tTransform.q.x,
			tTransform.q.y,
			tTransform.q.z,
			tTransform.q.w
		};
		return tFrame;
	}

	PxJointActorIndex::Enum ToJointPxActorIndex(
		CComPxJoint::ACTOR eActor)
	{
		return eActor == CComPxJoint::ACTOR::A
			? PxJointActorIndex::eACTOR0
			: PxJointActorIndex::eACTOR1;
	}

	_matrix ToJointWorldMatrix(const PxTransform& tTransform)
	{
		const _vector vRotation = XMVectorSet(
			tTransform.q.x,
			tTransform.q.y,
			tTransform.q.z,
			tTransform.q.w);
		return XMMatrixRotationQuaternion(vRotation) *
			XMMatrixTranslation(
				tTransform.p.x,
				tTransform.p.y,
				tTransform.p.z);
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

	if (CanRelocateWorldAnchoredRigidBody())
	{
		CComPxRigidBody* pRigidBody = m_pRigidBodyA;
		if (!pRigidBody)
			pRigidBody = m_pRigidBodyB;

		m_vGUIPlacementPosition = pRigidBody->GetPosition();
		m_vGUIPlacementRotation = pRigidBody->GetRotation();
		m_vAppliedPlacementPosition = m_vGUIPlacementPosition;
		m_vAppliedPlacementRotation = m_vGUIPlacementRotation;
		m_bGUIPlacementInitialized = true;
	}

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
	{
		m_tLocalFrameA = tFrame;
		m_tSettings.tLocalFrameA = tFrame;
	}
	else
	{
		m_tLocalFrameB = tFrame;
		m_tSettings.tLocalFrameB = tFrame;
	}

	return true;
}

_bool CComPxJoint::CanRelocateWorldAnchoredRigidBody() const
{
	if (!m_pJoint || m_pCharacterControllerA || m_pCharacterControllerB)
		return false;

	const _bool bWorldA = m_pRigidBodyA == nullptr;
	const _bool bWorldB = m_pRigidBodyB == nullptr;
	return (bWorldA && m_pRigidBodyB) ||
		(bWorldB && m_pRigidBodyA);
}

_bool CComPxJoint::RelocateWorldAnchoredRigidBody(
	const _float3& vPosition,
	const _float4& vRotation)
{
	if (!CanRelocateWorldAnchoredRigidBody())
		return false;

	CComPxRigidBody* pRigidBody = m_pRigidBodyA;
	ACTOR eWorldActor = ACTOR::B;
	const FRAME* pRigidBodyLocalFrame = &m_tLocalFrameA;
	FRAME tPreviousWorldFrame = m_tLocalFrameB;
	if (!pRigidBody)
	{
		pRigidBody = m_pRigidBodyB;
		eWorldActor = ACTOR::A;
		pRigidBodyLocalFrame = &m_tLocalFrameB;
		tPreviousWorldFrame = m_tLocalFrameA;
	}

	const _float3 vPreviousPosition = pRigidBody->GetPosition();
	const _float4 vPreviousRotation = pRigidBody->GetRotation();

	const PxTransform tNewRigidBodyPose{
		PxVec3{ vPosition.x, vPosition.y, vPosition.z },
		ToJointNormalizedQuat(vRotation)
	};
	const FRAME tNewWorldFrame = FromJointPxTransform(
		tNewRigidBodyPose *
		ToJointPxTransform(*pRigidBodyLocalFrame));
	const _float4 vNormalizedRotation{
		tNewRigidBodyPose.q.x,
		tNewRigidBodyPose.q.y,
		tNewRigidBodyPose.q.z,
		tNewRigidBodyPose.q.w
	};

	const _bool bWasEnabled = IsEnabled();
	if (bWasEnabled && !SetEnabled(false))
		return false;

	_bool bSucceeded = pRigidBody->SetPose(
		vPosition,
		vNormalizedRotation);
	if (bSucceeded)
		bSucceeded = SetLocalFrame(eWorldActor, tNewWorldFrame);
	if (bSucceeded && !pRigidBody->IsKinematic())
		bSucceeded = pRigidBody->SetLinearVelocity({});
	if (bSucceeded && !pRigidBody->IsKinematic())
		bSucceeded = pRigidBody->SetAngularVelocity({});

	if (!bSucceeded)
	{
		pRigidBody->SetPose(vPreviousPosition, vPreviousRotation);
		SetLocalFrame(eWorldActor, tPreviousWorldFrame);
	}

	if (bWasEnabled && !SetEnabled(true))
		bSucceeded = false;
	if (bSucceeded && !pRigidBody->IsKinematic())
		bSucceeded = pRigidBody->WakeUp();
	if (bSucceeded)
	{
		m_vAppliedPlacementPosition = vPosition;
		m_vAppliedPlacementRotation = vNormalizedRotation;
		m_vGUIPlacementPosition = m_vAppliedPlacementPosition;
		m_vGUIPlacementRotation = m_vAppliedPlacementRotation;
	}

	return bSucceeded;
}

_bool CComPxJoint::ResetWorldAnchoredRigidBodyToPlacement()
{
	if (!m_bGUIPlacementInitialized)
		return false;

	return RelocateWorldAnchoredRigidBody(
		m_vAppliedPlacementPosition,
		m_vAppliedPlacementRotation);
}

_bool CComPxJoint::SetWorldAnchoredRigidBodyLocalFrame(
	const FRAME& tRigidBodyLocalFrame)
{
	if (!CanRelocateWorldAnchoredRigidBody())
		return false;

	CComPxRigidBody* pRigidBody = m_pRigidBodyA;
	ACTOR eRigidBodyActor = ACTOR::A;
	ACTOR eWorldActor = ACTOR::B;
	FRAME tPreviousRigidBodyFrame = m_tLocalFrameA;
	FRAME tPreviousWorldFrame = m_tLocalFrameB;
	if (!pRigidBody)
	{
		pRigidBody = m_pRigidBodyB;
		eRigidBodyActor = ACTOR::B;
		eWorldActor = ACTOR::A;
		tPreviousRigidBodyFrame = m_tLocalFrameB;
		tPreviousWorldFrame = m_tLocalFrameA;
	}

	const PxTransform tWorldFrame =
		pRigidBody->GetActor()->getGlobalPose() *
		ToJointPxTransform(tRigidBodyLocalFrame);
	const FRAME tMatchingWorldFrame =
		FromJointPxTransform(tWorldFrame);

	const _bool bWasEnabled = IsEnabled();
	if (bWasEnabled && !SetEnabled(false))
		return false;

	_bool bSucceeded = SetLocalFrame(
		eRigidBodyActor,
		tRigidBodyLocalFrame);
	if (bSucceeded)
	{
		bSucceeded = SetLocalFrame(
			eWorldActor,
			tMatchingWorldFrame);
	}

	if (!bSucceeded)
	{
		SetLocalFrame(eRigidBodyActor, tPreviousRigidBodyFrame);
		SetLocalFrame(eWorldActor, tPreviousWorldFrame);
	}

	if (bWasEnabled && !SetEnabled(true))
		bSucceeded = false;
	if (bSucceeded && !pRigidBody->IsKinematic())
		bSucceeded = pRigidBody->WakeUp();

	return bSucceeded;
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

	if (CanRelocateWorldAnchoredRigidBody())
	{
		FRAME tRigidBodyLocalFrame = m_pRigidBodyA
			? m_tLocalFrameA
			: m_tLocalFrameB;

		ImGui::Separator();
		ImGui::TextUnformatted("Joint Anchor On RigidBody");
		_bool bAnchorChanged = ImGui::DragFloat3(
			"Anchor Local Position",
			&tRigidBodyLocalFrame.vPosition.x,
			0.01f);
		bAnchorChanged |= ImGui::DragFloat4(
			"Anchor Local Rotation (Quaternion)",
			&tRigidBodyLocalFrame.vRotation.x,
			0.01f);
		if (bAnchorChanged)
		{
			SetWorldAnchoredRigidBodyLocalFrame(
				tRigidBodyLocalFrame);
		}

		if (!m_bGUIPlacementInitialized)
		{
			CComPxRigidBody* pRigidBody = m_pRigidBodyA;
			if (!pRigidBody)
				pRigidBody = m_pRigidBodyB;

			m_vGUIPlacementPosition = pRigidBody->GetPosition();
			m_vGUIPlacementRotation = pRigidBody->GetRotation();
			m_vAppliedPlacementPosition = m_vGUIPlacementPosition;
			m_vAppliedPlacementRotation = m_vGUIPlacementRotation;
			m_bGUIPlacementInitialized = true;
		}

		ImGui::Separator();
		ImGui::TextUnformatted("World-Anchored Joint Placement");
		_bool bPlacementChanged = ImGui::DragFloat3(
			"Placement Position",
			&m_vGUIPlacementPosition.x,
			0.1f);
		bPlacementChanged |= ImGui::DragFloat4(
			"Placement Rotation (Quaternion)",
			&m_vGUIPlacementRotation.x,
			0.01f);

		if (bPlacementChanged)
		{
			RelocateWorldAnchoredRigidBody(
				m_vGUIPlacementPosition,
				m_vGUIPlacementRotation);
		}

		if (ImGui::Button("Read Actor Pose"))
		{
			CComPxRigidBody* pRigidBody = m_pRigidBodyA;
			if (!pRigidBody)
				pRigidBody = m_pRigidBodyB;

			m_vGUIPlacementPosition = pRigidBody->GetPosition();
			m_vGUIPlacementRotation = pRigidBody->GetRotation();
		}
	}

	ImGui::Separator();
	ImGui::Checkbox(
		"Debug Joint Frames",
		&m_bDebugDrawJointFrames);
	if (m_bDebugDrawJointFrames)
	{
		ImGui::SameLine();
		ImGui::Checkbox(
			"Joint Depth Test",
			&m_bDebugDrawDepthTest);
		ImGui::DragFloat(
			"Joint Frame Scale",
			&m_fDebugJointFrameScale,
			0.01f,
			0.05f,
			5.f,
			"%.2f");
		DrawDebugJointFrames();
	}
}

void CComPxJoint::DrawDebugJointFrames() const
{
	if (!m_pJoint)
		return;

	CDbgLineRender* pDebug =
		CGameInstance::Get().GetDbgLineRender();
	if (!pDebug)
		return;

	const PxRigidActor* pActorA = GetActorA();
	const PxRigidActor* pActorB = GetActorB();
	const PxTransform tLocalFrameA = m_pJoint->getLocalPose(
		PxJointActorIndex::eACTOR0);
	const PxTransform tLocalFrameB = m_pJoint->getLocalPose(
		PxJointActorIndex::eACTOR1);
	const PxTransform tWorldFrameA = pActorA
		? pActorA->getGlobalPose() * tLocalFrameA
		: tLocalFrameA;
	const PxTransform tWorldFrameB = pActorB
		? pActorB->getGlobalPose() * tLocalFrameB
		: tLocalFrameB;

	const _float3 vAnchorA{
		tWorldFrameA.p.x,
		tWorldFrameA.p.y,
		tWorldFrameA.p.z
	};
	const _float3 vAnchorB{
		tWorldFrameB.p.x,
		tWorldFrameB.p.y,
		tWorldFrameB.p.z
	};

	const _float4 vPreviousColor = pDebug->GetColor();
	const DBG_LINE_DEPTH_MODE ePreviousDepth = pDebug->GetDepthMode();
	pDebug->SetDepthTest(m_bDebugDrawDepthTest);
	pDebug->AddLine(
		vAnchorA,
		vAnchorB,
		{ 1.f, 0.f, 1.f, 1.f });

	pDebug->SetColor({ 1.f, 0.25f, 0.1f, 1.f });
	pDebug->AddCross(vAnchorA, m_fDebugJointFrameScale * 0.2f);
	pDebug->AddAxis(
		m_fDebugJointFrameScale,
		ToJointWorldMatrix(tWorldFrameA));

	pDebug->SetColor({ 0.1f, 0.75f, 1.f, 1.f });
	pDebug->AddCross(vAnchorB, m_fDebugJointFrameScale * 0.2f);
	pDebug->AddAxis(
		m_fDebugJointFrameScale,
		ToJointWorldMatrix(tWorldFrameB));

	pDebug->SetColor(vPreviousColor);
	pDebug->SetDepthMode(ePreviousDepth);
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
