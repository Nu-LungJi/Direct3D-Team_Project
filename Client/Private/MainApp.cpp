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
	// NvCloth A/B test switch. Change to DX11 to compare the same cape.
	EngineDesc.eNvClothBackend = Engine::NVCLOTH_BACKEND::CPU;

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

	E::CGameInstance::Get().RegisterLevelChangeFunc("TO_HOGWART_WORLD", [=]() {
		Engine::CGameInstance::Get().ChangeLevel(
			CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::HOGWART_WORLD));
		});

	E::CGameInstance::Get().RegisterLevelChangeFunc("TO_UIEditor", [=]() {
		Engine::CGameInstance::Get().ChangeLevel(
			CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::UIEDITOR));
		});

	E::CGameInstance::Get().RegisterLevelChangeFunc("TO_LAST_BOSS_RANROK", [=]() {
		Engine::CGameInstance::Get().ChangeLevel(
			CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::LAST_BOSS_RANROK));
		});

	// 초기 로딩에 소요된 시간을 첫 프레임의 DeltaTime에 포함하지 않는다.
	CGameInstance::Get().UpdateTimeProvider();

	LOG_MEMORY("CMainApp::Initialize End");

	// 광윤 추가 : 만약 이게 Merge할때 안 지워졌다면 전적으로 제 책임이니까 걍 죽여주세요
#ifdef _DEBUG
	CGameInstance::Get().ChangeLevel("TO_HOGWART_WORLD");
#endif
#ifdef NDEBUG
	CGameInstance::Get().ChangeLevel("TO_HOGWART_WORLD");
#endif

	return S_OK;
}

void CMainApp::FrameStart(_float fTimeDelta)
{
	CBaseApp::FrameStart(fTimeDelta);
	GET_SINGLE(UIManager)->Update(fTimeDelta);

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
