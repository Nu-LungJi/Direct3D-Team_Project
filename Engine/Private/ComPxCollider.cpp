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

namespace
{
	PxQuat ToColliderNormalizedQuat(const _float4& vQuaternion)
	{
		PxQuat tQuat{ vQuaternion.x, vQuaternion.y, vQuaternion.z, vQuaternion.w };
		return tQuat.magnitudeSquared() > 0.f ? tQuat.getNormalized() : PxQuat{ PxIdentity };
	}
}

_bool CComPxCollider::SetTrigger(_bool bTrigger)
{
	if (!m_pShape)
		return false;

	m_bIsTrigger = bTrigger;
	if (!m_bSimulationEnabled)
		return true;

	if (bTrigger)
	{
		m_pShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
		m_pShape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
	}
	else
	{
		m_pShape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, false);
		m_pShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
	}

	return true;
}

_bool CComPxCollider::IsTrigger() const
{
	return m_bIsTrigger;
}

_bool CComPxCollider::SetEnabled(_bool bEnabled)
{
	return SetSimulationEnabled(bEnabled) && SetQueryEnabled(bEnabled);
}

_bool CComPxCollider::IsEnabled() const
{
	return IsSimulationEnabled() || IsQueryEnabled();
}

_bool CComPxCollider::SetSimulationEnabled(_bool bEnabled)
{
	if (!m_pShape)
		return false;

	m_bSimulationEnabled = bEnabled;
	if (!bEnabled)
	{
		m_pShape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, false);
		m_pShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
		return true;
	}

	if (m_bIsTrigger)
	{
		m_pShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
		m_pShape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
	}
	else
	{
		m_pShape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, false);
		m_pShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
	}

	return true;
}

_bool CComPxCollider::IsSimulationEnabled() const
{
	return m_bSimulationEnabled;
}

_bool CComPxCollider::SetQueryEnabled(_bool bEnabled)
{
	if (!m_pShape)
		return false;

	m_pShape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, bEnabled);
	return true;
}

_bool CComPxCollider::IsQueryEnabled() const
{
	return m_pShape && m_pShape->getFlags().isSet(PxShapeFlag::eSCENE_QUERY_SHAPE);
}

_bool CComPxCollider::SetLocalPosition(const _float3& vPosition)
{
	if (!m_pShape)
		return false;

	PxTransform tPose = m_pShape->getLocalPose();
	tPose.p = PxVec3{ vPosition.x, vPosition.y, vPosition.z };
	m_pShape->setLocalPose(tPose);
	return true;
}

_float3 CComPxCollider::GetLocalPosition() const
{
	if (!m_pShape)
		return {};

	const PxVec3 vPosition = m_pShape->getLocalPose().p;
	return { vPosition.x, vPosition.y, vPosition.z };
}

_bool CComPxCollider::SetLocalRotation(const _float4& vQuaternion)
{
	if (!m_pShape)
		return false;

	PxTransform tPose = m_pShape->getLocalPose();
	tPose.q = ToColliderNormalizedQuat(vQuaternion);
	m_pShape->setLocalPose(tPose);
	return true;
}

_float4 CComPxCollider::GetLocalRotation() const
{
	if (!m_pShape)
		return { 0.f, 0.f, 0.f, 1.f };

	const PxQuat vRotation = m_pShape->getLocalPose().q;
	return { vRotation.x, vRotation.y, vRotation.z, vRotation.w };
}

_bool CComPxCollider::SetFilter(const PX_FILTER_DESC& tFilter)
{
	if (!m_pShape)
		return false;

	m_tFilter = tFilter;

	PxFilterData tSimulationFilter{};
	tSimulationFilter.word0 = tFilter.iLayer;
	tSimulationFilter.word1 = tFilter.iSimulationMask;
	tSimulationFilter.word2 = tFilter.iNotifyFlags;
	m_pShape->setSimulationFilterData(tSimulationFilter);

	PxFilterData tQueryFilter{};
	tQueryFilter.word0 = tFilter.iLayer;
	tQueryFilter.word1 = tFilter.iQueryMask;
	m_pShape->setQueryFilterData(tQueryFilter);

	if (PxRigidActor* pActor = m_pShape->getActor())
	{
		if (PxScene* pScene = pActor->getScene())
			pScene->resetFiltering(*pActor);
	}

	return true;
}

