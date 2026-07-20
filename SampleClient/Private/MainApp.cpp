#include "pch.h"

#include "MainApp.h"
#include "GameInstance.h"
#include "LevelLoading.h"
#include "Resources.h"
#include "Particle_Fire_CPU.h"
#include "Particle_Ribbon.h"
#include "BTMove.h"
#include "BTAnimation.h"
#include "Trail_Example.h"
#include "Particle_Fire_GPU.h"
#include "BTHeader_Definse.h"
#include "Client_Defines.h"
#include "UIManager.h"

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
	if (!_CrtCheckMemory()) { OutputDebugStringA("CORRUPT: CMainApp::Initialize VERY START\n"); __debugbreak(); }

	Engine::ENGINE_DESC EngineDesc{};
	EngineDesc.hWnd = g_hWnd;
	EngineDesc.hInstance = g_hInstance;
	EngineDesc.eWinMode = Engine::WINMODE::WIN;
	EngineDesc.iWinSizeX = g_iWinSizeX;
	EngineDesc.iWinSizeY = g_iWinSizeY;

	if (FAILED(CBaseApp::Initialize(EngineDesc)))
	{
		return E_FAIL;
	}
	CGameInstance::Get().ImguiEnableDocking(true, true);

	if (CBaseApp::StartLevel(CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::LOGO)))
	{
		return E_FAIL;
	}
	CGameInstance::Get().RegisterLevelChangeFunc("TO_LOGO", [=]() {
		Engine::CGameInstance::Get().ChangeLevel(
			CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::LOGO));
		});

	CGameInstance::Get().RegisterLevelChangeFunc("TO_Playground", [=]() {
		Engine::CGameInstance::Get().ChangeLevel(
			CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::PLAYGROUND));
		});

	CGameInstance::Get().RegisterLevelChangeFunc("TO_UIEditor", [=]() {
		Engine::CGameInstance::Get().ChangeLevel(
			CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::UIEDITOR));
		});

	CGameInstance::Get().RegisterLevelChangeFunc("TO_ANIMEditor", [=]() {
		Engine::CGameInstance::Get().ChangeLevel(
			CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::ANIMEDITOR));
		});

	CGameInstance::Get().RegisterLevelChangeFunc("TO_LightMap", [=]() {
		Engine::CGameInstance::Get().ChangeLevel(
			CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::LIGHTMAP));
		});

	CGameInstance::Get().RegisterLevelChangeFunc("TO_Collider", [=]() {
		Engine::CGameInstance::Get().ChangeLevel(
			CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::COLLIDER));
		});

	CGameInstance::Get().RegisterLevelChangeFunc("TO_Physx", [=]() {
		Engine::CGameInstance::Get().ChangeLevel(
			CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::PHYSX));
		});

	if (FAILED(CMainAppLoader::Load()))
	{
		MSG_BOX("MainLoader Failed");
		return E_FAIL;
	}
	
	
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
