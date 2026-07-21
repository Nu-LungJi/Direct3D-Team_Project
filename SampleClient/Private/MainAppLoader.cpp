#include "pch.h"

#include "MainAppLoader.h"
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

#include "UIManager.h"

NS_USING(Client)

HRESULT CMainAppLoader::Load()
{
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

		GET_SINGLE(UIManager)->Initialize(CGameInstance::Get().GetGraphicDevice(), CGameInstance::Get().GetGraphicDeviceContext());
	}
	return S_OK;
}

HRESULT CMainAppLoader::Load_Particle_Resources()
{
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_VSSHADER", "VS_VTX_GPU_PARTICLE_TEX", CResVertexShader::Create("./ShaderFiles/Shader_Structured_Tex_Particle.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}

	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_PSSHADER", "PS_VTX_GPU_PARTICLE_TEX", CResPixelShader::Create("./ShaderFiles/Shader_Structured_Tex_Particle.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_VSSHADER", "VS_VTX_GPU_PARTICLE_MESH", CResVertexShader::Create("./ShaderFiles/Shader_Structured_Mesh_Particle.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}

	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_PSSHADER", "PS_VTX_GPU_PARTICLE_MESH", CResPixelShader::Create("./ShaderFiles/Shader_Structured_Mesh_Particle.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_VSSHADER", "VS_VTX_CPU_PARTICLE_MESH", CResVertexShader::Create("./ShaderFiles/Shader_CPU_Mesh_Particle.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}

	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_PSSHADER", "PS_VTX_CPU_PARTICLE_MESH", CResPixelShader::Create("./ShaderFiles/Shader_CPU_Mesh_Particle.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}

	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_VSSHADER", "VS_VTX_CPU_PARTICLE_TEX", CResVertexShader::Create("./ShaderFiles/Shader_CPU_Tex_Particle.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_PSSHADER", "PS_VTX_CPU_PARTICLE_TEX", CResPixelShader::Create("./ShaderFiles/Shader_CPU_Tex_Particle.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_VSSHADER", "VS_VTX_RIBBON_PARTICLE_TEX", CResVertexShader::Create("./ShaderFiles/Shader_Ribbon.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_PSSHADER", "PS_VTX_RIBBON_PARTICLE_TEX", CResPixelShader::Create("./ShaderFiles/Shader_Ribbon.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_VSSHADER", "VS_VTX_Trail_TEX", CResVertexShader::Create("./ShaderFiles/Shader_Trail.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_PSSHADER", "PS_VTX_Trail_TEX", CResPixelShader::Create("./ShaderFiles/Shader_Trail.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_VSSHADER", "VS_VTX_GPU_SPLASH_TEX", CResVertexShader::Create("./ShaderFiles/Shader_GPU_Splash.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_PSSHADER", "PS_VTX_GPU_SPLASH_TEX", CResPixelShader::Create("./ShaderFiles/Shader_GPU_Splash.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_VSSHADER", "VS_VTX_GPU_SCROLL_MESH", CResVertexShader::Create("./ShaderFiles/Shader_Structured_Mesh_NoiseScroll.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_PSSHADER", "PS_VTX_GPU_SCROLL_MESH", CResPixelShader::Create("./ShaderFiles/Shader_Structured_Mesh_NoiseScroll.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_VSSHADER", "VS_VTX_GPU_SCROLLUV_MESH", CResVertexShader::Create("./ShaderFiles/Shader_Structured_Mesh_Particle_UvScroll.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("PERMANENT_PARTICLE_PSSHADER", "PS_VTX_GPU_SCROLLUV_MESH", CResPixelShader::Create("./ShaderFiles/Shader_Structured_Mesh_Particle_UvScroll.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}

	{
		//파티클 텍스쳐 로드
		//if (auto res = E::CGameInstance::Get().AddResource("SAMPLE_CLINET_TEXTURE", "TEX_RIBBONDISTORTION", E::CResTexture2D::Create("./Resources/SampleClient/Textures/EffectParticle/T_FX_Distortion_Ring3_N.png")))
		//{
		//	if (FAILED(res->Load()))
		//	{
		//		MSG_BOX("");
		//		//return E_FAIL;
		//	}
		//}
		//if (auto res = E::CGameInstance::Get().AddResource("SAMPLE_CLINET_TEXTURE", "TEX_RIBBON", E::CResTexture2D::Create("./Resources/SampleClient/Textures/EffectParticle/VFX_T_AncientMagicStreak_E.png")))
		//{
		//	if (FAILED(res->Load()))
		//	{
		//		MSG_BOX("");
		//		//return E_FAIL;
		//	}
		//}
		//if (auto res = E::CGameInstance::Get().AddResource("SAMPLE_CLINET_TEXTURE", "TEX_TRAIL", E::CResTexture2D::Create("./Resources/SampleClient/Textures/EffectParticle/trail.png")))
		//{
		//	if (FAILED(res->Load()))
		//	{
		//		MSG_BOX("");
		//		return E_FAIL;
		//	}
		//}

		//// model-> cpu  용인지 gpu용인지 구분하고 메쉬를 쓰는지 텍스쳐를 쓰는지 구분을하고 
		//if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>("Rock1", "Static_Model_Resource",
		//	CResStaticModel::Create("./Resources/SampleClient/Models/OriginData/Static/SM_rock1.fbx"))) {

		//	E::CResStaticModel::DESC pDesc{};
		//	pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f);

		//	if (FAILED(res->Load(pDesc)))
		//	{
		//		return E_FAIL;
		//	}
		//}

	}

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
		//if (auto res = E::CGameInstance::Get().AddResource("SAMPLE_CLINET_TEXTURE", "TEX_RIBBONNOISE", E::CResTexture2D::Create("./Resources/SampleClient/Textures/EffectParticle/trail.png")))
		//{
		//	if (FAILED(res->Load()))
		//	{
		//		MSG_BOX("");
		//		//return E_FAIL;
		//	}
		//}
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
		CGameInstance::Get().LoadParticleJson("./Resources/json/Particle/ParticleData.json");
		CGameInstance::Get().LoadParticlePresets("./Resources/json/Particle/Preset/ParticlePresets.json");
		//파티클 객채들 생성
		//CGameInstance::Get().Add_Particle("FIRE", "FIREBALL", CParticle_Fire_CPU::Create());
		//CGameInstance::Get().Add_Particle("FIRE", "FIRESMOKE", CParticle_Fire_GPU::Create());
		//CGameInstance::Get().Add_Particle("BEAM", "ATTACK", CParticle_Ribbon::Create());
		//CGameInstance::Get().Add_Particle("TRAIL", "SLASH", CTrail_Example::Create());

	}
	return S_OK;
}

