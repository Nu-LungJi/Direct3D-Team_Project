#include "pch.h"
#include "TestPhysXCapsule.h"
#include "GameInstance.h"
#include "Collider.h"
#include "ComPxRigidBody.h"
#include "ComPxBoxCollider.h"
#include "ComPxSphereCollider.h"
#include "ComPxCapsuleCollider.h"

NS_USING(Client)

CTestPhysXCapsule::CTestPhysXCapsule()
{
}

CTestPhysXCapsule::~CTestPhysXCapsule()
{
}

HRESULT CTestPhysXCapsule::Initialize(void* pArg)
{
	auto		pDesc = static_cast<DESC*>(pArg);

	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	{
		CComPxRigidBody::DESC Desc{};
		Desc.eType = CComPxRigidBody::TYPE::DYNAMIC;
		Desc.vPosition = pDesc->vInitialPos;
		if (FAILED(AddComponentFromProto("PHYSX", "Prototype_Component_ComPxRigidBody", "ComPxRigidBody", &Desc, &m_pComPxRigidBody)))
		{
			return E_FAIL;
		};
	}

	{
		CComPxCapsuleCollider::DESC Desc{};
		Desc.pComPxRigidBody = m_pComPxRigidBody;
		Desc.pResCapsuleGeo = CGameInstance::Get().GetResourceFirst<CResPhysXCapsuleGeometry>("SAMPLE_CLIENT_PHYSIX", "TMP_GEO_CAPSULE");
		Desc.pResMaterial = CGameInstance::Get().GetResourceFirst<CResPhysXMaterial>("SAMPLE_CLIENT_PHYSIX", "TMP_MATERIAL");
		if (FAILED(AddComponentFromProto("PHYSX", "Prototype_Component_ComPxCapsuleCollider", "ComPxCapsuleCollider", &Desc, &m_pComPxBoxCollider)))
		{
			return E_FAIL;
		};
	}

	return S_OK;
}

void CTestPhysXCapsule::PriorityUpdate(E::_float fTimeDelta)
{
}

void CTestPhysXCapsule::Update(E::_float fTimeDelta)
{
}

void CTestPhysXCapsule::LateUpdate(E::_float fTimeDelta)
{
	UpdatePhysicData();
	GetTransform().Update();
}

HRESULT CTestPhysXCapsule::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	return S_OK;
}

void CTestPhysXCapsule::OnWake()
{
}

void CTestPhysXCapsule::OnSleep()
{
	int x = 0;
}

void CTestPhysXCapsule::OnCollisionEnter(CGameObject* pObj, const PHYSIX_ON_COLLISION_DATA& info)
{
}

void CTestPhysXCapsule::OnCollisionExit(CGameObject* pObj, const PHYSIX_ON_COLLISION_DATA& info)
{
}

void CTestPhysXCapsule::OnTriggerEnter(CGameObject* pObj, const PHYSIX_ON_TRIGGER_DATA& info)
{
}

void CTestPhysXCapsule::OnTriggerExit(CGameObject* pObj, const PHYSIX_ON_TRIGGER_DATA& info)
{
}

E::UPtr<CTestPhysXCapsule> CTestPhysXCapsule::Create()
{
	auto pInstance = E::ToUPtr(new CTestPhysXCapsule{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CTestPhysXCapsule");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CTestPhysXCapsule::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CTestPhysXCapsule{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTestPhysXCapsule");
		return nullptr;
	}

	return pInstance;
}
