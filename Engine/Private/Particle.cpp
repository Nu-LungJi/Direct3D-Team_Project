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

void CParticle::ClearPendingSpawnsByOwner(uint32_t ownerID)
{
	// [LSY] StopEffect 이후 지연 스폰이 다시 살아나지 않도록
	// 아직 생성되지 않은 동일 Owner 요청도 함께 제거한다.
	std::erase_if(
		m_PendingSpawns,
		[ownerID](const PENDING_SPAWN& pending)
		{
			return pending.data.ownerID == ownerID;
		});
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
void CParticle::TransformPendingOwner(uint32_t ownerId, const _float4x4& deltaMatrixData)
{
	const XMMATRIX deltaMatrix = XMLoadFloat4x4(&deltaMatrixData);

	XMVECTOR scale{};
	XMVECTOR deltaRotation{};
	XMVECTOR translation{};

	if (!XMMatrixDecompose(&scale, &deltaRotation, &translation, deltaMatrix))
		return;

	deltaRotation = XMQuaternionNormalize(deltaRotation);

	XMFLOAT4X4 rotationMatrix{};
	XMStoreFloat4x4(&rotationMatrix, XMMatrixRotationQuaternion(deltaRotation));

	_vector vDeltaForward = XMVector3Normalize(XMVectorSet(rotationMatrix._31, rotationMatrix._32, rotationMatrix._33, 0.f));
	_float fDeltaPitch = asinf(std::clamp(-XMVectorGetY(vDeltaForward), -1.f, 1.f));
	_float fDeltaYaw = atan2f(XMVectorGetX(vDeltaForward), XMVectorGetZ(vDeltaForward));

	for (PENDING_SPAWN& pending : m_PendingSpawns)
	{
		PARTICLE_SPAWN_DATA& data = pending.data;

		if (data.ownerID != ownerId)
			continue;

		XMStoreFloat3(&data.position, XMVector3TransformCoord(XMLoadFloat3(&data.position), deltaMatrix));
		XMStoreFloat3(&data.originalPosition, XMVector3TransformCoord(XMLoadFloat3(&data.originalPosition), deltaMatrix));
		XMStoreFloat3(&data.velocity, XMVector3Rotate(XMLoadFloat3(&data.velocity), deltaRotation));
		XMStoreFloat3(&data.originalVelocity, XMVector3Rotate(XMLoadFloat3(&data.originalVelocity), deltaRotation));

		data.rotation.x = std::remainder(data.rotation.x + fDeltaPitch, XM_2PI);
		data.rotation.y = std::remainder(data.rotation.y + fDeltaYaw, XM_2PI);
	}
}
