#include "pch.h"

#include "LevelLoading.h"
#include "GameInstance.h"
#include "Resources.h"
#include "LevelLogo.h"
#include "LevelMapEditor.h"
#include "Particle.h"

#include "TestGuizmo.h"
#include "MapMeshObject.h"
#include "ResMapEditorTerrainVIBuffer.h"
#include "MapEditorTerrain.h"

#include <cctype>
#include <filesystem>

NS_USING(Client)

namespace
{
	std::string MakeStaticModelResourceTag(const std::filesystem::path& rootPath, const std::filesystem::path& binPath)
	{
		std::filesystem::path relativePath = binPath.lexically_relative(rootPath);
		if (relativePath.empty())
		{
			relativePath = binPath.filename();
		}

		relativePath.replace_extension();

		std::string resourceTag = relativePath.string();
		for (char& ch : resourceTag)
		{
			const unsigned char value = static_cast<unsigned char>(ch);
			if (!std::isalnum(value))
			{
				ch = '_';
			}
		}

		return resourceTag;
	}

	bool LoadLevelAnimEditorStaticModels()
	{
		const std::filesystem::path staticModelDir = E::PATH_MAPEDITOR_STATIC_MODEL_DIR;
		if (!std::filesystem::exists(staticModelDir))
		{
			return false;
		}

		for (const auto& entry : std::filesystem::recursive_directory_iterator(staticModelDir))
		{
			if (!entry.is_regular_file() || entry.path().extension() != ".bin")
			{
				continue;
			}

			const std::string resourceTag = MakeStaticModelResourceTag(staticModelDir, entry.path());
			auto res = E::CGameInstance::Get().AddResourceT<E::CResStaticModel>(
				E::TAG_RES_GRP_MAPEDITOR_STATIC_MODEL,
				resourceTag,
				E::CResStaticModel::Create(entry.path().string()));

			if (!res)
			{
				return false;
			}

			E::CResStaticModel::DESC desc{};
			desc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f);

			if (FAILED(res->Load(desc)))
			{
				return false;
			}
		}

		return true;
	}
}

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
	case LEVEL::MAPEDITOR:
		pNewLevel = CLevelMapEditor::Create();
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
				/*if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_LOGO", "Prototype_GameObject_BackGround", CBackGround::Create())))
				{
					return false;
				}*/

				//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				return  true;
			});

		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_LOGO", "TEX_SHM", E::CResTexture2D::Create("./Resources/SampleClient/Textures/SHM.png")))
		{
			res->Load();
		}
	}
	break;
	case LEVEL::MAPEDITOR:
	{

		// 터레인 띄우려고 SampleClient에서 복붙해옴
		{
			if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_TEX", "TEX2D_Terrain_Tile0", CResTexture2D::Create("./Resources/SampleClient/Textures/Terrain/Tile0.dds")))
			{
				if (FAILED(res->Load()))
				{
					MSG_BOX("");
				}
			}

			if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_BUFFER", "VIBUFFER_Terrain", CResMapEditorTerrainVIBuffer::Create("./Resources/SampleClient/Textures/Terrain/Height.bmp")))
			{
				if (FAILED(res->Load(CResMapEditorTerrainVIBuffer::DESC{})))
				{
					MSG_BOX("");
				}
			}
		}

		if (!LoadLevelAnimEditorStaticModels())
		{
			MSG_BOX("");
		}
		m_futLoadFinish = E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_MAPEDITOR", [this]()
			{
			
				// 터레인
				if (FAILED(E::CGameInstance::Get().AddPrototype("MAPEDITOR", "Prototype_GameObject_MapEditorTerrain", CMapEditorTerrain::Create())))
				{
					return false;
				}

				////TestGuizmo
				//if (FAILED(CGameInstance::Get().AddPrototype("MAPEDITOR", "Prototype_GameObject_TestGuizmo", CTestGuizmo::Create())))
				//{
				//	return false;
				//}

				return true;
			});

		//if (auto res = E::CGameInstance::Get().AddResource("LEVEL_LOGO", "TEX_SHM", E::CResTexture2D::Create("./Resources/SampleClient/Textures/SHM.png")))
		//{
		//	res->Load();
		//}
	}
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
