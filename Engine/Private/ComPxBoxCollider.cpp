#include "pch.h"
#include "ComPxBoxCollider.h"
#include "ComPxRigidBody.h"
#include "ResPhysXBoxGeometry.h"
#include "ResPhysXMaterial.h"

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

void CComPxBoxCollider::UpdateGUI()
{
    CComPxCollider::UpdateGUI();

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
	m_pResBoxGeo = pDesc->pResBoxGeo;
	if (!m_pResBoxGeo)
	{
		return E_FAIL;
	}
    if (FAILED(CComPxCollider::Initialize(pArg)))
    {
        return E_FAIL;
    }

	
	m_pShape = CGameInstance::Get().PxGetPhysics()->createShape(*m_pResBoxGeo->GetBoxGeometry(), *m_pResMaterial->GetMaterial());

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
    pActor->attachShape(*m_pShape);

    if (auto* dynamic = pActor->is<PxRigidDynamic>())
        PxRigidBodyExt::updateMassAndInertia(*dynamic, dynamic->getMass());
    
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
