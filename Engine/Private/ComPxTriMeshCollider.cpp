#include "pch.h"
#include "ComPxTriMeshCollider.h"

#include "ComPxRigidBody.h"
#pragma push_macro("new")
#undef new

#include "PxPhysicsAPI.h"

#pragma pop_macro("new")

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
	if (pDesc->bIsTrigger)
	{
		DEBUG_LOG("[PX][TriMesh] Triangle mesh triggers are not supported.\n");
		return E_FAIL;
	}

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

	auto* pActor = m_pComRigidBody->GetActor();
	if (!pActor)
		return E_FAIL;
	if (m_pComRigidBody->GetRigidBodyType() == CComPxRigidBody::TYPE::DYNAMIC)
	{
		DEBUG_LOG("[PX][TriMesh] Triangle mesh requires a static or kinematic rigid body.\n");
		return E_FAIL;
	}

    m_pShape = pPhysics->createShape(PxTriangleMeshGeometry(pTriMesh), *pMaterial, true);
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

	if (!pActor->attachShape(*m_pShape))
		return E_FAIL;

	if (!RegisterShape(PX_SHAPE_TYPE::TRIANGLE_MESH))
	{
		pActor->detachShape(*m_pShape);
		return E_FAIL;
	}

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
