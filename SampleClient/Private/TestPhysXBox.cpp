#include "pch.h"
#include "TestPhysXBox.h"
#include "GameInstance.h"
#include "Collider.h"
#include "ComPxRigidBody.h"
#include "ComPxBoxCollider.h"
#include "ComPxSphereCollider.h"
#include "ComPxCapsuleCollider.h"
#include "DbgLineRender.h"

NS_USING(Client)

CTestPhysXBox::CTestPhysXBox()
{
}

CTestPhysXBox::~CTestPhysXBox()
{
}

HRESULT CTestPhysXBox::Initialize(void* pArg)
{
	auto		pDesc = static_cast<DESC*>(pArg);
	if (!pDesc)
		return E_FAIL;

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
		CComPxBoxCollider::DESC Desc{};
		Desc.pComPxRigidBody = m_pComPxRigidBody;
		Desc.pResBoxGeo = CGameInstance::Get().GetResourceFirst<CResPhysXBoxGeometry>("SAMPLE_CLIENT_PX", "TMP_GEO_BOX");
		Desc.pResMaterial = CGameInstance::Get().GetResourceFirst<CResPhysXMaterial>("SAMPLE_CLIENT_PX", "TMP_MATERIAL");
		Desc.tFilter = pDesc->tFilter;
		if (FAILED(AddComponentFromProto("PHYSX", "Prototype_Component_ComPxBoxCollider", "ComPxBoxCollider", &Desc, &m_pComPxBoxCollider)))
		{
			return E_FAIL;
		};
	}

	if (!m_pComPxRigidBody->SetLinearVelocity(pDesc->vInitialVelocity))
		return E_FAIL;

	m_fPlayerCollisionDelay = pDesc->fPlayerCollisionDelay;

	return S_OK;
}

void CTestPhysXBox::PriorityUpdate(E::_float fTimeDelta)
{
}

void CTestPhysXBox::Update(E::_float fTimeDelta)
{
	if (m_fPlayerCollisionDelay >= 0.f)
	{
		m_fPlayerCollisionDelay -= fTimeDelta;
		if (m_fPlayerCollisionDelay <= 0.f)
		{
			auto tFilter = m_pComPxBoxCollider->GetFilter();
			tFilter.iSimulationMask |= ETOUI(COLLISION_LAYER::PLAYER_BODY);
			m_pComPxBoxCollider->SetFilter(tFilter);
			m_fPlayerCollisionDelay = -1.f;
		}
	}
}

void CTestPhysXBox::LateUpdate(E::_float fTimeDelta)
{
	UpdatePhysicData();
	GetTransform().Update();

	if (auto* pDbgLineRender = CGameInstance::Get().GetDbgLineRender())
	{
		const auto vPreviousColor = pDbgLineRender->GetColor();
		const auto ePreviousDepthMode = pDbgLineRender->GetDepthMode();
		pDbgLineRender->SetColor({ 1.f, 0.4f, 0.f, 1.f });
		pDbgLineRender->SetDepthTest(true);
		pDbgLineRender->AddBox({ 0.5f, 0.5f, 0.5f }, GetTransform().GetLoadedWorldMatrix());
		pDbgLineRender->SetColor(vPreviousColor);
		pDbgLineRender->SetDepthMode(ePreviousDepthMode);
	}
}

HRESULT CTestPhysXBox::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	return S_OK;
}

void CTestPhysXBox::OnWake()
{
}

void CTestPhysXBox::OnSleep()
{
	int x = 0;
}

void CTestPhysXBox::OnCollisionEnter(CGameObject* pObj, const PX_ON_COLLISION_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][TestPhysXBox] Collision Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

void CTestPhysXBox::OnCollisionExit(CGameObject* pObj, const PX_ON_COLLISION_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][TestPhysXBox] Collision Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

void CTestPhysXBox::OnTriggerEnter(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][TestPhysXBox] Trigger Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

void CTestPhysXBox::OnTriggerExit(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][TestPhysXBox] Trigger Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

E::UPtr<CTestPhysXBox> CTestPhysXBox::Create()
{
	auto pInstance = E::ToUPtr(new CTestPhysXBox{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CTestPhysX");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CTestPhysXBox::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CTestPhysXBox{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTestPhysXBox");
		return nullptr;
	}

	return pInstance;
}
