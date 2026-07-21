#include "pch.h"
#include "TestPhysXCapsule.h"
#include "GameInstance.h"
#include "Collider.h"
#include "ComPxRigidBody.h"
#include "ComPxBoxCollider.h"
#include "ComPxSphereCollider.h"
#include "ComPxCapsuleCollider.h"
#include "DbgLineRender.h"

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
		CComPxCapsuleCollider::DESC Desc{};
		Desc.pComPxRigidBody = m_pComPxRigidBody;
		Desc.pResCapsuleGeo = CGameInstance::Get().GetResourceFirst<CResPhysXCapsuleGeometry>("SAMPLE_CLIENT_PX", "TMP_GEO_CAPSULE");
		Desc.pResMaterial = CGameInstance::Get().GetResourceFirst<CResPhysXMaterial>("SAMPLE_CLIENT_PX", "TMP_MATERIAL");
		Desc.tFilter = pDesc->tFilter;
		if (FAILED(AddComponentFromProto("PHYSX", "Prototype_Component_ComPxCapsuleCollider", "ComPxCapsuleCollider", &Desc, &m_pComPxBoxCollider)))
		{
			return E_FAIL;
		};
	}

	if (!m_pComPxRigidBody->SetLinearVelocity(pDesc->vInitialVelocity))
		return E_FAIL;

	m_fPlayerCollisionDelay = pDesc->fPlayerCollisionDelay;

	return S_OK;
}

void CTestPhysXCapsule::PriorityUpdate(E::_float fTimeDelta)
{
}

void CTestPhysXCapsule::Update(E::_float fTimeDelta)
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

void CTestPhysXCapsule::LateUpdate(E::_float fTimeDelta)
{
	UpdatePhysicData();
	GetTransform().Update();

	if (auto* pDbgLineRender = CGameInstance::Get().GetDbgLineRender())
	{
		const auto vPreviousColor = pDbgLineRender->GetColor();
		const auto ePreviousDepthMode = pDbgLineRender->GetDepthMode();
		pDbgLineRender->SetColor({ 1.f, 1.f, 0.f, 1.f });
		pDbgLineRender->SetDepthTest(true);
		pDbgLineRender->AddCapsule(
			0.5f,
			0.5f,
			XMMatrixRotationZ(XM_PIDIV2) * GetTransform().GetLoadedWorldMatrix());
		pDbgLineRender->SetColor(vPreviousColor);
		pDbgLineRender->SetDepthMode(ePreviousDepthMode);
	}
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

void CTestPhysXCapsule::OnCollisionEnter(CGameObject* pObj, const PX_ON_COLLISION_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][TestPhysXCapsule] Collision Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

void CTestPhysXCapsule::OnCollisionExit(CGameObject* pObj, const PX_ON_COLLISION_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][TestPhysXCapsule] Collision Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

void CTestPhysXCapsule::OnTriggerEnter(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][TestPhysXCapsule] Trigger Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

void CTestPhysXCapsule::OnTriggerExit(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][TestPhysXCapsule] Trigger Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
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
