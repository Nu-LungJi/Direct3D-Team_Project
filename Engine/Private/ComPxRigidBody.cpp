
#include "pch.h"
#include "ComPxRigidBody.h"
#include "ComPxJoint.h"
#include "PhysXManager.h"

#pragma push_macro("new")
#undef new

#include "PxPhysicsAPI.h"

#pragma pop_macro("new")



using namespace physx;

namespace
{
	PxRigidDynamic* GetDynamicActor(PxRigidActor* pActor)
	{
		return pActor ? pActor->is<PxRigidDynamic>() : nullptr;
	}

	PxQuat ToRigidBodyNormalizedQuat(const _float4& vQuaternion)
	{
		PxQuat tQuat{ vQuaternion.x, vQuaternion.y, vQuaternion.z, vQuaternion.w };
		return tQuat.magnitudeSquared() > 0.f ? tQuat.getNormalized() : PxQuat{ PxIdentity };
	}
}

void CComPxRigidBody::RegisterJoint(CComPxJoint* pJoint)
{
	if (pJoint)
		m_Joints.insert(pJoint);
}

void CComPxRigidBody::UnregisterJoint(CComPxJoint* pJoint)
{
	if (pJoint)
		m_Joints.erase(pJoint);
}

void CComPxRigidBody::ReleaseConnectedJoints()
{
	while (!m_Joints.empty())
	{
		CComPxJoint* pJoint = *m_Joints.begin();
		if (!pJoint)
		{
			m_Joints.erase(m_Joints.begin());
			continue;
		}

		pJoint->OnRigidBodyReleased(this);
	}
}

_bool CComPxRigidBody::SetPosition(const _float3& vPosition)
{
	if (!m_pActor)
		return false;

	PxTransform tPose = m_pActor->getGlobalPose();
	tPose.p = PxVec3{ vPosition.x, vPosition.y, vPosition.z };
	m_pActor->setGlobalPose(tPose);
	return true;
}

_float3 CComPxRigidBody::GetPosition() const
{
	if (!m_pActor)
		return {};

	const PxVec3 vPosition = m_pActor->getGlobalPose().p;
	return { vPosition.x, vPosition.y, vPosition.z };
}

_bool CComPxRigidBody::SetRotation(const _float4& vQuaternion)
{
	if (!m_pActor)
		return false;

	PxTransform tPose = m_pActor->getGlobalPose();
	tPose.q = ToRigidBodyNormalizedQuat(vQuaternion);
	m_pActor->setGlobalPose(tPose);
	return true;
}

_float4 CComPxRigidBody::GetRotation() const
{
	if (!m_pActor)
		return { 0.f, 0.f, 0.f, 1.f };

	const PxQuat vRotation = m_pActor->getGlobalPose().q;
	return { vRotation.x, vRotation.y, vRotation.z, vRotation.w };
}

_bool CComPxRigidBody::SetPose(const _float3& vPosition, const _float4& vQuaternion)
{
	if (!m_pActor)
		return false;

	m_pActor->setGlobalPose(PxTransform{
		PxVec3{ vPosition.x, vPosition.y, vPosition.z },
		ToRigidBodyNormalizedQuat(vQuaternion) });
	return true;
}

_bool CComPxRigidBody::SetMass(_float fMass)
{
	auto* pDynamic = GetDynamicActor(m_pActor);
	if (!pDynamic || fMass <= 0.f)
		return false;

	if (!PxRigidBodyExt::setMassAndUpdateInertia(*pDynamic, fMass))
		return false;

	m_fMass = fMass;
	return true;
}

_bool CComPxRigidBody::SetKinematic(_bool bKinematic)
{
	auto* pDynamic = GetDynamicActor(m_pActor);
	if (!pDynamic)
		return false;

	const _bool bWasKinematic =
		pDynamic->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC);
	if (bWasKinematic == bKinematic)
	{
		m_eType = bKinematic ? TYPE::KINEMATIC : TYPE::DYNAMIC;
		return true;
	}

	pDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, bKinematic);
	if (!bKinematic &&
		!PxRigidBodyExt::setMassAndUpdateInertia(*pDynamic, m_fMass))
	{
		pDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
		return false;
	}

	m_eType = bKinematic ? TYPE::KINEMATIC : TYPE::DYNAMIC;
	return true;
}

_bool CComPxRigidBody::IsKinematic() const
{
	const auto* pDynamic = GetDynamicActor(m_pActor);
	return pDynamic && pDynamic->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC);
}

_float3 CComPxRigidBody::GetLinearVelocity() const
{
	const auto* pDynamic = GetDynamicActor(m_pActor);
	if (!pDynamic)
		return {};

	const PxVec3 vVelocity = pDynamic->getLinearVelocity();
	return { vVelocity.x, vVelocity.y, vVelocity.z };
}

