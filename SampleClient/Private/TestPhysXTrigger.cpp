#include "pch.h"
#include "TestPhysXTrigger.h"

#include "GameInstance.h"
#include "ComPxRigidBody.h"
#include "ComPxBoxCollider.h"
#include "ResPhysXBoxGeometry.h"
#include "ResPhysXMaterial.h"

NS_USING(Client)

CTestPhysXTrigger::CTestPhysXTrigger()
{
}

CTestPhysXTrigger::~CTestPhysXTrigger()
{
}

HRESULT CTestPhysXTrigger::Initialize(void* pArg)
{
	auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc)
		return E_FAIL;

	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	{
		CComPxRigidBody::DESC Desc{};
		Desc.eType = CComPxRigidBody::TYPE::STATIC;
		Desc.vPosition = pDesc->vInitialPos;
		if (FAILED(AddComponentFromProto("PHYSX", "Prototype_Component_ComPxRigidBody", "ComPxRigidBody", &Desc, &m_pComPxRigidBody)))
			return E_FAIL;
	}

	{
		CComPxBoxCollider::DESC Desc{};
		Desc.pComPxRigidBody = m_pComPxRigidBody;
		Desc.pResBoxGeo = CResPhysXBoxGeometry::CreateAndLoad({ .vHalfExtents = pDesc->vHalfExtents });
		Desc.pResMaterial = CGameInstance::Get().GetResourceFirst<CResPhysXMaterial>("SAMPLE_CLIENT_PX", "TMP_MATERIAL");
		Desc.bIsTrigger = pDesc->bIsTrigger;
		Desc.tFilter = pDesc->tFilter;
		if (!Desc.pResBoxGeo || !Desc.pResMaterial)
			return E_FAIL;

		if (FAILED(AddComponentFromProto("PHYSX", "Prototype_Component_ComPxBoxCollider", "ComPxBoxCollider", &Desc, &m_pComPxBoxCollider)))
			return E_FAIL;
	}

	return S_OK;
}

void CTestPhysXTrigger::PriorityUpdate(E::_float fTimeDelta)
{
}

void CTestPhysXTrigger::Update(E::_float fTimeDelta)
{
}

void CTestPhysXTrigger::LateUpdate(E::_float fTimeDelta)
{
	UpdatePhysicData();
	GetTransform().Update();
}

HRESULT CTestPhysXTrigger::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	return S_OK;
}

void CTestPhysXTrigger::OnWake()
{
}

void CTestPhysXTrigger::OnSleep()
{
}

void CTestPhysXTrigger::OnCollisionEnter(CGameObject* pObj, const PX_ON_COLLISION_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][TestPhysXTrigger] Collision Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

void CTestPhysXTrigger::OnCollisionExit(CGameObject* pObj, const PX_ON_COLLISION_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][TestPhysXTrigger] Collision Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

void CTestPhysXTrigger::OnTriggerEnter(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][TestPhysXTrigger] Trigger Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

void CTestPhysXTrigger::OnTriggerExit(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][TestPhysXTrigger] Trigger Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

E::UPtr<CTestPhysXTrigger> CTestPhysXTrigger::Create()
{
	auto pInstance = E::ToUPtr(new CTestPhysXTrigger{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CTestPhysXTrigger");
		return nullptr;
	}
	return pInstance;
}

E::UPtr<E::CPrototype> CTestPhysXTrigger::Clone(void* pArg)
{
	auto pInstance = E::ToUPtr(new CTestPhysXTrigger{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTestPhysXTrigger");
		return nullptr;
	}

	return pInstance;
}
