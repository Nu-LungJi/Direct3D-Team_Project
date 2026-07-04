#include "pch.h"
#include "Particle.h"
#include "GameInstance.h"
#include "ResTexture2D.h"

NS_USING(Engine)

CParticle::CParticle()
{
}

CParticle::CParticle(const CParticle& rhs)
{
}

CParticle::~CParticle()
{
}

HRESULT CParticle::LoadParticleTexture(std::pair<StringID, StringID> textureId)
{
	m_pParticleTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(textureId.first, textureId.second);
	return m_pParticleTexture ? S_OK : E_FAIL;
}