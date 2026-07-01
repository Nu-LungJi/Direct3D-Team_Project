#include "pch.h"
#include "ParticleManager.h"

CParticleManager::CParticleManager()
{
}

CParticleManager::~CParticleManager()
{
}
void CParticleManager::Update(_float fTimeDelta)
{
	for (auto& p : particles)
	{
		if (!p.alive)
			continue;
		
		XMStoreFloat3(&p.position,XMLoadFloat3(&p.velocity) * fTimeDelta);

		p.life -= fTimeDelta;

		if (p.life <= 0 && !p.loop)
			p.alive = false;
	}
}

HRESULT CParticleManager::Render()
{
	return S_OK;
}

void CParticleManager::UpdateGUI()
{

	ImGui::Begin("CParticleManager");

	ImGui::End();


	//m_pCurrentLevel->UpdateGUI();
}

void CParticleManager::FrameStart(_float fTimeDelta)
{
	//if (m_pLevelBeforeLevelChange.get())
	//{
	//	if (m_pCurrentLevel.get())
	//	{
	//		//m_pCurrentLevel->BeforeLevelChange();
	//		m_pCurrentLevel.reset();
	//	}
	//
	//	m_pCurrentLevel = std::move(m_pLevelBeforeLevelChange);
	//	//m_pCurrentLevel->AfterLevelChange();
	//}
	//
	//m_pCurrentLevel->FrameStart(fTimeDelta);
}

void CParticleManager::FrameEnd(_float fTimeDelta)
{
	//m_pCurrentLevel->FrameEnd(fTimeDelta);
}

UPtr<CParticleManager> CParticleManager::Create()
{
	return UPtr<CParticleManager>(new CParticleManager{});
}
