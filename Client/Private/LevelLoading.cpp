#include "pch.h"
#include "LevelLoading.h"
#include "GameInstance.h"
#include "Resources.h"
#include "LevelLogo.h"
#include "LevelLogoLoader.h"
#include "LevelCharlesRookwood.h"
#include "LevelCharlesRookwoodLoader.h"

#include "LevelBossCharlesRookwood.h"
#include "LevelBossCharlesRookwoodLoader.h"

#include "LevelTerrain.h"
#include "LevelTerrainLoader.h"

NS_USING(Client)

CLevelLoading::CLevelLoading(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext, LEVEL eNextLevelIndex) noexcept
	: CLevel{ ETOUI(LEVEL::LOADING) }
	, m_pDevice{ pDevice }
	, m_pContext{ pContext }
	, m_eNextLevelIndex(eNextLevelIndex)
{
}

CLevelLoading::~CLevelLoading()
{
}

bool CLevelLoading::IsLevelChangeLocked() const
{
	return m_ePhase != PHASE::COMPLETE && m_ePhase != PHASE::FAILED;
}

HRESULT CLevelLoading::Initialize()
{
	LOG_MEMORY("CLevelLoading::Initialize");

	const uint32_t iCurrentLevelID = Engine::CGameInstance::Get().GetCurrentLevelID();
	if (iCurrentLevelID != Engine::CLevel::INVALID_LEVEL_ID)
		m_ePreviousLevelIndex = static_cast<LEVEL>(iCurrentLevelID);

	Engine::CGameInstance::Get().GameObjectAllReset();


	return S_OK;
}

void CLevelLoading::Update(E::_float fTimeDelta)
{
	switch (m_ePhase)
	{
	case PHASE::READY:
		StartUnload();
		break;
	case PHASE::UNLOADING:
		CheckUnload();
		break;
	case PHASE::LOADING:
		CheckLoad();
		break;
	default:
		break;
	}
}

HRESULT CLevelLoading::Render()
{
	return S_OK;
}

void CLevelLoading::UpdateGUI()
{
	ImGui::Begin("LEVEL: CLevelLoading");
	ImGui::End();
}

void CLevelLoading::FrameEnd(E::_float fTimeDelta)
{
	if (m_bLoadEnd)
	{
		LoadEnd();
		return;
	}
}

HRESULT CLevelLoading::LoadEnd()
{
	Engine::UPtr<CLevel>	pNewLevel{};
	switch (m_eNextLevelIndex)
	{
	case LEVEL::LOGO:
		pNewLevel = CLevelLogo::Create();
		break;
	case LEVEL::CHARLES_ROOKWOOD:
		pNewLevel = CLevelCharlesRookwood::Create();
		break;
	case LEVEL::BOSS_CHARLES_ROOKWOOD:
		pNewLevel = CLevelBossCharlesRookwood::Create();
		break;
	case LEVEL::TERRAIN:
		pNewLevel = CLevelTerrain::Create();
		break;
	}
	assert(pNewLevel);

	if (FAILED(Engine::CGameInstance::Get().ChangeLevel(std::move(pNewLevel))))
	{
		MSG_BOX("ChangeLevelFailed in loading");
		return E_FAIL;
	}
	return S_OK;
}

void CLevelLoading::StartUnload()
{
	if (!m_ePreviousLevelIndex || *m_ePreviousLevelIndex == LEVEL::LOADING)
	{
		StartLoad();
		return;
	}

	m_ePhase = PHASE::UNLOADING;
	switch (*m_ePreviousLevelIndex)
	{
	case LEVEL::LOGO:
		m_futUnloadFinish = CLevelLogoLoader::UnLoad();
		break;
	case LEVEL::CHARLES_ROOKWOOD:
		m_futUnloadFinish = CLevelCharlesRookwoodLoader::UnLoad();
		break;
	case LEVEL::BOSS_CHARLES_ROOKWOOD:
		m_futUnloadFinish = CLevelBossCharlesRookwoodLoader::UnLoad();
		break;
	case LEVEL::TERRAIN:
		m_futUnloadFinish = CLevelTerrainLoader::UnLoad();
		break;
	default:
		StartLoad();
		break;
	}
}

void CLevelLoading::CheckUnload()
{
	if (!m_futUnloadFinish.valid())
		return;

	if (m_futUnloadFinish.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
		return;

	if (!m_futUnloadFinish.get())
	{
		m_ePhase = PHASE::FAILED;
		MSG_BOX("UNLOADING FAILED");
		return;
	}

	StartLoad();
}

void CLevelLoading::StartLoad()
{
	m_ePhase = PHASE::LOADING;
	switch (m_eNextLevelIndex)
	{
	case LEVEL::LOGO:
		m_futLoadFinish = CLevelLogoLoader::Load();
		break;
	case LEVEL::CHARLES_ROOKWOOD:
		m_futLoadFinish = CLevelCharlesRookwoodLoader::Load();
		break;
	case LEVEL::BOSS_CHARLES_ROOKWOOD:
		m_futLoadFinish = CLevelBossCharlesRookwoodLoader::Load();
		break;
	case LEVEL::TERRAIN:
		m_futLoadFinish = CLevelTerrainLoader::Load();
		break;
	default:
		m_ePhase = PHASE::COMPLETE;
		m_bLoadEnd = true;
		break;
	}

}

void CLevelLoading::CheckLoad()
{
	if (m_futLoadFinish.valid())
	{
		if (m_futLoadFinish.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
		{
			if (!m_futLoadFinish.get())
			{
				m_ePhase = PHASE::FAILED;
				MSG_BOX("LOADING FAILED");
				return;
			}

			m_ePhase = PHASE::COMPLETE;
			m_bLoadEnd = true;
		}
	}
}



Engine::UPtr<CLevelLoading> CLevelLoading::Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext, LEVEL eNextLevelIndex)
{
	auto	pInstance = Engine::UPtr<CLevelLoading>(new CLevelLoading(pDevice, pContext, eNextLevelIndex));

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CLevelLoading");
		return nullptr;
	}

	return pInstance;
}

void CLevelLoading::Free()
{
	LOG_MEMORY("CLevelLoading::Free");
	CLevel::Free();
}