HRESULT CMainAppLoader::Load_PhysX_Resource()
{
	{
		CGameInstance::Get().AddResource("SAMPLE_CLIENT_PHYSIX", "TMP_MATERIAL", CResPhysXMaterial::Create(CResPhysXMaterial::DESC{}));
		CGameInstance::Get().AddResource("SAMPLE_CLIENT_PHYSIX", "TMP_GEO_BOX", CResPhysXBoxGeometry::Create(CResPhysXBoxGeometry::DESC{}));
		CGameInstance::Get().AddResource("SAMPLE_CLIENT_PHYSIX", "TMP_GEO_SHPERE", CResPhysXSphereGeometry::Create(CResPhysXSphereGeometry::DESC{}));
		CGameInstance::Get().AddResource("SAMPLE_CLIENT_PHYSIX", "TMP_GEO_CAPSULE", CResPhysXCapsuleGeometry::Create(CResPhysXCapsuleGeometry::DESC{}));
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
	if (FAILED(CGameInstance::Get().AddPrototype(NODEGROUP::DECORATOR, "BTDecHit", CBTDecHit::Create())))
		return E_FAIL;
	if (FAILED(CGameInstance::Get().AddPrototype(NODEGROUP::DECORATOR, "BTDecHp", CBTDecHp::Create())))
		return E_FAIL;
	return S_OK;
}
