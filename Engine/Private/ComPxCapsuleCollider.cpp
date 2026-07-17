#include "pch.h"
#include "ComPxCapsuleCollider.h"

#include "ComPxRigidBody.h"
#pragma push_macro("new")
#undef new

#include "PxPhysicsAPI.h"

#pragma pop_macro("new")

using namespace physx;

NS_USING(Engine)

void CComPxCapsuleCollider::UpdateGUI()
{
    CComPxCollider::UpdateGUI();
}

CComPxCapsuleCollider::CComPxCapsuleCollider()
{
}

CComPxCapsuleCollider::~CComPxCapsuleCollider()
{
}

HRESULT CComPxCapsuleCollider::Initialize(void* pArg)
{
	auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc)
		return E_FAIL;

	m_pResCapsuleGeo = pDesc->pResCapsuleGeo;
	if (!m_pResCapsuleGeo)
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

	auto* pGeometry = m_pResCapsuleGeo->GetCapsuleGeometry();
	auto* pMaterial = m_pResMaterial->GetMaterial();
	if (!pGeometry || !pMaterial)
		return E_FAIL;

	m_pShape = pPhysics->createShape(*pGeometry, *pMaterial);
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

	if (!RegisterShape(PHYSX_SHAPE_TYPE::CAPSULE))
		return E_FAIL;
	auto pActor = m_pComRigidBody->GetActor();
	if (!pActor)
		return E_FAIL;

	pActor->attachShape(*m_pShape);

	if (auto* dynamic = pActor->is<PxRigidDynamic>())
		PxRigidBodyExt::updateMassAndInertia(*dynamic, dynamic->getMass());
    return S_OK;
}

UPtr<CComPxCapsuleCollider> CComPxCapsuleCollider::Create()
{
    auto pInstance = ToUPtr(new CComPxCapsuleCollider{});
    if (FAILED(pInstance->InitializePrototype()))
    {
        MSG_BOX("Failed to Created : CComPxCapsuleCollider");
        return nullptr;
    }
    return pInstance;
}

UPtr<CPrototype> CComPxCapsuleCollider::Clone(void* pArg)
{
    auto pInstance = ToUPtr(new CComPxCapsuleCollider{ *this });
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned: CComPxCapsuleCollider");
        return nullptr;
    }
    return pInstance;
}

void CComPxCapsuleCollider::Free()
{
    CComPxCollider::Free();
}