_bool CComPxRigidBody::SetLinearVelocity(const _float3& vVelocity)
{
	auto* pDynamic = GetDynamicActor(m_pActor);
	if (!pDynamic || pDynamic->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
		return false;

	pDynamic->setLinearVelocity(PxVec3{ vVelocity.x, vVelocity.y, vVelocity.z });
	return true;
}

_float3 CComPxRigidBody::GetAngularVelocity() const
{
	const auto* pDynamic = GetDynamicActor(m_pActor);
	if (!pDynamic)
		return {};

	const PxVec3 vVelocity = pDynamic->getAngularVelocity();
	return { vVelocity.x, vVelocity.y, vVelocity.z };
}

_bool CComPxRigidBody::SetAngularVelocity(const _float3& vVelocity)
{
	auto* pDynamic = GetDynamicActor(m_pActor);
	if (!pDynamic || pDynamic->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
		return false;

	pDynamic->setAngularVelocity(PxVec3{ vVelocity.x, vVelocity.y, vVelocity.z });
	return true;
}

_bool CComPxRigidBody::AddForce(const _float3& vForce)
{
	auto* pDynamic = GetDynamicActor(m_pActor);
	if (!pDynamic || pDynamic->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
		return false;

	pDynamic->addForce(PxVec3{ vForce.x, vForce.y, vForce.z }, PxForceMode::eFORCE);
	return true;
}

_bool CComPxRigidBody::AddImpulse(const _float3& vImpulse)
{
	auto* pDynamic = GetDynamicActor(m_pActor);
	if (!pDynamic || pDynamic->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
		return false;

	pDynamic->addForce(PxVec3{ vImpulse.x, vImpulse.y, vImpulse.z }, PxForceMode::eIMPULSE);
	return true;
}

_bool CComPxRigidBody::AddTorque(const _float3& vTorque)
{
	auto* pDynamic = GetDynamicActor(m_pActor);
	if (!pDynamic || pDynamic->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
		return false;

	pDynamic->addTorque(PxVec3{ vTorque.x, vTorque.y, vTorque.z }, PxForceMode::eFORCE);
	return true;
}

_bool CComPxRigidBody::SetKinematicTarget(const _float3& vPosition)
{
	return SetKinematicTarget(vPosition, GetRotation());
}

_bool CComPxRigidBody::SetKinematicTarget(const _float3& vPosition, const _float4& vQuaternion)
{
	auto* pDynamic = GetDynamicActor(m_pActor);
	if (!pDynamic || !pDynamic->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
		return false;

	pDynamic->setKinematicTarget(PxTransform{
		PxVec3{ vPosition.x, vPosition.y, vPosition.z },
		ToRigidBodyNormalizedQuat(vQuaternion) });
	return true;
}

_bool CComPxRigidBody::SetGravityEnabled(_bool bEnabled)
{
	if (!m_pActor)
		return false;

	m_pActor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !bEnabled);
	return true;
}

_bool CComPxRigidBody::IsGravityEnabled() const
{
	return m_pActor && !m_pActor->getActorFlags().isSet(PxActorFlag::eDISABLE_GRAVITY);
}

_bool CComPxRigidBody::SetLinearDamping(_float fDamping)
{
	auto* pDynamic = GetDynamicActor(m_pActor);
	if (!pDynamic || fDamping < 0.f)
		return false;

	pDynamic->setLinearDamping(fDamping);
	return true;
}

_bool CComPxRigidBody::SetAngularDamping(_float fDamping)
{
	auto* pDynamic = GetDynamicActor(m_pActor);
	if (!pDynamic || fDamping < 0.f)
		return false;

	pDynamic->setAngularDamping(fDamping);
	return true;
}

_bool CComPxRigidBody::SetMaxDepenetrationVelocity(_float fVelocity)
{
	auto* pDynamic = GetDynamicActor(m_pActor);
	if (!pDynamic || fVelocity < 0.f)
		return false;

	pDynamic->setMaxDepenetrationVelocity(fVelocity);
	return true;
}

_bool CComPxRigidBody::WakeUp()
{
	auto* pDynamic = GetDynamicActor(m_pActor);
	if (!pDynamic || pDynamic->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
		return false;

	pDynamic->wakeUp();
	return true;
}

_bool CComPxRigidBody::PutToSleep()
{
	auto* pDynamic = GetDynamicActor(m_pActor);
	if (!pDynamic || pDynamic->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
		return false;

	pDynamic->putToSleep();
	return true;
}

_bool CComPxRigidBody::IsSleeping() const
{
	const auto* pDynamic = GetDynamicActor(m_pActor);
	return pDynamic && !pDynamic->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC) && pDynamic->isSleeping();
}

_bool CComPxRigidBody::SetSleepNotificationsEnabled(_bool bEnabled)
{
	if (!m_pActor || !m_bIsDynamic)
		return false;

	m_pActor->setActorFlag(PxActorFlag::eSEND_SLEEP_NOTIFIES, bEnabled);
	return true;
}

_bool CComPxRigidBody::IsSleepNotificationsEnabled() const
{
	return m_pActor && m_bIsDynamic &&
		m_pActor->getActorFlags().isSet(PxActorFlag::eSEND_SLEEP_NOTIFIES);
}

void CComPxRigidBody::UpdateGUI()
{
    CComponent::UpdateGUI();
	ImGui::PushID(this);

    if (m_pActor == nullptr)
    {
        ImGui::Text("Actor: nullptr");
		ImGui::PopID();
        return;
    }

	const char* pTypeName = "Static";
	if (m_eType == TYPE::DYNAMIC)
		pTypeName = "Dynamic";
	else if (m_eType == TYPE::KINEMATIC)
		pTypeName = "Kinematic";
	ImGui::Text("Actor Type: %s", pTypeName);

	const _float3 vPosition = GetPosition();
	float fPos[3] = { vPosition.x, vPosition.y, vPosition.z };
    if (ImGui::DragFloat3("Position", fPos, 0.1f))
		SetPosition({ fPos[0], fPos[1], fPos[2] });

	const _float4 vRotation = GetRotation();
	float fRotation[4] = { vRotation.x, vRotation.y, vRotation.z, vRotation.w };
	if (ImGui::DragFloat4("Rotation (Quaternion)", fRotation, 0.01f))
		SetRotation({ fRotation[0], fRotation[1], fRotation[2], fRotation[3] });

    if (m_bIsDynamic)
    {
        PxRigidDynamic* pDynamic = static_cast<PxRigidDynamic*>(m_pActor);

        float fMass = pDynamic->getMass();
        if (ImGui::DragFloat("Mass", &fMass, 0.1f, 0.01f, 1000.0f))
			SetMass(fMass);

		bool bIsKinematic = IsKinematic();
		if (ImGui::Checkbox("Is Kinematic", &bIsKinematic))
			SetKinematic(bIsKinematic);

		bool bGravity = !(pDynamic->getActorFlags() & PxActorFlag::eDISABLE_GRAVITY);
		if (ImGui::Checkbox("Use Gravity", &bGravity))
			pDynamic->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !bGravity);

		bool bSendSleepNotifies = IsSleepNotificationsEnabled();
		if (ImGui::Checkbox("Send Wake/Sleep Notifications", &bSendSleepNotifies))
			SetSleepNotificationsEnabled(bSendSleepNotifies);

		_float3 vLinearVelocity = GetLinearVelocity();
		float fLinearVelocity[3] = { vLinearVelocity.x, vLinearVelocity.y, vLinearVelocity.z };
		if (ImGui::DragFloat3("Linear Velocity", fLinearVelocity, 0.1f))
			SetLinearVelocity({ fLinearVelocity[0], fLinearVelocity[1], fLinearVelocity[2] });

		_float3 vAngularVelocity = GetAngularVelocity();
		float fAngularVelocity[3] = { vAngularVelocity.x, vAngularVelocity.y, vAngularVelocity.z };
		if (ImGui::DragFloat3("Angular Velocity", fAngularVelocity, 0.1f))
			SetAngularVelocity({ fAngularVelocity[0], fAngularVelocity[1], fAngularVelocity[2] });

        float fLinDamp = pDynamic->getLinearDamping();
        if (ImGui::DragFloat("Linear Damping", &fLinDamp, 0.01f, 0.0f, 10.0f))
            pDynamic->setLinearDamping(fLinDamp);

        float fAngDamp = pDynamic->getAngularDamping();
        if (ImGui::DragFloat("Angular Damping", &fAngDamp, 0.01f, 0.0f, 10.0f))
            pDynamic->setAngularDamping(fAngDamp);

        bool bIsSleeping = pDynamic->isSleeping();
        ImGui::Text("Sleeping: %s", bIsSleeping ? "true" : "false");
		if (!bIsSleeping && ImGui::Button("Put To Sleep"))
			PutToSleep();
		if (bIsSleeping && ImGui::Button("Wake Up"))
			WakeUp();

		if (!bIsKinematic)
		{
			static float s_fForce[3]{};
			static float s_fImpulse[3]{};
			static float s_fTorque[3]{};

			ImGui::Separator();
			ImGui::DragFloat3("Test Force", s_fForce, 0.1f);
			if (ImGui::Button("Add Force"))
				AddForce({ s_fForce[0], s_fForce[1], s_fForce[2] });

			ImGui::DragFloat3("Test Impulse", s_fImpulse, 0.1f);
			if (ImGui::Button("Add Impulse"))
				AddImpulse({ s_fImpulse[0], s_fImpulse[1], s_fImpulse[2] });

			ImGui::DragFloat3("Test Torque", s_fTorque, 0.1f);
			if (ImGui::Button("Add Torque"))
				AddTorque({ s_fTorque[0], s_fTorque[1], s_fTorque[2] });
		}
		else
		{
			static float s_fKinematicPosition[3]{};
			static float s_fKinematicRotation[4]{ 0.f, 0.f, 0.f, 1.f };

			ImGui::Separator();
			ImGui::DragFloat3("Kinematic Target Position", s_fKinematicPosition, 0.1f);
			ImGui::DragFloat4("Kinematic Target Rotation", s_fKinematicRotation, 0.01f);
			if (ImGui::Button("Set Kinematic Target"))
			{
				SetKinematicTarget(
					{ s_fKinematicPosition[0], s_fKinematicPosition[1], s_fKinematicPosition[2] },
					{ s_fKinematicRotation[0], s_fKinematicRotation[1], s_fKinematicRotation[2], s_fKinematicRotation[3] });
			}
		}
    }

    PxU32 nNbShapes = m_pActor->getNbShapes();
    ImGui::Text("Attached Shapes: %d", nNbShapes);
	ImGui::PopID();
}

CComPxRigidBody::CComPxRigidBody()
{
}
CComPxRigidBody::~CComPxRigidBody()
{
}
HRESULT CComPxRigidBody::Initialize(void* pArg)
{
    auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc)
		return E_FAIL;

    m_eType = pDesc->eType;
	m_fMass = pDesc->fMass;
    if (FAILED(CComponent::Initialize(pArg)))
    {
        return E_FAIL;
    }

    PxPhysics* pPhysics = CGameInstance::Get().PxGetPhysics();
    if (pPhysics == nullptr)
        return E_FAIL;

    PxTransform tPose(
        PxVec3(pDesc->vPosition.x, pDesc->vPosition.y, pDesc->vPosition.z),
        PxQuat(pDesc->vRotation.x, pDesc->vRotation.y, pDesc->vRotation.z, pDesc->vRotation.w));

    switch (pDesc->eType)
    {
    case TYPE::STATIC:
    {
        m_pActor = pPhysics->createRigidStatic(tPose);
        m_bIsDynamic = false;
    }
    break;
    case TYPE::DYNAMIC:
    {
        PxRigidDynamic* pDynamic = pPhysics->createRigidDynamic(tPose);
		if (!pDynamic)
			return E_FAIL;

        pDynamic->setMass(pDesc->fMass);
        pDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, false);
        m_pActor = pDynamic;
        m_bIsDynamic = true;
    }
    break;
    case TYPE::KINEMATIC:
    {
        PxRigidDynamic* pDynamic = pPhysics->createRigidDynamic(tPose);
		if (!pDynamic)
			return E_FAIL;

        pDynamic->setMass(pDesc->fMass);
        pDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
        m_pActor = pDynamic;
        m_bIsDynamic = true;
    }
    break;
    }

    if (m_pActor == nullptr)
        return E_FAIL;

	if (m_bIsDynamic)
		m_pActor->setActorFlag(
			PxActorFlag::eSEND_SLEEP_NOTIFIES,
			pDesc->bSendSleepNotifies);

	auto* pScene = CGameInstance::Get().PxGetScene();
	if (!pScene)
		return E_FAIL;

	auto* pPhysXManager = CGameInstance::Get().GetPhysXManager();
	PX_ACTOR_USER_DATA userData{};
	userData.hGameObject = GetGameObject()->GetHandle();
	userData.eType = PX_ACTOR_TYPE::RIGID_BODY;
	if (!pPhysXManager || !pPhysXManager->RegisterActor(m_pActor, userData))
		return E_FAIL;

	m_pActor->userData = nullptr;
	pScene->addActor(*m_pActor);
    return S_OK;
}
UPtr<CComPxRigidBody> CComPxRigidBody::Create()
{
    auto pInstance = ToUPtr(new CComPxRigidBody{});
    if (FAILED(pInstance->InitializePrototype()))
    {
        MSG_BOX("Failed to Created : CComPxRigidBody");
        return nullptr;
    }
    return pInstance;
}

UPtr<CPrototype> CComPxRigidBody::Clone(void* pArg)
{
    auto pInstance = ToUPtr(new CComPxRigidBody{ *this });
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned: CComPxRigidBody");
        return nullptr;
    }
    return pInstance;
}

void CComPxRigidBody::Free()
{
	ReleaseConnectedJoints();

    if (m_pActor != nullptr)
    {
		if (auto* pPhysXManager = CGameInstance::Get().GetPhysXManager())
			pPhysXManager->UnregisterActor(m_pActor);

		m_pActor->userData = nullptr;
        CGameInstance::Get().PxGetScene()->removeActor(*m_pActor);
        m_pActor->release();
        m_pActor = nullptr;
    }
	CComponent::Free();
}
