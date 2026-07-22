#include "pch.h"
#include "MainApp.h"
#include "GameInstance.h"
#include "LevelLoading.h"

#include "MainAppLoader.h"


NS_USING(Client)

CMainApp::CMainApp()
{
}

CMainApp::~CMainApp()
{
}

HRESULT CMainApp::Initialize()
{
	Engine::ENGINE_DESC EngineDesc{};
	EngineDesc.hWnd = g_hWnd;
	EngineDesc.hInstance = g_hInstance;
	EngineDesc.eWinMode = Engine::WINMODE::WIN;
	EngineDesc.iWinSizeX = g_iWinSizeX;
	EngineDesc.iWinSizeY = g_iWinSizeY;
	//EngineDesc.iNumLevels = Engine::ETOUI(LEVEL::END);

	if (FAILED(CBaseApp::Initialize(EngineDesc)))
	{
		return E_FAIL;
	}
	LOG_MEMORY("CBaseApp::Initialize End");

	CGameInstance::Get().ImguiEnableDocking(true, true);

	if (FAILED(CMainAppLoader::Load()))
	{
		MSG_BOX("MainLoader Failed");
		return E_FAIL;
	}

	if (CBaseApp::StartLevel(CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::LOGO)))
	{
		return E_FAIL;
	}

	E::CGameInstance::Get().RegisterLevelChangeFunc("TO_LOGO", [=]() {
		Engine::CGameInstance::Get().ChangeLevel(
			CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::LOGO));
		});

	E::CGameInstance::Get().RegisterLevelChangeFunc("TO_CHARLES_ROOKWOOD", [=]() {
		Engine::CGameInstance::Get().ChangeLevel(
			CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::CHARLES_ROOKWOOD));
		});

	E::CGameInstance::Get().RegisterLevelChangeFunc("TO_BOSS_CHARLES_ROOKWOOD", [=]() {
		Engine::CGameInstance::Get().ChangeLevel(
			CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::BOSS_CHARLES_ROOKWOOD));
		});

	return S_OK;
}



Engine::UPtr<CMainApp> CMainApp::Create()
{
	auto pInstance = Engine::UPtr<CMainApp>(new CMainApp{});

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CMainApp");
	}

	return pInstance;
}
