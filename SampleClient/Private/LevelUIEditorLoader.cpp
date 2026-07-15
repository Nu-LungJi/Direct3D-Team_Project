#include "pch.h"
#include "LevelUIEditorLoader.h"

#include "GameInstance.h"
#include "BackGround.h"
#include "EffectUI.h"
#include "TextureUI.h"
#include "Button.h"
#include "TextBox.h"

NS_USING(Client)
std::future<bool> CLevelUIEditorLoader::Load()
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

	// 메인 스레드 종료
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_UIEDITOR", []()
		{
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
			// 워커 스레드 종료
			return  true;
		});
}

HRESULT CLevelUIEditorLoader::UnLoad()
{
	LOG_MEMORY("start");
	E::CGameInstance::Get().DelPrototype("LEVEL_LOGO");
	E::CGameInstance::Get().DelResource("LEVEL_LOGO");
	LOG_MEMORY("end");

	return S_OK;
}
