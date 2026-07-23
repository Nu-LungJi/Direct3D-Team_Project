#include "pch.h"
#include "Particle.h"
#include "GameInstance.h"
#include "ResTexture2D.h"
#include "ComModelInstance.h"

NS_USING(Engine)

CParticle::CParticle()
{

}


CParticle::~CParticle()
{
}

HRESULT CParticle::LoadParticleTexture(std::pair<StringID, StringID> textureId)
{
	m_pParticleTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(textureId.first, textureId.second);

	if (!m_pParticleTexture)
	{
		OutputDebugStringA("텍스처를 찾을 수 없음!\n");
		return E_FAIL;
	}

	// ---- GetDbgStr() 결과를 std::string으로 먼저 받아서 수명 보장 ----
	std::string strID1 = textureId.first.GetDbgStr();
	std::string strID2 = textureId.second.GetDbgStr();

	char buf[256];
	sprintf_s(buf, "텍스처 로드: ID1=%s, ID2=%s, SRV주소=%p\n",
		strID1.c_str(), strID2.c_str(),
		m_pParticleTexture->GetSRV().Get());
	OutputDebugStringA(buf);

	return S_OK;
}
void CParticle::RequestSpawn(const std::vector<PARTICLE_SPAWN_DATA>& spawnList)
{
	char buf[64];
	sprintf_s(buf, "RequestSpawn: %zu items queued\n", spawnList.size());
	OutputDebugStringA(buf);

	m_PendingSpawns.reserve(m_PendingSpawns.size() + spawnList.size());
	for (const auto& s : spawnList)
	{
		m_PendingSpawns.push_back({ s, s.spawnDelay });
	}
}

HRESULT CParticle::Set_BlendState(BLENDTYPE blendType)
{
	switch (ETOUI(blendType)){
	case 0:
		m_pBlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_EFFECT");
		break;
	case 1:
		m_pBlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_ADDITIVE");
		break;
	case 2:
		m_pBlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_BLEND_NONE");
		break;
	default:
		m_pBlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_EFFECT");
		break;
	}
	if (m_pBlendState == nullptr) {
		MSG_BOX("BlendState Nullptr");
		return E_FAIL;
	}
	m_iBlendIndex = ETOUI(blendType);
	return S_OK;
}

void CParticle::ProcessPendingSpawns(E::_float fTimeDelta)
{
	if (m_PendingSpawns.empty())
		return;

	char buf[64];
	sprintf_s(buf, "ProcessPendingSpawns: pending=%zu\n", m_PendingSpawns.size());
	OutputDebugStringA(buf);



	std::vector<PARTICLE_SPAWN_DATA> readyList;
	auto it = std::remove_if(m_PendingSpawns.begin(), m_PendingSpawns.end(),
		[&](PENDING_SPAWN& p)
		{
			p.remainingDelay -= fTimeDelta;
			if (p.remainingDelay <= 0.f)
			{
				readyList.push_back(p.data);
				return true;
			}
			return false;
		});
	m_PendingSpawns.erase(it, m_PendingSpawns.end());

	if (!readyList.empty())
	{
		HRESULT hr = Spawn((uint32_t)readyList.size(), readyList.data());
		if (FAILED(hr))
			OutputDebugStringA("Spawn FAILED!\n");
		else
			OutputDebugStringA("Spawn SUCCESS!\n");
	}
}
void CParticle::TranslateOwner(uint32_t ownerId, const _float3& delta) {

}
void CParticle::TransformOwner(uint32_t ownerId, const _float4x4& deltaMatrixData) {

}
