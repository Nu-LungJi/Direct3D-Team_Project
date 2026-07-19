#include "pch.h"
#include "TestPhysXBall.h"
#include "GameInstance.h"
#include "Collider.h"
#include "ComPxRigidBody.h"
#include "ComPxBoxCollider.h"
#include "ComPxSphereCollider.h"
#include "ComPxCapsuleCollider.h"
#include "DbgLineRender.h"

NS_USING(Client)

CTestPhysXBall::CTestPhysXBall()
{
}

CTestPhysXBall::~CTestPhysXBall()
{
}

HRESULT CTestPhysXBall::Initialize(void* pArg)
{
	auto		pDesc = static_cast<DESC*>(pArg);
	if (!pDesc)
		return E_FAIL;
	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	//"SAMPLE_CLIENT_PX", "TMP_MATERIAL",
	//	"SAMPLE_CLIENT_PX", "TMP_GEO_BOX",
	//	"SAMPLE_CLIENT_PX", "TMP_GEO_SPHERE",
	//	"SAMPLE_CLIENT_PX", "TMP_GEO_CAPSULE"


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
		CComPxSphereCollider::DESC Desc{};
		Desc.pComPxRigidBody = m_pComPxRigidBody;
		if (pDesc->fRadius == 0.5f)
			Desc.pResSphereGeo = CGameInstance::Get().GetResourceFirst<CResPhysXSphereGeometry>("SAMPLE_CLIENT_PX", "TMP_GEO_SPHERE");
		else
			Desc.pResSphereGeo = CResPhysXSphereGeometry::CreateAndLoad({ .fRadius = pDesc->fRadius });
		if (pDesc->fRestitution >= 0.f)
		{
			Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad({
				.fStaticFriction = 0.f,
				.fDynamicFriction = 0.f,
				.fRestitution = pDesc->fRestitution });
		}
		else
		{
			Desc.pResMaterial = CGameInstance::Get().GetResourceFirst<CResPhysXMaterial>("SAMPLE_CLIENT_PX", "TMP_MATERIAL");
		}
		Desc.tFilter = pDesc->tFilter;
		if (FAILED(AddComponentFromProto("PHYSX", "Prototype_Component_ComPxSphereCollider", "ComPxBoxCollider", &Desc, &m_pComPxSphereCollider)))
		{
			return E_FAIL;
		};
	}

	if (!m_pComPxRigidBody->SetLinearVelocity(pDesc->vInitialVelocity))
		return E_FAIL;
	if (!m_pComPxRigidBody->SetGravityEnabled(pDesc->bUseGravity))
		return E_FAIL;

	m_fRadius = pDesc->fRadius;
	m_fLifetime = pDesc->fLifetime;
	m_fPlayerCollisionDelay = pDesc->fPlayerCollisionDelay;

	return S_OK;
}

void CTestPhysXBall::PriorityUpdate(E::_float fTimeDelta)
{
}

void CTestPhysXBall::Update(E::_float fTimeDelta)
{
	if (m_fLifetime >= 0.f)
	{
		m_fLifetime -= fTimeDelta;
		if (m_fLifetime <= 0.f)
		{
			SetPendingDestroyCascade();
			return;
		}
	}

	if (m_fPlayerCollisionDelay >= 0.f)
	{
		m_fPlayerCollisionDelay -= fTimeDelta;
		if (m_fPlayerCollisionDelay <= 0.f)
		{
			auto tFilter = m_pComPxSphereCollider->GetFilter();
			tFilter.iSimulationMask |= ETOUI(COLLISION_LAYER::PLAYER_BODY);
			m_pComPxSphereCollider->SetFilter(tFilter);
			m_fPlayerCollisionDelay = -1.f;
		}
	}
}

void CTestPhysXBall::LateUpdate(E::_float fTimeDelta)
{
	UpdatePhysicData();
	GetTransform().Update();

	if (auto* pDbgLineRender = CGameInstance::Get().GetDbgLineRender())
	{
		const auto vPreviousColor = pDbgLineRender->GetColor();
		const auto ePreviousDepthMode = pDbgLineRender->GetDepthMode();
		pDbgLineRender->SetColor({ 0.f, 1.f, 1.f, 1.f });
		pDbgLineRender->SetDepthTest(true);
		pDbgLineRender->AddSphere(m_fRadius, GetTransform().GetLoadedWorldMatrix());
		pDbgLineRender->SetColor(vPreviousColor);
		pDbgLineRender->SetDepthMode(ePreviousDepthMode);
	}
	//CGameInstance::Get().AddColliderGroup("Coll_TestPhysX", m_pComCollider->Get());
	//m_pComCollider->Get()->Transform(GetTransform().GetLoadedCombinedWorldMatrix());
}

HRESULT CTestPhysXBall::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	return S_OK;
}

void CTestPhysXBall::OnWake()
{
}

void CTestPhysXBall::OnSleep()
{
	int x = 0;
}

void CTestPhysXBall::OnCollisionEnter(CGameObject* pObj, const PX_ON_COLLISION_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][TestPhysXBall] Collision Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

void CTestPhysXBall::OnCollisionExit(CGameObject* pObj, const PX_ON_COLLISION_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][TestPhysXBall] Collision Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

void CTestPhysXBall::OnTriggerEnter(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][TestPhysXBall] Trigger Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

void CTestPhysXBall::OnTriggerExit(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][TestPhysXBall] Trigger Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

E::UPtr<CTestPhysXBall> CTestPhysXBall::Create()
{
	auto pInstance = E::ToUPtr(new CTestPhysXBall{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CTestPhysX");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CTestPhysXBall::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CTestPhysXBall{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTestPhysXBall");
		return nullptr;
	}

	return pInstance;
}