void CComPxCollider::UpdateGUI()
{
	CComponent::UpdateGUI();
	ImGui::PushID(this);
	if (!m_pShape)
	{
		ImGui::Text("Shape: nullptr");
		ImGui::PopID();
		return;
	}

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
	bool bIsTrigger = IsTrigger();
	if (ImGui::Checkbox("Is Trigger", &bIsTrigger))
		SetTrigger(bIsTrigger);

	bool bSimulationEnabled = IsSimulationEnabled();
	if (ImGui::Checkbox("Simulation Enabled", &bSimulationEnabled))
		SetSimulationEnabled(bSimulationEnabled);

	bool bQueryEnabled = IsQueryEnabled();
	if (ImGui::Checkbox("Scene Query Enabled", &bQueryEnabled))
		SetQueryEnabled(bQueryEnabled);

	if (ImGui::Button("Enable All"))
		SetEnabled(true);
	ImGui::SameLine();
	if (ImGui::Button("Disable All"))
		SetEnabled(false);

	// ---- Local Pose (오프셋 + 회전) ----
	const _float3 vLocalPosition = GetLocalPosition();
	float fOffset[3] = { vLocalPosition.x, vLocalPosition.y, vLocalPosition.z };
	if (ImGui::DragFloat3("Local Offset", fOffset, 0.05f))
		SetLocalPosition({ fOffset[0], fOffset[1], fOffset[2] });

	const _float4 vLocalRotation = GetLocalRotation();
	float fRotation[4] = { vLocalRotation.x, vLocalRotation.y, vLocalRotation.z, vLocalRotation.w };
	if (ImGui::DragFloat4("Local Rotation (Quaternion)", fRotation, 0.01f))
		SetLocalRotation({ fRotation[0], fRotation[1], fRotation[2], fRotation[3] });

	PX_FILTER_DESC tFilter = GetFilter();
	bool bFilterChanged{};
	if (auto* pPhysXManager = CGameInstance::Get().GetPhysXManager())
	{
		bFilterChanged |= pPhysXManager->EditCollisionLayerGUI(
			"Layer", tFilter.iLayer);
		bFilterChanged |= pPhysXManager->EditCollisionLayerMaskGUI(
			"Simulation Mask", tFilter.iSimulationMask);
		bFilterChanged |= pPhysXManager->EditCollisionLayerMaskGUI(
			"Query Mask", tFilter.iQueryMask);
	}

	auto EditNotifyFlag = [&](const char* pLabel, uint32_t iFlag)
	{
		bool bEnabled = (tFilter.iNotifyFlags & iFlag) != 0;
		if (!ImGui::Checkbox(pLabel, &bEnabled))
			return;

		if (bEnabled)
			tFilter.iNotifyFlags |= iFlag;
		else
			tFilter.iNotifyFlags &= ~iFlag;
		bFilterChanged = true;
	};
	EditNotifyFlag("Notify Touch Found", PX_NOTIFY_TOUCH_FOUND);
	EditNotifyFlag("Notify Touch Lost", PX_NOTIFY_TOUCH_LOST);
	EditNotifyFlag("Notify Contact Points", PX_NOTIFY_CONTACT_POINTS);

	if (bFilterChanged)
		SetFilter(tFilter);

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
	m_iShapeSubIndex = pDesc->iShapeSubIndex;
	m_bIsTrigger = pDesc->bIsTrigger;

    return S_OK;
}

_bool CComPxCollider::RegisterShape(PX_SHAPE_TYPE eType)
{
	if (!m_pShape || !GetGameObject())
		return false;

	auto* pPhysXManager = CGameInstance::Get().GetPhysXManager();
	if (!pPhysXManager)
		return false;

	PxFilterData simulationFilter{};
	simulationFilter.word0 = m_tFilter.iLayer;
	simulationFilter.word1 = m_tFilter.iSimulationMask;
	simulationFilter.word2 = m_tFilter.iNotifyFlags;
	m_pShape->setSimulationFilterData(simulationFilter);

	PxFilterData queryFilter{};
	queryFilter.word0 = m_tFilter.iLayer;
	queryFilter.word1 = m_tFilter.iQueryMask;
	m_pShape->setQueryFilterData(queryFilter);

	PX_SHAPE_USER_DATA userData{};
	userData.hGameObject = GetGameObject()->GetHandle();
	userData.eType = eType;
	userData.iSubIndex = m_iShapeSubIndex;

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
