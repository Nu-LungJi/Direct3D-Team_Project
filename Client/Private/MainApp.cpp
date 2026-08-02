#include "pch.h"
#include "MainApp.h"
#include "GameInstance.h"
#include "LevelLoading.h"

#include "MainAppLoader.h"
#include "CinematicEditor.h"
#include "UIManager.h"

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

	GET_SINGLE(UIManager)->InitializeActions();
	GET_SINGLE(UIManager)->InitializeFunc();

	if (FAILED(CBaseApp::Initialize(EngineDesc)))
	{
		return E_FAIL;
	}
	LOG_MEMORY("CBaseApp::Initialize End");

	CGameInstance::Get().ImguiEnableDocking(true, true);

	m_pCinematicEditor = CCinematicEditor::Create();
	if (m_pCinematicEditor == nullptr)
	{
		return E_FAIL;
	}

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

	E::CGameInstance::Get().RegisterLevelChangeFunc("TO_TERRAIN", [=]() {
		Engine::CGameInstance::Get().ChangeLevel(
			CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::TERRAIN));
		});

	E::CGameInstance::Get().RegisterLevelChangeFunc("TO_UIEditor", [=]() {
		Engine::CGameInstance::Get().ChangeLevel(
			CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::UIEDITOR));
		});

	// 초기 로딩에 소요된 시간을 첫 프레임의 DeltaTime에 포함하지 않는다.
	CGameInstance::Get().UpdateTimeProvider();

	LOG_MEMORY("CMainApp::Initialize End");
	return S_OK;
}

void CMainApp::FrameStart(_float fTimeDelta)
{
	CBaseApp::FrameStart(fTimeDelta);

	if (m_pCinematicEditor && E::CGameInstance::Get().ImguiGetActive())
	{
		m_pCinematicEditor->UpdateGUI();
	}
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
