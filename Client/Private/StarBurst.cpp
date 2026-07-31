#include "pch.h"
#include "StarBurst.h"
#include "Client_Resources.h"
#include "Trail_CPU.h"
#include "ComPxSphereCollider.h"
#include "ComPxRigidBody.h"

#include "BossTMB.h"

NS_USING(Client)

CStarBurst::CStarBurst()	: CGameObject{}	{}
CStarBurst::~CStarBurst()					{}

void CStarBurst::UpdateGUI() {
	CGameObject::UpdateGUI();
}

HRESULT CStarBurst::InitializePrototype(void* pArg) {
	return S_OK;
}
HRESULT CStarBurst::Initialize(void* pArg) {
	if (nullptr == pArg)	return E_FAIL;

	if (FAILED(CGameObject::Initialize(pArg)))	return E_FAIL;

	const auto pDesc = static_cast<const STARBURST_DESC*>(pArg);
	{
		CComPxRigidBody::DESC Desc{};
		Desc.eType = CComPxRigidBody::TYPE::KINEMATIC;
		Desc.vPosition = pDesc->vStartPosition;

		if (FAILED(AddComponentFromProto("PHYSX", "Prototype_Component_ComPxRigidBody", "ComPxRigidBody", &Desc, &m_pComPxRigidBody)))	return E_FAIL;
	}
	{
		CComPxSphereCollider::DESC Desc{};
		Desc.pComPxRigidBody = m_pComPxRigidBody;
		Desc.pResSphereGeo = CResPhysXSphereGeometry::CreateAndLoad({ .fRadius = pDesc->fRadius });
		Desc.pResMaterial = CGameInstance::Get().GetResourceFirst<CResPhysXMaterial>("CLIENT_PX", "TMP_MATERIAL");
		Desc.bIsTrigger = true;
		Desc.tFilter = pDesc->tFilter;

		if (FAILED(AddComponentFromProto("PHYSX", "Prototype_Component_ComPxSphereCollider", "ComPxShpereCollider", &Desc, &m_pComPxShpereCollider)))	return E_FAIL;
	}

	GetTransform().SetPosition(m_pComPxRigidBody->GetPosition());
	return S_OK;
}

void CStarBurst::PriorityUpdate(E::_float fTimeDelta)
{
}

void CStarBurst::Update(E::_float fTimeDelta)
{
}

void CStarBurst::LateUpdate(E::_float fTimeDelta) {
	GetTransform().SetPosition(m_pComPxRigidBody->GetPosition());
	GetTransform().Update();

	auto matrix = XMLoadFloat4x4(m_pComTransform->GetWorldMatrix());

	auto cachedCol = CGameInstance::Get().GetDbgLineRender()->GetColor();
	auto cachedDepth = CGameInstance::Get().GetDbgLineRender()->GetDepthMode();
	CGameInstance::Get().GetDbgLineRender()->SetColor({ 1.f, 0.f, 0.f, 1.f });
	CGameInstance::Get().GetDbgLineRender()->SetDepthTest(false);
	CGameInstance::Get().GetDbgLineRender()->AddSphere(0.1f, matrix);
	CGameInstance::Get().GetDbgLineRender()->SetColor(cachedCol);
	CGameInstance::Get().GetDbgLineRender()->SetDepthMode(cachedDepth);
}
HRESULT CStarBurst::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	return S_OK;
}

void CStarBurst::OnCollisionEnter(CGameObject* pObj, const PX_ON_COLLISION_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CStarBurst] Collision Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

void CStarBurst::OnCollisionExit(CGameObject* pObj, const PX_ON_COLLISION_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CStarBurst] Collision Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

void CStarBurst::OnTriggerEnter(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CStarBurst] Trigger Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

void CStarBurst::OnTriggerExit(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CStarBurst] Trigger Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

E::UPtr<CStarBurst>		CStarBurst::Create()
{
	auto pInstance = E::ToUPtr(new CStarBurst{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CStarBurst");
		return nullptr;
	}
	return pInstance;
}

E::UPtr<E::CPrototype> CStarBurst::Clone(void* pArg)
{
	auto pInstance = E::ToUPtr(new CStarBurst{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CStarBurst");
		return nullptr;
	}

	return pInstance;
}
