#include "Coin.h"
#include "Client_Defines.h"
#include "GameInstance.h"
#include "Player.h"
NS_USING(Client)


HRESULT CCoin::Initialize(void* pArg)
{
	auto* pDesc = static_cast<CPhysXCollisionProxyObject::DESC*>(pArg);

	if (nullptr == pDesc || nullptr == pDesc->pCollisionData || pDesc->pCollisionData->actors.empty())
		return E_FAIL;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	const auto& actor = pDesc->pCollisionData->actors.front();

	GetTransform().SetPosition(XMLoadFloat3(&actor.vPosition));
	GetTransform().SetQuaternion(XMLoadFloat4(&actor.vRotation));
	GetTransform().Update();

	m_bParticleSpawned = false;
	m_bCollected = false;
	m_iParticleID = INVALID_PARTICLE_OWNER_ID;

	return S_OK;
}

void CCoin::Update(_float fTimeDelta)
{
	__super::Update(fTimeDelta);

	if (!m_bParticleSpawned)
	{
		m_iParticleID = CGameInstance::Get().Spawn("Coin.json", *GetTransform().GetWorldMatrix());
		m_bParticleSpawned = true;
	}
}

void CCoin::OnTriggerEnter(E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info)
{
   	if (nullptr == pObj || m_bCollected)
		return;

	auto* pPlayer = Engine::Cast<CPlayer>(pObj);
	if (nullptr == pPlayer)
		return;

	m_bCollected = true;

	if (m_iParticleID != INVALID_PARTICLE_OWNER_ID)
	{
		CGameInstance::Get().ClearParticleOwner(m_iParticleID);
		m_iParticleID = INVALID_PARTICLE_OWNER_ID;
	}

	_float4x4 effectWorld = *GetTransform().GetWorldMatrix();
	CGameInstance::Get().PlayEffect("CoinEarn", effectWorld);

	SetCollisionEnabled(false);
	SetPendingDestroy();
}
void CCoin::OnTriggerExit(E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info)
{
}


E::UPtr<CCoin> CCoin::Create()
{
	auto instance = E::ToUPtr(new CCoin{});
	if (FAILED(instance->InitializePrototype()))
		return nullptr;
	return instance;
}

E::UPtr<E::CPrototype> CCoin::Clone(void* pArg)
{
	auto instance = E::ToUPtr(new CCoin{ *this });
	if (FAILED(instance->Initialize(pArg)))
		return nullptr;
	return instance;
}
