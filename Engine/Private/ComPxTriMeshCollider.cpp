#include "pch.h"
#include "ComPxTriMeshCollider.h"

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

NS_USING(Engine)

void CComPxTriMeshCollider::UpdateGUI()
{
    CComPxCollider::UpdateGUI();
}

CComPxTriMeshCollider::CComPxTriMeshCollider()
{
}

CComPxTriMeshCollider::~CComPxTriMeshCollider()
{
}

HRESULT CComPxTriMeshCollider::Initialize(void* pArg)
{
    auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc)
		return E_FAIL;

    if (pDesc->pResTriMesh == nullptr)
        return E_FAIL;

    if (FAILED(CComPxCollider::Initialize(pArg)))
    {
        return E_FAIL;
    }

    m_pResTriMesh = pDesc->pResTriMesh;
    auto* pPhysics = CGameInstance::Get().PxGetPhysics();
	auto* pTriMesh = m_pResTriMesh->GetTriMesh();
	auto* pMaterial = m_pResMaterial->GetMaterial();
	if (!pPhysics || !pTriMesh || !pMaterial)
		return E_FAIL;

    m_pShape = pPhysics->createShape(PxTriangleMeshGeometry(pTriMesh), *pMaterial);
    if (m_pShape == nullptr)
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

    m_pShape->userData = this;

    auto pActor = m_pComRigidBody->GetActor();
	if (!pActor)
		return E_FAIL;

    //if (pActor->is<PxRigidDynamic>() != nullptr)
    //{
    //    bool bIsKinematic = (static_cast<PxRigidDynamic*>(pActor)->getRigidBodyFlags()
    //        & PxRigidBodyFlag::eKINEMATIC) != 0;
    //    if (!bIsKinematic)
    //    {
    //        MSG_BOX("TriMesh Collider needs STATIC or KINEMATIC RigidBody");
    //        return E_FAIL;
    //    }
    //}

    pActor->attachShape(*m_pShape);

    return S_OK;
}

UPtr<CComPxTriMeshCollider> CComPxTriMeshCollider::Create()
{
    auto pInstance = ToUPtr(new CComPxTriMeshCollider{});
    if (FAILED(pInstance->InitializePrototype()))
    {
        MSG_BOX("Failed to Created : CComPxTriMeshCollider");
        return nullptr;
    }
    return pInstance;
}

UPtr<CPrototype> CComPxTriMeshCollider::Clone(void* pArg)
{
    auto pInstance = ToUPtr(new CComPxTriMeshCollider{ *this });
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned: CComPxTriMeshCollider");
        return nullptr;
    }
    return pInstance;
}

void CComPxTriMeshCollider::Free()
{
    CComPxCollider::Free();
}
