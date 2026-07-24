#include "pch.h"

#include "MainAppLoader.h"
#include "GameInstance.h"
#include "PhysXManager.h"
#include "LevelLoading.h"
#include "Resources.h"
#include "Particle_Fire_CPU.h"
#include "Particle_Ribbon.h"
#include "BTMove.h"
#include "BTAnimation.h"
#include "Trail_Example.h"
#include "Particle_Fire_GPU.h"
#include "BTHeader_Definse.h"

// UI
#include "UIManager.h"
#include "EffectUI.h"
#include "TextureUI.h"
#include "TextBox.h"

NS_USING(Client)

HRESULT CMainAppLoader::Load()
{
	{
		std::vector<std::pair<uint32_t, std::string>> layerNames{};
		for (const auto& [layer, name] : magic_enum::enum_entries<COLLISION_LAYER>())
			layerNames.emplace_back(ETOUI(layer), std::string{ name });
		CGameInstance::Get().GetPhysXManager()->SetCollisionLayerNames(std::move(layerNames));
	}

	{
		// TODO   SampleClinet  초기 이니셜라이즈
		{
			if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_SHADER", "VS_VTX_NOR_TEX", CResVertexShader::Create("./ShaderFiles/Shader_VtxNorTex.hlsl")))
			{
				if (FAILED(res->Load()))
				{
					//MSG_BOX("");
					return E_FAIL;
				}
			}

			if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_SHADER", "PS_VTX_NOR_TEX", CResPixelShader::Create("./ShaderFiles/Shader_VtxNorTex.hlsl")))
			{
				if (FAILED(res->Load()))
				{
					//MSG_BOX("");
					return E_FAIL;
				}
			}
			if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_SHADER", "VS_VTX_NOR_TEX_UI", CResPixelShader::Create("./ShaderFiles/Shader_VtxNorTex_UI.hlsl")))
			{
				if (FAILED(res->Load()))
				{
					//MSG_BOX("");
					return E_FAIL;
				}
			}
			if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_SHADER", "PS_VTX_NOR_TEX_UI", CResPixelShader::Create("./ShaderFiles/Shader_VtxNorTex_UI.hlsl")))
			{
				if (FAILED(res->Load()))
				{
					//MSG_BOX("");
					return E_FAIL;
				}
			}
		}


		if (FAILED(Load_Particle_Resources()))
		{
			MSG_BOX("Failed Load_Particle_Resources");
			return E_FAIL;
		}

		if (FAILED(Load_PhysX_Resource()))
		{
			MSG_BOX("Failed Load_PhysX_Resource");
			return E_FAIL;
		}

		if (FAILED(Create_ActionNode()))
		{
			MSG_BOX("Failed Action Node To MainApp");
			return E_FAIL;
		}

		if (FAILED(Load_UIStaitc_Resources()))
		{
			MSG_BOX("Failed Load_UIStaitc_Resources");
			return E_FAIL;
		}

		GET_SINGLE(UIManager)->Initialize(CGameInstance::Get().GetGraphicDevice(), CGameInstance::Get().GetGraphicDeviceContext());
	}



	//클라이언트 사운드 버스 초기화
	{
		if (FAILED(Initialize_Sound()))
		{
			return E_FAIL;
		}
		
	}
	

	
	return S_OK;
}

