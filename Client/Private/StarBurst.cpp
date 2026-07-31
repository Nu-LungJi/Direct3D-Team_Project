#include "pch.h"
#include "StarBurst.h"
#include "Client_Resources.h"
#include "Trail_CPU.h"
#include "ComPxSphereCollider.h"
#include "ComPxRigidBody.h"

#include "BossTMB.h"
#include "Player.h"

NS_USING(Client)

CBoss_StarBurst::CBoss_StarBurst()	: CGameObject{}	{}
CBoss_StarBurst::~CBoss_StarBurst()					{}

void CBoss_StarBurst::UpdateGUI() {
	CGameObject::UpdateGUI();
}

HRESULT CBoss_StarBurst::InitializePrototype(void* pArg) {
	return S_OK;
}
HRESULT CBoss_StarBurst::Initialize(void* pArg) {
	if (nullptr == pArg)	return E_FAIL;

	if (FAILED(CGameObject::Initialize(pArg)))	return E_FAIL;

	const auto pDesc = static_cast<const STARBURST_DESC*>(pArg);
	{
		CComPxRigidBody::DESC Desc{};
		Desc.eType = CComPxRigidBody::TYPE::KINEMATIC;
		Desc.vPosition = pDesc->vStartPosition;

		m_pTargetHandle = pDesc->pTargetHandle;
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

	m_pLightEffectID = CGameInstance::Get().PlayEffect("Boss_StarBurst_A", *GetTransform().GetWorldMatrix(), _vector{},
		[h = GetHandle(), this](EFFECT_INSTANCE_ID effectId, EFFECT_FINISH_REASON reason)
		{
			if (auto pOBJ = CGameInstance::Get().GetGameObjectByHandleT<CBoss_StarBurst>(h)) {
				m_pLightEffectID = INVALID_EFFECT_INSTANCE_ID;
			}
		});

	GetTransform().SetPosition(m_pComPxRigidBody->GetPosition());

	return S_OK;
}

void CBoss_StarBurst::PriorityUpdate(E::_float fTimeDelta)
{
}

void CBoss_StarBurst::Update(E::_float fTimeDelta) {
	const _float LavaFlame_SpawnInterval = 0.4f;
	const _float LavaFlame_CastingTime	 = 2.0f;
	const _float LavaFlame_StayTime = 1.5f;
	const _float LavaFlame_AttackingTime = 1.0f;

	m_fEffectSpawnTimer += fTimeDelta;
	m_fEffectLifeTime	+= fTimeDelta;

	if (m_fEffectLifeTime <= LavaFlame_CastingTime) {
		Translate_Casting(m_fEffectLifeTime / LavaFlame_CastingTime);
	}
	else if (m_fEffectLifeTime <= LavaFlame_CastingTime + LavaFlame_StayTime) {

	}
	else if (m_fEffectLifeTime <= LavaFlame_CastingTime + LavaFlame_AttackingTime + LavaFlame_StayTime) {
		Translate_Attacking((m_fEffectLifeTime - LavaFlame_CastingTime - LavaFlame_StayTime) / LavaFlame_AttackingTime);
	}

	if (m_fEffectSpawnTimer >= LavaFlame_SpawnInterval) {
		m_fEffectSpawnTimer = 0.f;
		CGameInstance::Get().PlayEffect("Boss_StarBurst_B", *GetTransform().GetWorldMatrix(), XMVECTOR{});
	}

	if (m_pLightEffectID == INVALID_EFFECT_INSTANCE_ID) {
		SetPendingDestroy();
	}
}

void CBoss_StarBurst::LateUpdate(E::_float fTimeDelta) {
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
HRESULT CBoss_StarBurst::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) {

	return S_OK;
}

void CBoss_StarBurst::Translate_Casting(_float _Ratio){

	_float3 LocalDestination = { 16.f, 10.f, 2.f };

	m_fCurrEffectMovementValue = 1.f - pow(1.f - _Ratio, 3.f);
	_float DeltaMovementValue = m_fCurrEffectMovementValue - m_fPrevEffectMovementValue;
	m_fPrevEffectMovementValue = m_fCurrEffectMovementValue;

	_float3 CurrentPosition = m_pComPxRigidBody->GetPosition();

	m_pComPxRigidBody->SetPosition(XMFLOAT3(
		CurrentPosition.x + DeltaMovementValue * LocalDestination.x, 
		CurrentPosition.y + DeltaMovementValue * LocalDestination.y,
		CurrentPosition.z + DeltaMovementValue * LocalDestination.z));
	CGameInstance::Get().SetEffectPosition(m_pLightEffectID, m_pComPxRigidBody->GetPosition());
}

void CBoss_StarBurst::Translate_Attacking(_float _Ratio) {
	auto GamePlayer = CGameInstance::Get().GetGameObjectByHandleT<CPlayer>(m_pTargetHandle);
	if (nullptr == GamePlayer)  return;
}

void CBoss_StarBurst::OnCollisionEnter(CGameObject* pObj, const PX_ON_COLLISION_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CBoss_StarBurst] Collision Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

void CBoss_StarBurst::OnCollisionExit(CGameObject* pObj, const PX_ON_COLLISION_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CBoss_StarBurst] Collision Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

void CBoss_StarBurst::OnTriggerEnter(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CBoss_StarBurst] Trigger Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

void CBoss_StarBurst::OnTriggerExit(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CBoss_StarBurst] Trigger Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

E::UPtr<CBoss_StarBurst>		CBoss_StarBurst::Create()
{
	auto pInstance = E::ToUPtr(new CBoss_StarBurst{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBoss_StarBurst");
		return nullptr;
	}
	return pInstance;
}

E::UPtr<E::CPrototype> CBoss_StarBurst::Clone(void* pArg)
{
	auto pInstance = E::ToUPtr(new CBoss_StarBurst{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBoss_StarBurst");
		return nullptr;
	}

	return pInstance;
}
