#include "pch.h"
#include "LevelUIEditorLoader.h"

#include "GameInstance.h"
#include "BackGround.h"
#include "EffectUI.h"
#include "TextureUI.h"
#include "Button.h"
#include "TextBox.h"
#include "SpellMeter.h"
#include "HPBar.h"
#include "MiniMap.h"
#include "GameOverMask.h"

NS_USING(Client)
std::future<bool> CLevelUIEditorLoader::Load()
{
	// 메인 스레드 종료
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_UIEDITOR", []()
		{
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
			if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "TEX_T_WaterCaustics_Disorder_A", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/TexUI/SpellMeter/T_WaterCaustics_Disorder_A.png")))
			{
				res->Load();
			}
			if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "TEX_UI_T_spellmeter_Generic", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/TexUI/SpellMeter/UI_T_spellmeter_Generic.png")))
			{
				res->Load();
			}
			if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "TEX_VFX_T_Wavy_N", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/TexUI/SpellMeter/VFX_T_Wavy_N.png")))
			{
				res->Load();
			}
			if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "TEX_VFX_T_WispyNoise_D", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/TexUI/SpellMeter/VFX_T_WispyNoise_D.png")))
			{
				res->Load();
			}
			if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "TEX_T_CollectionsMeterLine_A", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/TexUI/SpellMeter/T_CollectionsMeterLine_A.png")))
			{
				res->Load();
			}
			if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "TEX_UI_T_arrestomomentum", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/TexUI/SpellMeter/UI_T_arrestomomentum.png")))
			{
				res->Load();
			}
			{
				namespace fs = std::filesystem;

				std::string targetDir = "./Resources/SampleClient/Textures/UI/TexUI";

				if (fs::exists(targetDir) && fs::is_directory(targetDir))
				{
					for (const auto& entry : fs::directory_iterator(targetDir))
					{
						if (entry.is_regular_file() && entry.path().extension() == ".png")
						{
							std::string fileName = entry.path().stem().string();


							std::string resTag = "TEX_" + fileName;

							std::string fullPath = entry.path().generic_string();

							if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", resTag, E::CResTexture2D::Create(fullPath)))
							{
								res->Load();
							}
						}
					}
				}
			}

			// 워커 스레드
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
			if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_UIEDITOR", "Prototype_GameObject_SpellMeter", CSpellMeter::Create())))
			{
				return false;
			}
			if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_UIEDITOR", "Prototype_GameObject_HPBar", CHPBar::Create())))
			{
				return false;
			}
			if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_UIEDITOR", "Prototype_GameObject_MiniMap", CMiniMap::Create())))
			{
				return false;
			}
			if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_UIEDITOR", "Prototype_GameObject_GameOverMask", CGameOverMask::Create())))
			{
				return false;
			}
			// 워커 스레드 종료
			return  true;
		});
}

std::future<bool> CLevelUIEditorLoader::UnLoad()
{
	LOG_MEMORY("start");
	E::CGameInstance::Get().DelPrototype("LEVEL_UIEDITOR");
	E::CGameInstance::Get().DelResource("LEVEL_UIEDITOR");
	LOG_MEMORY("end");

	return E::CGameInstance::Get().WorkerEnqueueWithFuture("UNLOADING_UIEDITOR", []()
		{
			return true;
		});
}
