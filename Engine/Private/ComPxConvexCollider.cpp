#include "pch.h"
#include "ComPxConvexCollider.h"
#include "ComPxRigidBody.h"
#include "ResPhysXConvexGeometry.h"

#pragma push_macro("new")
#undef new
#include "PxPhysicsAPI.h"
#pragma pop_macro("new")

using namespace physx;
NS_USING(Engine)

CComPxConvexCollider::CComPxConvexCollider() = default;
CComPxConvexCollider::~CComPxConvexCollider() = default;

HRESULT CComPxConvexCollider::Initialize(void* pArg)
{
	auto* desc = static_cast<DESC*>(pArg);
	if (!desc || !desc->pResConvex ||
		desc->vScale.x <= 0.f || desc->vScale.y <= 0.f || desc->vScale.z <= 0.f)
	{
		return E_FAIL;
	}

	if (FAILED(CComPxCollider::Initialize(pArg)))
		return E_FAIL;

	m_pResConvex = desc->pResConvex;
	auto* physics = CGameInstance::Get().PxGetPhysics();
	auto* convexMesh = m_pResConvex->GetConvexMesh();
	auto* material = m_pResMaterial->GetMaterial();
	auto* actor = m_pComRigidBody->GetActor();
	if (!physics || !convexMesh || !material || !actor)
		return E_FAIL;

	const PxMeshScale meshScale{ PxVec3{ desc->vScale.x, desc->vScale.y, desc->vScale.z } };
	m_pShape = physics->createShape(PxConvexMeshGeometry{ convexMesh, meshScale }, *material, true);
	if (!m_pShape)
		return E_FAIL;

	if (desc->bIsTrigger)
	{
		m_pShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
		m_pShape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
	}

	if (!SetLocalPosition(desc->vLocalOffset))
		return E_FAIL;

	if (!actor->attachShape(*m_pShape))
		return E_FAIL;

	if (!RegisterShape(PX_SHAPE_TYPE::CONVEX_MESH))
	{
		actor->detachShape(*m_pShape);
		return E_FAIL;
	}

	if (auto* dynamicActor = actor->is<PxRigidDynamic>();
		dynamicActor && !dynamicActor->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
	{
		if (!PxRigidBodyExt::updateMassAndInertia(*dynamicActor, dynamicActor->getMass()))
			return E_FAIL;
	}

	return S_OK;
}

void CComPxConvexCollider::UpdateGUI()
{
	CComPxCollider::UpdateGUI();
}

UPtr<CComPxConvexCollider> CComPxConvexCollider::Create()
{
	auto instance = ToUPtr(new CComPxConvexCollider{});
	if (FAILED(instance->InitializePrototype()))
		return nullptr;
	return instance;
}

UPtr<CPrototype> CComPxConvexCollider::Clone(void* pArg)
{
	auto instance = ToUPtr(new CComPxConvexCollider{ *this });
	if (FAILED(instance->Initialize(pArg)))
		return nullptr;
	return instance;
}

void CComPxConvexCollider::Free()
{
	m_pResConvex.reset();
	CComPxCollider::Free();
}
