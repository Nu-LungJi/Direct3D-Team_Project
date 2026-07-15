#include "pch.h"

#include "LevelLoading.h"
#include "GameInstance.h"
#include "Resources.h"
#include "Client_Resources.h"
#include "LevelLogo.h"
#include "BackGround.h"
#include "LevelPlayground.h"
#include "Terrain.h"
#include "Particle.h"

#include "LevelUIEditor.h"
#include "LevelAnimEditor.h"
#include "LevelLightMap.h"
#include "LevelCollider.h"
#include "TestCollider.h"
#include "LevelPhysX.h"
#include "TestPhysX.h"
#include "TestPhysXTerrain.h"
#include "TestPhysXBox.h"
#include "TestPhysXBall.h"
#include "TestPhysXCapsule.h"
#include "LightObject.h"
#include "CTexUI.h"
#include "FlipBook.h"
#include "TextureUI.h"
#include "EffectUI.h"
#include "TextBox.h"
#include "Weapon.h"
#include "Button.h"

#include "TestGob.h"

#include "TestCharacter.h"

#include "LevelLogoLoader.h"
#include "LevelPhysXLoader.h"
#include "LevelColliderLoader.h"
#include "LevelAnimatorLoader.h"
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
	case LEVEL::ANIMEDITOR:
		pNewLevel = CLevelAnimEditor::Create();
		break;
	case LEVEL::COLLIDER:
		pNewLevel = CLevelCollider::Create();
		break;
	case LEVEL::LIGHTMAP:
		pNewLevel = CLevelLightMap::Create();
		break;
	case LEVEL::PHYSX:
		pNewLevel = CLevelPhysX::Create();
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
		m_futLoadFinish = CLevelLogoLoader::Load();
		//m_futLoadFinish = E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_LOGO", [this]()
		//	{
		//		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_LOGO", "Prototype_GameObject_BackGround", CBackGround::Create())))
		//		{
		//			return false;
		//		}

		//		//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		//		return  true;
		//	});

		//if (auto res = E::CGameInstance::Get().AddResource("LEVEL_LOGO", "TEX_SHM", E::CResTexture2D::Create("./Resources/SampleClient/Textures/SHM.png")))
		//{
		//	res->Load();
		//}
	}
	break;
	case LEVEL::PLAYGROUND:
	{
	
	}
	break;
	case LEVEL::UIEDITOR:
		/* Texture */
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "TEX_SHM", E::CResTexture2D::Create("./Resources/SampleClient/Textures/SHM.png")))
		{
			res->Load();
		}
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "TEX_MAP", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/T_Map_OverlandPaper_D.png")))
		{
			res->Load();
		}
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "TEX_UI_T_NurtureMeterDiamond_Back_4k", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/TexUI/UI_T_NurtureMeterDiamond_Back_4k.png")))
		{
			res->Load();
		}
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "TEX_UI_T_NurtureMeterDiamond_Ready_4k", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/TexUI/UI_T_NurtureMeterDiamond_Ready_4k.png")))
		{
			res->Load();
		}
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "TEX_UI_T_NurtureMeterDiamond_Outer_4k", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/TexUI/UI_T_NurtureMeterDiamond_Outer_4k.png")))
		{
			res->Load();
		}
		/* Mask */
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "MASK_UI_T_ButtonFlameTopClamp", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/TexUI/UI_T_ButtonFlameTopClamp.png")))
		{
			res->Load();
		}
		
		/* FlipBook */
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "Flipbook_LoadingWidget_Flame", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/UI_T_LoadingWidget_Flame.png")))
		{
			res->Load();
		}
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "Flipbook_LoadingWidget_Houses", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/UI_T_LoadingWidget_Houses.png")))
		{
			res->Load();
		}
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "Flipbook_VFXSmokeSim_D", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/UI_T_VFXSmokeSim_D.png")))
		{
			res->Load();
		}
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "Flipbook_VFX_T_ItemSpark_8x8_D", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/UI_T_VFX_T_ItemSpark_8x8_D.png")))
		{
			res->Load();
		}
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "Flipbook_VFX_T_PopVFX_8x8_D", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/UI_T_VFX_T_PopVFX_8x8_D.png")))
		{
			res->Load();
		}
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "Flipbook_VFX_BlinkingStars", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/UI_T_VFX_BlinkingStars.png")))
		{
			res->Load();
		}
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "Flipbook_UI_T_MagicEffect1", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/FlipBook/UI_T_MagicEffect1.png")))
		{
			res->Load();
		}
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "Flipbook_UI_T_SmokeWispy_D", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/FlipBook/UI_T_SmokeWispy_D.png")))
		{
			res->Load();
		}
		{
			namespace fs = std::filesystem;

			std::string targetDir = "./Resources/SampleClient/Textures/UI/TexUI";

			// 1. �ش� ������ �����ϴ��� ���� Ȯ�� (������ġ)
			if (fs::exists(targetDir) && fs::is_directory(targetDir))
			{
				// 2. ���� ���� ��� ������ ��ȸ
				for (const auto& entry : fs::directory_iterator(targetDir))
				{
					// 3. �����̸鼭 Ȯ���ڰ� .png ���� Ȯ��
					if (entry.is_regular_file() && entry.path().extension() == ".png")
					{
						// 4. ���� �̸��� ���� (��: "UI_T_NurtureMeterDiamond_Back_4k")
						std::string fileName = entry.path().stem().string();

						// 5. ���ҽ� �±� ���� (��: "TEX_UI_T_NurtureMeterDiamond_Back_4k")
						std::string resTag = "TEX_" + fileName;

						// 6. ��ü ���� ��� ���� (�ü���� �°� ��ΰ� ���յ�)
						// generic_string()�� ���� �����쿡���� ��������(\) ��� ������(/)�� ��θ� ��ȯ�մϴ�.
						std::string fullPath = entry.path().generic_string();

						// 7. ������ ���ҽ� �߰� �� �ε�
						if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", resTag, E::CResTexture2D::Create(fullPath)))
						{
							res->Load();
						}
					}
				}
			}
		}
	
		
		m_futLoadFinish = E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_UIEDITOR", [this]()
			{
				if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_UIEDITOR", "Prototype_GameObject_BackGround", CBackGround::Create())))
				{
					return false;
				}
				if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_UIEDITOR", "Prototype_GameObject_TextureUI", CTextureUI::Create())))
				{
					return false;
				}
				if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_UIEDITOR", "Prototype_GameObject_EffectUI", CEffectUI::Create())))
				{
					return false;
				}
				if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_UIEDITOR", "Prototype_GameObject_TextBox", CTextBox::Create())))
				{
					return false;
				}
				if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_UIEDITOR", "Prototype_GameObject_Button", CButton::Create())))
				{
					return false;
				}

				//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				return  true;
			});

	
		break;
	case LEVEL::ANIMEDITOR:
	{
		//if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TEST", "Prototype_GameObject_LightObject", CLightObject::Create())))
		//{
		//	int a = 0;
		//	//return false;
		//}
		m_futLoadFinish = CLevelAnimatorLoader::Load();
	}
		break;
	case LEVEL::COLLIDER:
	{
		m_futLoadFinish = CLevelColliderLoader::Load();
	}
	break;
	case LEVEL::LIGHTMAP:
	{
		//"LIGHT", "Prototype_GameObject_TestModel"
		//"LIGHT", "Prototype_GameObject_Terrain"
		//"SAMPLE_CLIENT_TEX", "TEX2D_Terrain_Tile0" p
		//"LIGHT", "Prototype_GameObject_LightObject"
		//"SAMPLE_CLIENT_BUFFER", "VIBUFFER_Terrain"

	}
	break;
	case LEVEL::PHYSX:
	{
		m_futLoadFinish = CLevelPhysXLoader::Load();
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
