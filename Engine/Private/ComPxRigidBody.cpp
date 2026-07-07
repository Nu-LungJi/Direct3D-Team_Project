
#include "pch.h"
#include "ComPxRigidBody.h"

#ifdef _DEBUG
// 라이브러리 설정 전후로 매크로 잠시 해제
#undef new
#endif

#include "PxPhysicsAPI.h"

#ifdef _DEBUG
#define new DBG_NEW
#endif



using namespace physx;

void CComPxRigidBody::UpdateGUI()
{
    CComponent::UpdateGUI();

    if (m_pActor == nullptr)
    {
        ImGui::Text("Actor: nullptr");
        return;
    }

    ImGui::Text("Actor Type: %s", m_bIsDynamic ? "Dynamic" : "Static");

    // ---- Transform (공통) ----
    PxTransform tPose = m_pActor->getGlobalPose();
    float fPos[3] = { tPose.p.x, tPose.p.y, tPose.p.z };
    if (ImGui::DragFloat3("Position", fPos, 0.1f))
    {
        tPose.p = PxVec3(fPos[0], fPos[1], fPos[2]);
        m_pActor->setGlobalPose(tPose);
    }

    // 쿼터니언을 오일러로 보여주되, 직접 수정은 쿼터니언 그대로 두는 게 안전함 (짐벌락 방지)
    ImGui::Text("Rotation (Quat): %.3f, %.3f, %.3f, %.3f",
        tPose.q.x, tPose.q.y, tPose.q.z, tPose.q.w);

    // ---- Dynamic 전용 ----
    if (m_bIsDynamic)
    {
        PxRigidDynamic* pDynamic = static_cast<PxRigidDynamic*>(m_pActor);

        float fMass = pDynamic->getMass();
        if (ImGui::DragFloat("Mass", &fMass, 0.1f, 0.01f, 1000.0f))
            pDynamic->setMass(fMass);

        bool bIsKinematic = pDynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC;
        if (ImGui::Checkbox("Is Kinematic", &bIsKinematic))
            pDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, bIsKinematic);

        bool bGravity = !(pDynamic->getActorFlags() & PxActorFlag::eDISABLE_GRAVITY);
        if (ImGui::Checkbox("Use Gravity", &bGravity))
            pDynamic->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !bGravity);

        PxVec3 vLinVel = pDynamic->getLinearVelocity();
        ImGui::Text("Linear Velocity: %.3f, %.3f, %.3f", vLinVel.x, vLinVel.y, vLinVel.z);

        PxVec3 vAngVel = pDynamic->getAngularVelocity();
        ImGui::Text("Angular Velocity: %.3f, %.3f, %.3f", vAngVel.x, vAngVel.y, vAngVel.z);

        float fLinDamp = pDynamic->getLinearDamping();
        if (ImGui::DragFloat("Linear Damping", &fLinDamp, 0.01f, 0.0f, 10.0f))
            pDynamic->setLinearDamping(fLinDamp);

        float fAngDamp = pDynamic->getAngularDamping();
        if (ImGui::DragFloat("Angular Damping", &fAngDamp, 0.01f, 0.0f, 10.0f))
            pDynamic->setAngularDamping(fAngDamp);

        bool bIsSleeping = pDynamic->isSleeping();
        ImGui::Text("Sleeping: %s", bIsSleeping ? "true" : "false");
        if (!bIsSleeping && ImGui::Button("Force Sleep"))
            pDynamic->putToSleep();
    }

    // ---- Shape 개수 (부착된 Collider 확인용) ----
    PxU32 nNbShapes = m_pActor->getNbShapes();
    ImGui::Text("Attached Shapes: %d", nNbShapes);
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
    m_eType = pDesc->eType;
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
        pDynamic->setMass(pDesc->fMass);
        pDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, false);
        m_pActor = pDynamic;
        m_bIsDynamic = true;
    }
    break;
    case TYPE::KINEMATIC:
    {
        PxRigidDynamic* pDynamic = pPhysics->createRigidDynamic(tPose);
        pDynamic->setMass(pDesc->fMass);
        pDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
        m_pActor = pDynamic;
        m_bIsDynamic = true;
    }
    break;
    }

    if (m_pActor == nullptr)
        return E_FAIL;


    m_pActor->userData = this;
    CGameInstance::Get().PxGetScene()->addActor(*m_pActor);
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
    if (m_pActor != nullptr)
    {
        CGameInstance::Get().PxGetScene()->removeActor(*m_pActor);
        m_pActor->release();
        m_pActor = nullptr;
    }
	CComponent::Free();
}
