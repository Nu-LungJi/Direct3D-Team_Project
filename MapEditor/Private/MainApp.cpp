#include "pch.h"

#include "MainApp.h"
#include "GameInstance.h"
#include "LevelLoading.h"
#include "Resources.h"

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

	CGameInstance::Get().RegisterLevelChangeFunc("TO_MAPEDITOR", [=]() {
		Engine::CGameInstance::Get().ChangeLevel(
			CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::MAPEDITOR));
		});



	// 터레인 띄우려고 SampleClient에서 복붙해온 셰이더
	{
		if (auto res = CGameInstance::Get().AddResource(
			"SAMPLE_CLIENT_SHADER",
			"VS_VTX_NOR_TEX",
			CResVertexShader::Create("./ShaderFiles/Shader_VtxNorTex.hlsl")))
		{
			if (FAILED(res->Load()))
				return E_FAIL;
		}

		if (auto res = CGameInstance::Get().AddResource(
			"SAMPLE_CLIENT_SHADER",
			"PS_VTX_NOR_TEX",
			CResPixelShader::Create("./ShaderFiles/Shader_VtxNorTex.hlsl")))
		{
			if (FAILED(res->Load()))
				return E_FAIL;
		}
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
