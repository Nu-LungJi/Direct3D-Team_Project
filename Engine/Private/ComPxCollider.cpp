#include "pch.h"
#include "ComPxCollider.h"
#include "ComPxRigidBody.h"
#include "PhysXManager.h"

#pragma push_macro("new")
#undef new

#include "PxPhysicsAPI.h"

#pragma pop_macro("new")

using namespace physx;

NS_USING(Engine)

void CComPxCollider::UpdateGUI()
{
    CComponent::UpdateGUI();
	ImGui::PushID(this);
	//ImGui::Text("Collider Type: %s", GetTypeName());

	// ---- Material ----
	if (m_pResMaterial != nullptr)
	{
		const auto& m_pMaterial = m_pResMaterial->GetMaterial();
		float fStaticFriction = m_pMaterial->getStaticFriction();
		if (ImGui::DragFloat("Static Friction", &fStaticFriction, 0.01f, 0.0f, 10.0f))
			m_pMaterial->setStaticFriction(fStaticFriction);

		float fDynamicFriction = m_pMaterial->getDynamicFriction();
		if (ImGui::DragFloat("Dynamic Friction", &fDynamicFriction, 0.01f, 0.0f, 10.0f))
			m_pMaterial->setDynamicFriction(fDynamicFriction);

		float fRestitution = m_pMaterial->getRestitution();
		if (ImGui::DragFloat("Restitution", &fRestitution, 0.01f, 0.0f, 1.0f))
			m_pMaterial->setRestitution(fRestitution);
	}

	// ---- Trigger ----
	bool bIsTrigger = m_pShape->getFlags() & PxShapeFlag::eTRIGGER_SHAPE;
	if (ImGui::Checkbox("Is Trigger", &bIsTrigger))
	{
		m_pShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, !bIsTrigger);
		m_pShape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, bIsTrigger);
	}

	// ---- Local Pose (오프셋 + 회전) ----
	PxTransform tLocalPose = m_pShape->getLocalPose();
	float fOffset[3] = { tLocalPose.p.x, tLocalPose.p.y, tLocalPose.p.z };
	if (ImGui::DragFloat3("Local Offset", fOffset, 0.05f))
	{
		tLocalPose.p = PxVec3(fOffset[0], fOffset[1], fOffset[2]);
		m_pShape->setLocalPose(tLocalPose);
		m_pShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
		m_pShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
	}
	ImGui::Text("Local Rotation (Quat): %.3f, %.3f, %.3f, %.3f",
		tLocalPose.q.x, tLocalPose.q.y, tLocalPose.q.z, tLocalPose.q.w);

	ImGui::Separator();
	ImGui::PopID();
}

CComPxCollider::CComPxCollider()
{
}

CComPxCollider::~CComPxCollider()
{
}

HRESULT CComPxCollider::Initialize(void* pArg)
{
    auto* pDesc = static_cast<DESC*>(pArg);
    if (!pDesc->pComPxRigidBody)
    {
        MSG_BOX("RIGIDBODY NEED");
        return E_FAIL;
    }
    m_pComRigidBody = pDesc->pComPxRigidBody;
    if (FAILED(CComponent::Initialize(pArg)))
    {
        return E_FAIL;
    }

	m_pResMaterial = pDesc->pResMaterial;
    if (m_pResMaterial == nullptr)
        return E_FAIL;
	m_tFilter = pDesc->tFilter;

    return S_OK;
}

_bool CComPxCollider::RegisterShape(PX_SHAPE_TYPE eType, uint32_t iSubIndex)
{
	if (!m_pShape || !GetGameObject())
		return false;

	auto* pPhysXManager = CGameInstance::Get().GetPhysXManager();
	if (!pPhysXManager)
		return false;

	PxFilterData simulationFilter{};
	simulationFilter.word0 = m_tFilter.iLayer;
	simulationFilter.word1 = m_tFilter.iSimulationMask;
	m_pShape->setSimulationFilterData(simulationFilter);

	PxFilterData queryFilter{};
	queryFilter.word0 = m_tFilter.iLayer;
	queryFilter.word1 = m_tFilter.iQueryMask;
	m_pShape->setQueryFilterData(queryFilter);

	PX_SHAPE_USER_DATA userData{};
	userData.hGameObject = GetGameObject()->GetHandle();
	userData.eType = eType;
	userData.iSubIndex = iSubIndex;

	m_pShape->userData = nullptr;
	return pPhysXManager->RegisterShape(m_pShape, userData);
}

void CComPxCollider::Free()
{
	if (m_pShape)
	{
		if (auto* pPhysXManager = CGameInstance::Get().GetPhysXManager())
			pPhysXManager->UnregisterShape(m_pShape);

		m_pShape->userData = nullptr;
		if (auto* pActor = m_pShape->getActor())
			pActor->detachShape(*m_pShape);

		m_pShape->release();
		m_pShape = nullptr;
	}

	m_pComRigidBody = nullptr;
    CComponent::Free();
}
