#include "pch.h"
#include "ComPxBoxCollider.h"
#include "ComPxRigidBody.h"
#include "ResPhysXBoxGeometry.h"
#include "ResPhysXMaterial.h"

#pragma push_macro("new")
#undef new

#include "PxPhysicsAPI.h"

#pragma pop_macro("new")

using namespace physx;

NS_USING(Engine)

void CComPxBoxCollider::UpdateGUI()
{
    CComPxCollider::UpdateGUI();

	ImGui::PushID(this);
	ImGui::TextUnformatted("Collider Type: Box");
	_float3 vHalfExtents = GetHalfExtents();
	if (ImGui::DragFloat3(
		"Half Extents", &vHalfExtents.x, 0.05f, 0.001f, 10000.f))
	{
		SetHalfExtents(vHalfExtents);
	}
	ImGui::PopID();
}

_bool CComPxBoxCollider::SetHalfExtents(const _float3& vHalfExtents)
{
	if (!m_pShape || vHalfExtents.x <= 0.f ||
		vHalfExtents.y <= 0.f || vHalfExtents.z <= 0.f)
	{
		return false;
	}

	m_pShape->setGeometry(PxBoxGeometry{
		vHalfExtents.x, vHalfExtents.y, vHalfExtents.z });

	if (auto* pActor = m_pShape->getActor())
	{
		if (auto* pDynamic = pActor->is<PxRigidDynamic>();
			pDynamic && !pDynamic->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
		{
			if (!PxRigidBodyExt::setMassAndUpdateInertia(*pDynamic, pDynamic->getMass()))
				return false;
		}
	}
	return true;
}

_float3 CComPxBoxCollider::GetHalfExtents() const
{
	if (!m_pShape)
		return {};
	const PxGeometryHolder geometryHolder = m_pShape->getGeometry();
	if (geometryHolder.getType() != PxGeometryType::eBOX)
		return {};
	const PxBoxGeometry& geometry = geometryHolder.box();
	return { geometry.halfExtents.x, geometry.halfExtents.y, geometry.halfExtents.z };
}

CComPxBoxCollider::CComPxBoxCollider()
{
}

CComPxBoxCollider::~CComPxBoxCollider()
{
}

HRESULT CComPxBoxCollider::Initialize(void* pArg)
{
    auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc)
		return E_FAIL;

	m_pResBoxGeo = pDesc->pResBoxGeo;
	if (!m_pResBoxGeo)
	{
		return E_FAIL;
	}
    if (FAILED(CComPxCollider::Initialize(pArg)))
    {
        return E_FAIL;
    }

	
	auto* pPhysics = CGameInstance::Get().PxGetPhysics();
	if (!pPhysics)
		return E_FAIL;

	auto* pGeometry = m_pResBoxGeo->GetBoxGeometry();
	auto* pMaterial = m_pResMaterial->GetMaterial();
	if (!pGeometry || !pMaterial)
		return E_FAIL;

	m_pShape = pPhysics->createShape(*pGeometry, *pMaterial, true);
	if (!m_pShape)
		return E_FAIL;

    if (pDesc->bIsTrigger)
    {
        m_pShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
        m_pShape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
    }

    if (pDesc->vLocalOffset.x != 0.f || pDesc->vLocalOffset.y != 0.f || pDesc->vLocalOffset.z != 0.f)
    {
        PxTransform tLocalPose = m_pShape->getLocalPose();
        tLocalPose.p += PxVec3(pDesc->vLocalOffset.x, pDesc->vLocalOffset.y, pDesc->vLocalOffset.z);
        m_pShape->setLocalPose(tLocalPose);
    }

	if (!RegisterShape(PX_SHAPE_TYPE::BOX))
		return E_FAIL;
    auto pActor = m_pComRigidBody->GetActor();
	if (!pActor)
		return E_FAIL;

    pActor->attachShape(*m_pShape);

    if (auto* dynamic = pActor->is<PxRigidDynamic>();
        dynamic && !dynamic->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
    {
        if (!PxRigidBodyExt::setMassAndUpdateInertia(*dynamic, dynamic->getMass()))
            return E_FAIL;
    }
    
    return S_OK;
}

UPtr<CComPxBoxCollider> CComPxBoxCollider::Create()
{
    auto pInstance = ToUPtr(new CComPxBoxCollider{});
    if (FAILED(pInstance->InitializePrototype()))
    {
        MSG_BOX("Failed to Created : CComPxBoxCollider");
        return nullptr;
    }
    return pInstance;
}

UPtr<CPrototype> CComPxBoxCollider::Clone(void* pArg)
{
    auto pInstance = ToUPtr(new CComPxBoxCollider{ *this });
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned: CComPxBoxCollider");
        return nullptr;
    }
    return pInstance;
}

void CComPxBoxCollider::Free()
{
    CComPxCollider::Free();
}
