#include "pch.h"
#include "ComPxSphereCollider.h"
#include "ComPxRigidBody.h"

#pragma push_macro("new")
#undef new

#include "PxPhysicsAPI.h"

#pragma pop_macro("new")

#include "ResPhysXSphereGeometry.h"
using namespace physx;

NS_USING(Engine)

void CComPxSphereCollider::UpdateGUI()
{
    CComPxCollider::UpdateGUI();
}

CComPxSphereCollider::CComPxSphereCollider()
{
}

CComPxSphereCollider::~CComPxSphereCollider()
{
}

HRESULT CComPxSphereCollider::Initialize(void* pArg)
{
    auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc)
		return E_FAIL;

	m_pResSphereGeo = pDesc->pResSphereGeo;
	if (!m_pResSphereGeo)
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

	auto* pGeometry = m_pResSphereGeo->GetSphereGeometry();
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

	if (!RegisterShape(PX_SHAPE_TYPE::SPHERE))
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

UPtr<CComPxSphereCollider> CComPxSphereCollider::Create()
{
    auto pInstance = ToUPtr(new CComPxSphereCollider{});
    if (FAILED(pInstance->InitializePrototype()))
    {
        MSG_BOX("Failed to Created : CComPxCapsuleCollider");
        return nullptr;
    }
    return pInstance;
}

UPtr<CPrototype> CComPxSphereCollider::Clone(void* pArg)
{
    auto pInstance = ToUPtr(new CComPxSphereCollider{ *this });
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned: CComPxSphereCollider");
        return nullptr;
    }
    return pInstance;
}

void CComPxSphereCollider::Free()
{
    CComPxCollider::Free();
}