HRESULT CMainAppLoader::Load_Particle_Resources()
{
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_VSSHADER", "VS_VTX_GPU_PARTICLE_TEX", CResVertexShader::Create("./ShaderFiles/Shader_Structured_Tex_Particle.hlsl")))
	{
		if (!res)
		{
			MSG_BOX("");
			return E_FAIL;
		}
	}

	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_PSSHADER", "PS_VTX_GPU_PARTICLE_TEX", CResPixelShader::Create("./ShaderFiles/Shader_Structured_Tex_Particle.hlsl")))
	{
		if (!res)
		{
			MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_VSSHADER", "VS_VTX_GPU_PARTICLE_MESH", CResVertexShader::Create("./ShaderFiles/Shader_Structured_Mesh_Particle.hlsl")))
	{
		if (!res)
		{
			MSG_BOX("");
			return E_FAIL;
		}
	}

	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_PSSHADER", "PS_VTX_GPU_PARTICLE_MESH", CResPixelShader::Create("./ShaderFiles/Shader_Structured_Mesh_Particle.hlsl")))
	{
		if (!res)
		{
			MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_VSSHADER", "VS_VTX_CPU_PARTICLE_MESH", CResVertexShader::Create("./ShaderFiles/Shader_CPU_Mesh_Particle.hlsl")))
	{
		if (!res)
		{
			MSG_BOX("");
			return E_FAIL;
		}
	}

	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_PSSHADER", "PS_VTX_CPU_PARTICLE_MESH", CResPixelShader::Create("./ShaderFiles/Shader_CPU_Mesh_Particle.hlsl")))
	{
		if (!res)
		{
			MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_PSSHADER", "PS_VTX_CPU_PARTICLE_MESH_SMOKE", CResPixelShader::Create("./ShaderFiles/Shader_CPU_Mesh_Particle.hlsl")))
	{
		if (FAILED(!res->Load(CResShader::DESC{ .sEntryPoint = "PS_SMOKE_MAIN", .sTarget = "ps_5_0" })))
		{
			MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_VSSHADER", "VS_VTX_CPU_PARTICLE_TEX", CResVertexShader::Create("./ShaderFiles/Shader_CPU_Tex_Particle.hlsl")))
	{
		if (!res)
		{
			MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_PSSHADER", "PS_VTX_CPU_PARTICLE_TEX", CResPixelShader::Create("./ShaderFiles/Shader_CPU_Tex_Particle.hlsl")))
	{
		if (!res)
		{
			MSG_BOX("");
			return E_FAIL;
		}
	}	
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_PSSHADER", "PS_VTX_CPU_PARTICLE_TEX_SMOKE", CResPixelShader::Create("./ShaderFiles/Shader_CPU_Tex_Particle.hlsl")))
	{
		if (FAILED(res->Load(CResShader::DESC{ .sEntryPoint = "SMOKE", .sTarget = "ps_5_0" })))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_PSSHADER", "PS_VTX_CPU_PARTICLE_TEX_SMOKE_DEF", CResPixelShader::Create("./ShaderFiles/Shader_CPU_Tex_Particle.hlsl")))
	{
		if (FAILED(res->Load(CResShader::DESC{ .sEntryPoint = "PS_SMOKE_DEF", .sTarget = "ps_5_0" })))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}

	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_VSSHADER", "VS_VTX_RIBBON_PARTICLE_TEX", CResVertexShader::Create("./ShaderFiles/Shader_Ribbon.hlsl")))
	{
		if (!res)
		{
			MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_PSSHADER", "PS_VTX_RIBBON_PARTICLE_TEX", CResPixelShader::Create("./ShaderFiles/Shader_Ribbon.hlsl")))
	{
		if (!res)
		{
			MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_VSSHADER", "VS_VTX_Trail_TEX", CResVertexShader::Create("./ShaderFiles/Shader_Trail.hlsl")))
	{
		if (!res)
		{
			MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_PSSHADER", "PS_VTX_Trail_TEX", CResPixelShader::Create("./ShaderFiles/Shader_Trail.hlsl")))
	{
		if (!res)
		{
			MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_VSSHADER", "VS_VTX_GPU_SPLASH_TEX", CResVertexShader::Create("./ShaderFiles/Shader_GPU_Splash.hlsl")))
	{
		if (!res)
		{
			MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_PSSHADER", "PS_VTX_GPU_SPLASH_TEX", CResPixelShader::Create("./ShaderFiles/Shader_GPU_Splash.hlsl")))
	{
		if (!res)
		{
			MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_VSSHADER", "VS_VTX_GPU_SCROLL_MESH", CResVertexShader::Create("./ShaderFiles/Shader_Structured_Mesh_NoiseScroll.hlsl")))
	{
		if (!res)
		{
			MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_PSSHADER", "PS_VTX_GPU_SCROLL_MESH", CResPixelShader::Create("./ShaderFiles/Shader_Structured_Mesh_NoiseScroll.hlsl")))
	{
		if (!res)
		{
			MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_VSSHADER", "VS_VTX_GPU_SCROLL_X_MESH", CResVertexShader::Create("./ShaderFiles/Shader_Structured_Mesh_UV_XScroll.hlsl")))
	{
		if (!res)
		{
			MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_PSSHADER", "PS_VTX_GPU_SCROLL_X_MESH", CResPixelShader::Create("./ShaderFiles/Shader_Structured_Mesh_UV_XScroll.hlsl")))
	{
		if (!res)
		{
			MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_VSSHADER", "VS_VTX_GPU_SCROLLUV_MESH", CResVertexShader::Create("./ShaderFiles/Shader_Structured_Mesh_Particle_UvScroll.hlsl")))
	{
		if (!res)
		{
			MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_PSSHADER", "PS_VTX_GPU_SCROLLUV_MESH", CResPixelShader::Create("./ShaderFiles/Shader_Structured_Mesh_Particle_UvScroll.hlsl")))
	{
		if (!res)
		{
			MSG_BOX("");
			return E_FAIL;
		}
	}
	////////// -- 광윤 추가 -- //////////
	if (nullptr == CGameInstance::Get().AddResource("PERMANENT_PARTICLE_VSSHADER", "VS_VTX_GPU_LIGHTNING_MESH", CResVertexShader::Create("./ShaderFiles/Shader_Lightning.hlsl")))		 return E_FAIL;

	if (nullptr == CGameInstance::Get().AddResource("PERMANENT_PARTICLE_PSSHADER", "PS_VTX_GPU_LIGHTNING_MESH", CResPixelShader::Create("./ShaderFiles/Shader_Lightning.hlsl")))		 return E_FAIL;
	
	if (nullptr == CGameInstance::Get().AddResource("PERMANENT_PARTICLE_VSSHADER", "VS_VTX_GPU_LIGHTNING_TEX", CResVertexShader::Create("./ShaderFiles/Shader_Lightning_Texture.hlsl"))) return E_FAIL;

	if (nullptr == CGameInstance::Get().AddResource("PERMANENT_PARTICLE_PSSHADER", "PS_VTX_GPU_LIGHTNING_TEX", CResPixelShader::Create("./ShaderFiles/Shader_Lightning_Texture.hlsl")))	 return E_FAIL;
	/////////////////////////////////////

	{
		//노이즈 텍스쳐
		if (auto res = E::CGameInstance::Get().AddResource("SAMPLE_CLINET_TEXTURE", "TEX_NOISE", E::CResTexture2D::Create("./Resources/SampleClient/Textures/EffectParticle/Noise/VFX_T_NoiseGreypack03_D.png")))
		{
			if (FAILED(res->Load()))
			{
				MSG_BOX("");
				//return E_FAIL;
			}
		}
	}

	{
		//파티클 버퍼 생성
		if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_PARTICLEBF", "VIBUF_ParticleQuad", CResQuadTexBuffer::Create()))
		{
			if (FAILED(res->Load()))
			{
				MSG_BOX("파티클 쿼드 버퍼 로드 실패");
				return E_FAIL;
			}
		}
	}

	{
		auto k = CGameInstance::Get().Load_FilePath_ByExtension("./Resources/json/Particle/ParticleData", ".json");
		CGameInstance::Get().Load_ParticleJsonPackage(k);
		//CGameInstance::Get().LoadParticleJson("./Resources/json/Particle/Lightning_Texture.json");
		//CGameInstance::Get().LoadParticlePresets("./Resources/json/Particle/Preset/ParticlePresets.json");
	}
	return S_OK;
}

HRESULT CMainAppLoader::Load_PhysX_Resource()
{
	{
		CGameInstance::Get().AddResource("SAMPLE_CLIENT_PX", "TMP_MATERIAL", CResPhysXMaterial::CreateAndLoad(CResPhysXMaterial::DESC{}));
		CGameInstance::Get().AddResource("SAMPLE_CLIENT_PX", "TMP_GEO_BOX", CResPhysXBoxGeometry::CreateAndLoad(CResPhysXBoxGeometry::DESC{}));
		CGameInstance::Get().AddResource("SAMPLE_CLIENT_PX", "TMP_GEO_SPHERE", CResPhysXSphereGeometry::CreateAndLoad(CResPhysXSphereGeometry::DESC{}));
		CGameInstance::Get().AddResource("SAMPLE_CLIENT_PX", "TMP_GEO_CAPSULE", CResPhysXCapsuleGeometry::CreateAndLoad(CResPhysXCapsuleGeometry::DESC{}));
	}
	return S_OK;
}

HRESULT CMainAppLoader::Create_ActionNode()
{
	//프로토타입 이니셜라이즈랑 이름 맞출것
	if (FAILED(CGameInstance::Get().AddPrototype(NODEGROUP::ACTION, "BTMove", CBTMove::Create())))
		return E_FAIL;
	if (FAILED(CGameInstance::Get().AddPrototype(NODEGROUP::ACTION, "BTTurnDirect", CBTTurnDirect::Create())))
		return E_FAIL;
	if (FAILED(CGameInstance::Get().AddPrototype(NODEGROUP::ACTION, "BTTurnSlow", CBTTurnSlow::Create())))
		return E_FAIL;
	if (FAILED(CGameInstance::Get().AddPrototype(NODEGROUP::ACTION, "BTOnlyFalse", CBTOnlyFalse::Create())))
		return E_FAIL;
	if (FAILED(CGameInstance::Get().AddPrototype(NODEGROUP::ACTION, "BTOnlyTrue", CBTOnlyTrue::Create())))
		return E_FAIL;
	if (FAILED(CGameInstance::Get().AddPrototype(NODEGROUP::ACTION, "BTTeleport", CBTTeleport::Create())))
		return E_FAIL;
	if (FAILED(CGameInstance::Get().AddPrototype(NODEGROUP::ACTION, "BTDamage", CBTDamage::Create())))
		return E_FAIL;
	if (FAILED(CGameInstance::Get().AddPrototype(NODEGROUP::ACTION, "BTCreatureFlag", CBTCreatureFlag::Create())))
		return E_FAIL;
	if (FAILED(CGameInstance::Get().AddPrototype(NODEGROUP::ACTION, "BTMonAttType", CBTMonAttType::Create())))
		return E_FAIL;

	if (FAILED(CGameInstance::Get().AddPrototype(NODEGROUP::ANIMATION, "BTRandMoveAnim", CBTRandMoveAnim::Create())))
		return E_FAIL;
	if (FAILED(CGameInstance::Get().AddPrototype(NODEGROUP::ANIMATION, "BTAnimation", CBTAnimation::Create())))
		return E_FAIL;
	if (FAILED(CGameInstance::Get().AddPrototype(NODEGROUP::ANIMATION, "BTTurnAnimation", CBTTurnAnimation::Create())))
		return E_FAIL;
	if (FAILED(CGameInstance::Get().AddPrototype(NODEGROUP::ANIMATION, "BTAttackAnimation", CBTAttackAnimation::Create())))
		return E_FAIL;
	if (FAILED(CGameInstance::Get().AddPrototype(NODEGROUP::ANIMATION, "BTHitAnimMonster", CBTHitAnimMonster::Create())))
		return E_FAIL;

	if (FAILED(CGameInstance::Get().AddPrototype(NODEGROUP::DECORATOR, "BTDecSearch", CBTDecSearch::Create())))
		return E_FAIL;
	if (FAILED(CGameInstance::Get().AddPrototype(NODEGROUP::DECORATOR, "BTDecTimer", CBTDecTimer::Create())))
		return E_FAIL;
	if (FAILED(CGameInstance::Get().AddPrototype(NODEGROUP::DECORATOR, "BTDecLier", CBTDecLier::Create())))
		return E_FAIL;
	if (FAILED(CGameInstance::Get().AddPrototype(NODEGROUP::DECORATOR, "BTDecInvert", CBTDecInvert::Create())))
		return E_FAIL;
	if (FAILED(CGameInstance::Get().AddPrototype(NODEGROUP::DECORATOR, "BTDecFlag", CBTDecFlag::Create())))
		return E_FAIL;
	if (FAILED(CGameInstance::Get().AddPrototype(NODEGROUP::DECORATOR, "BTDecHp", CBTDecHp::Create())))
		return E_FAIL;
	if (FAILED(CGameInstance::Get().AddPrototype(NODEGROUP::DECORATOR, "BTDecHitCnt", CBTDecHitCnt::Create())))
		return E_FAIL;
	
	return S_OK;
}

HRESULT CMainAppLoader::Initialize_Sound()
{
	auto* pSoundManager = CGameInstance::Get().GetSoundManager();

	for (const auto eBus : magic_enum::enum_values<Client::SOUND_BUS>())
	{
		if (eBus == Client::SOUND_BUS::END)
			continue;

		if (!pSoundManager->CreateBus(eBus))
			return E_FAIL;
	}

	// 사운드 테스트
	if(false)
	{
		const _string sSoundPath = "./Resources/SampleClient/Sound/Verses_1_4_of_the_National_Anthem.mp3";
		auto* pSoundManager = CGameInstance::Get().GetSoundManager();
		if (pSoundManager == nullptr ||
			!pSoundManager->Preload(sSoundPath))
			return E_FAIL;

		const SOUND_ID iSoundID = pSoundManager->Play2D(
			sSoundPath,
			E::SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::VOICE,
				.fVolume = 1.f,
				.fPitch = 1.f,
				.iPriority = 64,
				.bLoop = true
			});
		if (iSoundID == INVALID_SOUND_ID)
			return E_FAIL;
	}
	return S_OK;
}

HRESULT CMainAppLoader::Load_UIStaitc_Resources()
{
	{
		namespace fs = std::filesystem;

		std::string targetDir = "./Resources/SampleClient/Textures/UI/TexUI/LoadingScreen";

		if (fs::exists(targetDir) && fs::is_directory(targetDir))
		{
			for (const auto& entry : fs::directory_iterator(targetDir))
			{
				if (entry.is_regular_file() && entry.path().extension() == ".png")
				{
					std::string fileName = entry.path().stem().string();


					std::string resTag = "TEX_" + fileName;

					std::string fullPath = entry.path().generic_string();

					if (auto res = E::CGameInstance::Get().AddResource("LEVEL_LOADING", resTag, E::CResTexture2D::Create(fullPath)))
					{
						res->Load();
					}
				}
			}
		}
	}
	if (auto res = E::CGameInstance::Get().AddResource("LEVEL_LOADING", "Flipbook_LoadingWidget_Houses", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/UI_T_LoadingWidget_Houses.png")))
	{
		res->Load();
	}

	if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_LOADING", "Prototype_GameObject_TextureUI", CTextureUI::Create())))
	{
		return false;
	}
	if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_LOADING", "Prototype_GameObject_EffectUI", CEffectUI::Create())))
	{
		return false;
	}
	if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_LOADING", "Prototype_GameObject_TextBox", CTextBox::Create())))
	{
		return false;
	}

	return S_OK;
}
