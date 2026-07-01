#include "pch.h"

#include "LevelLoading.h"
#include "GameInstance.h"
#include "Resources.h"
#include "Client_Resources.h"
#include "LevelLogo.h"
#include "BackGround.h"
#include "LevelPlayground.h"
#include "Terrain.h"
#include "TestModel.h"
#include "LevelUIEditor.h"

NS_USING(Client)

CLevelLoading::CLevelLoading(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext, LEVEL eNextLevelIndex) noexcept
	: m_pDevice{ pDevice }
	, m_pContext{ pContext }
	, m_eNextLevelIndex(eNextLevelIndex)
{
}

CLevelLoading::~CLevelLoading()
{
}

HRESULT CLevelLoading::Initialize()
{
	Engine::CGameInstance::Get().GameObjectAllReset();


	return S_OK;
}

void CLevelLoading::Update(E::_float fTimeDelta)
{
	if (!m_bThreadStart)
	{
		m_bThreadStart = true;

		ThreadStart();
	}

	LoadingCheck();

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
	case LEVEL::PLAYGROUND:
		pNewLevel = CLevelPlayground::Create();
		break;
	case LEVEL::UIEDITOR:
		pNewLevel = CLevelUIEditor::Create();
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

void CLevelLoading::ThreadStart()
{
	switch (m_eNextLevelIndex)
	{
	case LEVEL::LOGO:
	{
		m_futLoadFinish = E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_LOGO", [this]()
			{
				if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_LOGO", "Prototype_GameObject_BackGround", CBackGround::Create())))
				{
					return false;
				}

				//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				return  true;
			});

		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_LOGO", "TEX_SHM", E::CResTexture2D::Create("./Resources/SampleClient/Textures/SHM.png")))
		{
			res->Load();
		}

	}
	break;
	case LEVEL::PLAYGROUND:
	{
		if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_TEX", "TEX2D_Terrain_Tile0", CResTexture2D::Create("./Resources/SampleClient/Textures/Terrain/Tile0.dds")))
		{
			if (FAILED(res->Load()))
			{
				MSG_BOX("");
				//return E_FAIL;
			}
		}



		

		m_futLoadFinish = E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_PLAYGROUND", [this]()
			{

				if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_BUFFER", "VIBUFFER_Terrain", CResTerrainVIBuffer::Create("./Resources/SampleClient/Textures/Terrain/Height.bmp")))
				{
					if (FAILED(res->Load(CResTerrainVIBuffer::DESC{})))
					{
						//MSG_BOX("");
						return false;
					}
				}

				if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_PLAYGROUND", "Prototype_GameObject_Terrain", CTerrain::Create())))
				{
					return false;
				}
				if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_PLAYGROUND", "Prototype_GameObject_TestModel", CTestModel::Create())))
				{
					return false;
				}


				//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				return  true;
			});

	}
	break;
	case LEVEL::UIEDITOR:
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "TEX_SHM", E::CResTexture2D::Create("./Resources/SampleClient/Textures/SHM.png")))
		{
			res->Load();
		}
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "TEX_MAP", E::CResTexture2D::Create("./Resources/SampleClient/Textures/T_Map_OverlandPaper_D.png")))
		{
			res->Load();
		}
		
		m_futLoadFinish = E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_UIEDITOR", [this]()
			{
				if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_UIEDITOR", "Prototype_GameObject_BackGround", CBackGround::Create())))
				{
					return false;
				}

				//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				return  true;
			});

	
		break;
	default:
		m_bLoadEnd = true;
		break;
	}

}

void CLevelLoading::LoadingCheck()
{
	if (m_futLoadFinish.valid())
	{
		if (m_futLoadFinish.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
		{
			m_bLoadEnd = m_futLoadFinish.get();

			if (!m_bLoadEnd)
			{
				MSG_BOX("LOADING FAILD");
			}
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
	CLevel::Free();
}
