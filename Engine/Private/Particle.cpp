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

	//if (auto res = CGameInstance::Get().AddResourceT<E::CResTestModel>("LOBJ", "Model_Resource", CResTestModel::Create("./Resources/SampleClient/Models/LightObject/LightObject.fbx"))) {
	//	E::CResTestModel::DESC pDesc = { MODEL::NONANIM, XMMatrixIdentity() };
	//	if (FAILED(res->Load(pDesc)))	return E_FAIL;
	//}
	//
	//
	//CComModelInstance::DESC Desc{};
	//Desc.sGroupTag = "LOBJ";
	//Desc.sResTag = "Model_Resource";
	//
	//if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ModelInstance", "ComCModelIntance", &Desc, &m_pComModelInstance)))
	//{
	//	return E_FAIL;
	//};


	m_pParticleTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(textureId.first, textureId.second);
	return m_pParticleTexture ? S_OK : E_FAIL;
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
		Spawn((uint32_t)readyList.size(), readyList.data()); // 순수가상함수 → 파생클래스(GPU/CPU) 구현 호출
	}
}
