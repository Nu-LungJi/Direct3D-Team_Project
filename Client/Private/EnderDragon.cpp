#include "pch.h"
#include "EnderDragon.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComModelInstance.h"
#include "ComAnimator.h"
#include "Resources.h"
#include "ComBeHavior.h"
#include "GameInstance.h"
#include "ComCollider.h"
#include "ComPxCharacterController.h"
#include "ComCharacterMoveIntent.h"
#include "ComCharacterMotor.h"
#include "DbgLineRender.h"
#include "ComPxRigidBody.h"
#include "ComPxSphereCollider.h"
#include "UIController.h"
#include "UIManager.h"
//FSM
#include "EnderDragon_State.h"
#include "Edg_Spawn.h"
#include "Edg_Combat.h"
#include "Edg_Hit.h"
#include "Edg_Phase.h"
#include "Edg_Dead.h"
#include "Edg_Godae.h"
//BB
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
//Skill
#include "EdgFireBall.h"
#include "EdgBreath.h"
#include "EdgPulse.h"
#include "Trail_CPU.h"
NS_USING(Client)

CEnderDragon::CEnderDragon()
{
}


CEnderDragon::~CEnderDragon()
{
}

void CEnderDragon::UpdateGUI()
{
	__super::UpdateGUI();
	ImGui::DragInt("HP", &m_iHp, 0, 1);
	
	if (ImGui::Button("AddWay"))
	{
		auto pSrc = CGameInstance::Get().GetActiveCamera();
		if (nullptr == pSrc) return;

		m_DebugPoint.push_back(pSrc->GetTransform().GetPosition());
	}
	if (ImGui::Button("DelWay"))
	{
		if (!m_DebugPoint.empty())
			m_DebugPoint.pop_back();
	}
	if (ImGui::Button("SaveWay"))
		m_bPopup = true;
	if (ImGui::Button("LoadWay"))
	{
		m_bPopupL = true;
	}
	if (m_bPopupL)
	{
		ImGui::OpenPopup("LoadWay");
		_char NameBUffer[64]{};
		if (ImGui::BeginPopup("LoadWay", ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("FileName");
			if (ImGui::InputText("##FileName", &NameBUffer[0], IM_ARRAYSIZE(NameBUffer))) //이름 입력
			{
				m_WayName = NameBUffer;

			}
			if (ImGui::Button("Ok"))
			{
				m_bPopupL = false;
				if (m_WayName.empty())
				{
					ImGui::CloseCurrentPopup();
					MSG_BOX("NoName");
				}
				else
				{
					m_DebugPoint.clear();
					auto pRes = CGameInstance::Get().GetResourceFirst<CResJson>("EDGWAYPT", m_WayName);
					if (nullptr == pRes)
					{
						MSG_BOX("Load Failed Json To EDGWAYPT SPAWN");
						return;
					}
					auto json = pRes->Get_Json();
					JsonSaveLoadManager::LoadJsonTypeFloat3list(json, m_WayName, m_DebugPoint);
				}
			} ImGui::SameLine(100.f);
			if (ImGui::Button("Cancle"))
			{
				m_bPopupL = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		
	}
	if (m_bPopup)
	{
		_string Path = "./Resources/json/WayPoint/";
		ImGui::OpenPopup("SaveWay");
		_char NameBUffer[64]{};
		if (ImGui::BeginPopup("SaveWay", ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("FileName");
			if (ImGui::InputText("##FileName", &NameBUffer[0], IM_ARRAYSIZE(NameBUffer))) //이름 입력
			{
				m_WayName = NameBUffer;

			}
			if (ImGui::Button("Ok"))
			{
				m_bPopup = false;
				if (m_WayName.empty())
				{
					ImGui::CloseCurrentPopup();
					MSG_BOX("NoName");
				}
				else
				{
					nlohmann::json j;
					JsonSaveLoadManager::SaveJsonTypeFloat3list(j, m_WayName, m_DebugPoint);
					Path += m_WayName + ".json";
					std::ofstream path(Path);
					path << j.dump(4);
					path.close();
				}
			} ImGui::SameLine(100.f);
			if (ImGui::Button("Cancle"))
			{
				m_bPopup = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}
	if (ImGui::Button("Clear"))
		m_DebugPoint.clear();
	Phase_Debug();
}

HRESULT CEnderDragon::InitializePrototype(void* pArg)
{
	if (FAILED(__super::InitializePrototype(pArg)))
	{
		return E_FAIL;
	}
	/*----------- 광윤 추가 -----------*/
	if (m_pResDragonBodyPixelShader = CGameInstance::Get().AddResourceT<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_DragonBody", "./ShaderFiles/Shader_EnderDragon.hlsl")) {
		if (FAILED(m_pResDragonBodyPixelShader->Load(CResShader::DESC{ .sEntryPoint = "PSMain_DragonBody", .sTarget = "ps_5_0" })))    return E_FAIL;
	}
	if (m_pResDragonWingPixelShader = CGameInstance::Get().AddResourceT<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_DragonWing", "./ShaderFiles/Shader_EnderDragon.hlsl")) {
		if (FAILED(m_pResDragonWingPixelShader->Load(CResShader::DESC{ .sEntryPoint = "PSMain_DragonWing", .sTarget = "ps_5_0" })))    return E_FAIL;
	}
	if (m_pResDragonWingFXPixelShader = CGameInstance::Get().AddResourceT<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_EtherealWing", "./ShaderFiles/Shader_EnderDragon.hlsl")) {
		if (FAILED(m_pResDragonWingFXPixelShader->Load(CResShader::DESC{ .sEntryPoint = "PSMain_EtherealWing", .sTarget = "ps_5_0" })))    return E_FAIL;
	}
	if (m_pResDragonEyePixelShader = CGameInstance::Get().AddResourceT<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_DragonEye", "./ShaderFiles/Shader_EnderDragon.hlsl")) {
		if (FAILED(m_pResDragonEyePixelShader->Load(CResShader::DESC{ .sEntryPoint = "PSMain_DragonEye", .sTarget = "ps_5_0" })))    return E_FAIL;
	}
	
	m_pBodyMaskTexture		= CResTexture2D::Create("./Resources/SampleClient/Textures/Skeleton/Dragon/T_ConjuredDragon_Body_MSK.dds");
	if (!m_pBodyMaskTexture || FAILED(m_pBodyMaskTexture->Load()))	return E_FAIL;

	m_pWingsMaskTexture		= CResTexture2D::Create("./Resources/SampleClient/Textures/Skeleton/Dragon/T_ConjuredDragon_Wings_MSK.dds");
	if (!m_pWingsMaskTexture || FAILED(m_pWingsMaskTexture->Load()))	return E_FAIL;

	m_pBodyMROTexture		= CResTexture2D::Create("./Resources/SampleClient/Textures/Skeleton/Dragon/T_ConjuredDragon_Body_MRO.dds");
	if (!m_pBodyMROTexture || FAILED(m_pBodyMROTexture->Load()))		return E_FAIL;

	m_pWingsMROTexture		= CResTexture2D::Create("./Resources/SampleClient/Textures/Skeleton/Dragon/T_ConjuredDragon_Wings_MRO.dds");
	if (!m_pWingsMROTexture || FAILED(m_pWingsMROTexture->Load()))		return E_FAIL;

	m_pEtherealWingsTexture = CResTexture2D::Create("./Resources/SampleClient/Textures/Skeleton/Dragon/VFX_T_ConjuredDragonWing_M.dds");
	if (!m_pEtherealWingsTexture || FAILED(m_pEtherealWingsTexture->Load()))	return E_FAIL;

	m_pMarbleNoiseTexture	= CResTexture2D::Create("./Resources/SampleClient/Textures/Skeleton/Dragon/VFX_T_MarbleSkin_01_BPF_D.dds");
	if (!m_pMarbleNoiseTexture || FAILED(m_pMarbleNoiseTexture->Load()))	return E_FAIL;

	m_pRiverNoiseTexture	= CResTexture2D::Create("./Resources/SampleClient/Textures/Skeleton/Dragon/VFX_T_NoiseCaustics02_D.dds");
	if (!m_pRiverNoiseTexture || FAILED(m_pRiverNoiseTexture->Load()))	return E_FAIL;

	m_pCausticNoiseTexture	= CResTexture2D::Create("./Resources/SampleClient/Textures/Skeleton/Dragon/VFX_T_CausticNoise_C_Seamless_D.dds");
	if (!m_pCausticNoiseTexture || FAILED(m_pCausticNoiseTexture->Load()))		return E_FAIL;

	m_pDetailNoiseTexture	 = CResTexture2D::Create("./Resources/SampleClient/Textures/Skeleton/Dragon/VFX_T_Noise08_D.dds");
	if (!m_pDetailNoiseTexture || FAILED(m_pDetailNoiseTexture->Load()))		return E_FAIL;

	if (m_pResWingFXRasterizer = CGameInstance::Get().AddResourceT<CResRasterizerState>(TAG_RES_GRP_PERMANENT_STATE, "RS_DRAGON_WING_FX", CResRasterizerState::Create())) {
		D3D11_RASTERIZER_DESC Desc{};

		Desc.FillMode = D3D11_FILL_SOLID;
		Desc.CullMode = D3D11_CULL_NONE;
		Desc.DepthClipEnable = TRUE;

		Desc.DepthBias = -10;
		Desc.SlopeScaledDepthBias = -0.5f;
		Desc.DepthBiasClamp = 0.f;

		if (FAILED(m_pResWingFXRasterizer->Load(Desc)))	return E_FAIL;
	}

	if (m_pResWingFXDSS = CGameInstance::Get().AddResourceT<CResDepthStencilState>(TAG_RES_GRP_PERMANENT_STATE, "DS_DRAGON_WING_FX", CResDepthStencilState::Create())) {
		D3D11_DEPTH_STENCIL_DESC Desc{};

		Desc.DepthEnable = TRUE;
		Desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		Desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		Desc.StencilEnable = FALSE;

		if (FAILED(m_pResWingFXDSS->Load(Desc)))	return E_FAIL;
	}

	/*---------------------------------*/
	return S_OK;
}

HRESULT CEnderDragon::Initialize(void* pArg)
{
	auto MonDesc = static_cast<DRAGON_DESC*>(pArg);
	if (FAILED(__super::Initialize(pArg)))
	{
		return E_FAIL;
	}
	m_iHp = m_iMaxHp = 555;
	
	{
		CComPxRigidBody::DESC Desc{};
		Desc.eType = CComPxRigidBody::TYPE::KINEMATIC;
		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::PHYSX,
			ES_EngineProtoPhysXComponent::Prototype_Component_ComPxRigidBody, "ComPxRigidBody", &Desc, &m_pComRigidBody)))
		{
			MSG_BOX("Create Failed ComPxRigidBody EnderDragon");
			return E_FAIL;
		}
	}

	{
		CComPxSphereCollider::DESC Desc{};
		Desc.pComPxRigidBody = m_pComRigidBody;
		Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});
		Desc.bIsTrigger = false;
		Desc.tFilter = PX_FILTER_DESC{
			.iLayer = ETOUI(COLLISION_LAYER::ENEMY_HURTBOX),
			.iSimulationMask = ETOUI(COLLISION_LAYER::PLAYER_PROJECTILE),
			//.iQueryMask = ETOUI(COLLISION_LAYER::PLAYER_PROJECTILE),
		};
		Desc.pResSphereGeo = CResPhysXSphereGeometry::CreateAndLoad({ .fRadius = 1.2f });
		if (!Desc.pResMaterial ||
			FAILED(AddComponentFromProto(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxSphereCollider,
				"ComPxSphereCollider", &Desc, &m_pComSphereCol)))
		{
			MSG_BOX("Create Failed ComPxSphereCollider EnderDragon");
			return E_FAIL;
		}
		if (!m_pComSphereCol->SetQueryEnabled(false))
			return E_FAIL;
	}

	//피직스
	{
		CComPxCharacterController::DESC Desc{};
		Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});
		const _float fHorizontalScale =
			std::max(std::abs(MonDesc->vScale.x), std::abs(MonDesc->vScale.z));
		const _float fVerticalScale = std::abs(MonDesc->vScale.y);
		const _float3 vCenterOffset{
			MonDesc->vCCTCenterOffset.x * MonDesc->vScale.x,
			MonDesc->vCCTCenterOffset.y * fVerticalScale,
			MonDesc->vCCTCenterOffset.z * MonDesc->vScale.z };
		Desc.fHeight = MonDesc->fCCTHeight * fVerticalScale;
		Desc.fRadius = MonDesc->fCCTRadius * fHorizontalScale;
		Desc.fStepOffset = MonDesc->fCCTStepOffset;
		Desc.vPosition = {
			MonDesc->vPos.x + vCenterOffset.x,
			MonDesc->vPos.y + vCenterOffset.y,
			MonDesc->vPos.z + vCenterOffset.z };
		Desc.tFilter = MonDesc->tFilter;
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PHYSX,
			ES_EngineProtoPhysXComponent::Prototype_Component_ComPxCharacterController,
			"ComPxCharacterController", &Desc, &m_pCharacterController)))
		{
			return E_FAIL;
		}
	}
	//캐릭컨트롤러
	{
		CComCharacterMoveIntent::DESC Desc{};
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoComponent::Prototype_Component_ComCharacterMoveIntent,
			"ComCharacterMoveIntent", &Desc, &m_pMoveIntent)))
		{
			return E_FAIL;
		}
	}
	//캐릭 모터
	{
		CComCharacterMotor::DESC Desc{};
		Desc.pMoveIntent = m_pMoveIntent;
		Desc.pCharacterController = m_pCharacterController;
		Desc.fGravity = -9.81f;
		Desc.vControllerCenterOffset = {
			MonDesc->vCCTCenterOffset.x * MonDesc->vScale.x,
			MonDesc->vCCTCenterOffset.y * std::abs(MonDesc->vScale.y),
			MonDesc->vCCTCenterOffset.z * MonDesc->vScale.z };
		Desc.bUseGravity = true;
		Desc.bSyncTransform = true;
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoComponent::Prototype_Component_ComCharacterMotor,
			"ComCharacterMotor", &Desc, &m_pCharacterMotor)))
		{
			return E_FAIL;
		}
	}

	CComBeHavior::BEHAVIOR_DESC Desc{};
	Desc.OwnerName = "Com_BT";
	Desc.resBeHaviorMajor = MonDesc->resBeHaviorMajor;
	Desc.resBeHaviorMinor = MonDesc->resBeHaviorMinor;
	if (FAILED(AddComponentFromProto("BEHAVIOR", "Prototype_Component_BeHavior", "Com_BT", &Desc, &m_pBeHavior)))
	{
		return E_FAIL;
	};
	{
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject", &Desc, &m_pComCBufferPerObject)))
		{
			return E_FAIL;
		};
	}
	{
		CComModelInstance::DESC Desc{};
		Desc.sGroupTag = MonDesc->LevelTag;
		Desc.sResTag = MonDesc->ReSourceTag;

		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ModelInstance", "ComCModelIntance", &Desc, &m_pComModelInstance)))
		{
			return E_FAIL;
		};
	}
	/*----------- 광윤 추가 -----------*/
	{
		CComModelInstance::DESC Desc{};
		Desc.sGroupTag = MonDesc->LevelTag;
		Desc.sResTag = "Model_Resource_Dragon_BoneModel";

		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ModelInstance", "ComCModelIntance_Outliner", &Desc, &m_pComOutlineModelInstance)))
		{
			return E_FAIL;
		};
	}
	/*---------------------------------*/
	{
		CComAnimator::DESC DescAnim{};
		DescAnim.sComTag = "ComCModelIntance";

		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_Animator", "ComCModelAnimator", &DescAnim, &m_pModelAnimator)))
		{
			return E_FAIL;
		};
	}
	{
		CComCollider::DESC Desc{};
		Desc.eCollType = CollType::Box;
		Desc.vExtents = { 1.f, 1.f, 1.f };
		if (FAILED(AddComponentFromProto("COLLIDER", "Prototype_Component_Collider", "ComColl", &Desc, &m_pComCollider)))
		{
			return E_FAIL;
		};
	}
	
	if (FAILED(Ready_Fsm(MonDesc->LevelTag)))
	{
		MSG_BOX("Create Failed Fsm");
		return E_FAIL;
	}
	if (FAILED(Ready_Skill(MonDesc->LevelTag)))
	{
		MSG_BOX("Create Failed Skill");
		return E_FAIL;
	}
	Ready_BBKeyValue();

	GetTransform().SetPosition(m_pCharacterController->GetFootPosition());
	GetTransform().Update();
	m_pModelAnimator->SetEvaluationMode(CComAnimator::EVALUATION_MODE::CPU_GPU);
	m_pModelAnimator->Build_BoneMatrices_CPU(0.f);

	GetTransform().SetPosition(XMLoadFloat3(&MonDesc->vPos));
	m_eMonType = MONSTER_TYPE::BOSS;
	InitializeEffects();
	ReadySound();
	m_pComSphereCol->SetQueryEnabled(true);
	m_iColliderBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("chest_targetSocket");
	m_iLeft1WingParticleBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("indexmiddlewing_03_left");
	m_iRight1WingParticleBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("indexmiddlewing_03_right");
	m_iLeft2WingParticleBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("indexmiddlewing_04_left");
	m_iRight2WingParticleBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("indexmiddlewing_04_right");
	return S_OK;
}
void CEnderDragon::ReadySound()
{
	m_SoundTable["PulseReady"] = { "./Resources/SampleClient/Sound/LastBossRanrok/Ambient/BeforeSinra.wav", };

	m_SoundTable["WingDefault"] = { "./Resources/SampleClient/Sound/LastBossRanrok/Move/enemies_dragon_conjured_akb__WIngSmall.wav",
			"./Resources/SampleClient/Sound/LastBossRanrok/Move/enemies_dragon_conjured_akb__WIngSmall2.wav",
	"./Resources/SampleClient/Sound/LastBossRanrok/Move/enemies_dragon_conjured_akb__WIngSmall3.wav",
	"./Resources/SampleClient/Sound/LastBossRanrok/Move/enemies_dragon_conjured_akb__WIngSmall4.wav" };

	m_SoundTable["WingMove"] = { "./Resources/SampleClient/Sound/LastBossRanrok/Move/enemies_dragon_conjured_akb__WingMove1.wav",
	"./Resources/SampleClient/Sound/LastBossRanrok/Move/enemies_dragon_conjured_akb__WingMove2.wav",
	"./Resources/SampleClient/Sound/LastBossRanrok/Move/enemies_dragon_conjured_akb__WingMove3.wav", 
	"./Resources/SampleClient/Sound/LastBossRanrok/Move/enemies_dragon_conjured_akb__WingMove4.wav", };

	m_SoundTable["Doljin"]={ 
	"./Resources/SampleClient/Sound/LastBossRanrok/Move/enemies_dragon_conjured_akb__Doljin2.wav"

	};
	m_SoundTable["Phase"] = {
	"./Resources/SampleClient/Sound/LastBossRanrok/enemies_dragon_conjured_akb__PhaseChange.wav"
	};
	m_SoundTable["Hit"] = {
		"./Resources/SampleClient/Sound/LastBossRanrok/enemies_dragon_conjured_akb__Hit.wav"
	};
	m_SoundTable["Houling"] = {
		"./Resources/SampleClient/Sound/LastBossRanrok/enemies_dragon_conjured_akb__Houling.wav"
	};
	m_SoundTable["Ground"] = {
	"./Resources/SampleClient/Sound/LastBossRanrok/enemies_dragon_conjured_akb__MaybeGround.wav"
	};
}
HRESULT CEnderDragon::Ready_Fsm(const _string& LevelTag)
{
	CEnderDragon_State::DESC Desc{};
	if (FAILED(AddComponentFromProto(LevelTag, "Prototype_Component_Dragon_FSM", "EnderDragon_Fsm", &Desc, &m_pFsm))) return E_FAIL;
	
	if (false == m_pFsm->Add_State(MON_STATE::SPAWN, CEdg_Spawn::Create(LevelTag))) return E_FAIL;

	if (false == m_pFsm->Add_State(MON_STATE::COMBAT, CEdg_Combat::Create())) return E_FAIL;

	if (false == m_pFsm->Add_State(MON_STATE::HIT, CEdg_Hit::Create())) return E_FAIL;

	if (false == m_pFsm->Add_State(MON_STATE::PHASE_CHANGE, CEdg_Phase::Create(LevelTag))) return E_FAIL;

	if (false == m_pFsm->Add_State(MON_STATE::DEAD, CEdg_Dead::Create())) return E_FAIL;

	if (false == m_pFsm->Add_State(MON_STATE::GODAE, CEdg_Godae::Create(this))) return E_FAIL;

	if (false == m_pFsm->Initialize_State(MON_STATE::SPAWN)) return E_FAIL;


	return S_OK;
}
HRESULT CEnderDragon::Ready_Skill(const _string& LevelTag)
{
	if (ETOUI(DRAGON_SKILL::END) > ETOUI(ATTMON::END))
		return E_FAIL;

	m_MonSkillLists[ATTMON::SLOT0] = ETOUI(DRAGON_SKILL::BOOM);
	m_MonSkillLists[ATTMON::SLOT1] = ETOUI(DRAGON_SKILL::BREATH);
	m_MonSkillLists[ATTMON::SLOT2] = ETOUI(DRAGON_SKILL::FIREBALL);
	m_MonSkillLists[ATTMON::SLOT3] = ETOUI(DRAGON_SKILL::PULSE);
	m_MonSkillLists[ATTMON::SLOT4] = ETOUI(DRAGON_SKILL::RANDOMBALL);
	m_MonSkillLists[ATTMON::SLOT5] = ETOUI(DRAGON_SKILL::THREEBALL);
	m_MonSkillLists[ATTMON::SLOT6] = ETOUI(DRAGON_SKILL::BLACKBALL);
	m_MonSkillLists[ATTMON::SLOT7] = ETOUI(DRAGON_SKILL::TURNBREATH);
	m_MonSkillLists[ATTMON::SLOT8] = ETOUI(DRAGON_SKILL::LONGBREATH);
	m_MonSkillLists[ATTMON::SLOT9] = ETOUI(DRAGON_SKILL::GASI);
	m_MonSkillLists[ATTMON::SLOT10] = ETOUI(DRAGON_SKILL::GASIBREATH);
	m_MonSkillLists[ATTMON::SLOT11] = ETOUI(DRAGON_SKILL::GROUND);
	//////////////////////파티클 넣는곳/////////////////////////
	m_EffectNames[ETOUI(DRAGON_SKILL::FIREBALL)]   = "FireBall";
	m_EffectNames[ETOUI(DRAGON_SKILL::BREATH)]	   = "DragonBreath";
	m_EffectNames[ETOUI(DRAGON_SKILL::TURNBREATH)] = "DragonBreath";
	m_EffectNames[ETOUI(DRAGON_SKILL::LONGBREATH)] = "DragonBreath";
	m_EffectNames[ETOUI(DRAGON_SKILL::GASIBREATH)] = "DragonBreath";
	m_EffectNames[ETOUI(DRAGON_SKILL::PULSE)]	   = "PulseSphere";
	m_EffectNames[ETOUI(DRAGON_SKILL::RANDOMBALL)] = "RandomBall";
	m_EffectNames[ETOUI(DRAGON_SKILL::THREEBALL)]  = "FireBall";
	m_EffectNames[ETOUI(DRAGON_SKILL::BLACKBALL)]  = "BlackBall";
	m_EffectNames[ETOUI(DRAGON_SKILL::GASI)]		= "BreathSpike";
	m_EffectNames[ETOUI(DRAGON_SKILL::GROUND)] = "GroundEffect";
	////////////////////////////////////////////////////////////
	CDragonSkill::EDG_SKILL_DESC SkillDesc{};
	int32_t iHeadBoneIndex{};

	int32_t iOffsetBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("SKT_Mouth");

	iHeadBoneIndex = SkillDesc.iBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("SKT_Head");
	SkillDesc.iOffsetBoneIndex = iOffsetBoneIndex;
	SkillDesc.hOwner = GetHandle();
	auto BreathHandle = CGameInstance::Get().AddGameObjectToLayer(LevelTag, PROTO_GAMEOBJECT::Prototype_GameObject_Dragon_Breath, "03.Breath", &SkillDesc);
	if (!BreathHandle) return E_FAIL;

	SkillDesc.iBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("chest_Main");
	SkillDesc.eType = DRAGON_SKILL::PULSE;
	auto PulseHandle = CGameInstance::Get().AddGameObjectToLayer(LevelTag, PROTO_GAMEOBJECT::Prototype_GameObject_Dragon_Pulse, "03.Pulse", &SkillDesc);
	if (!PulseHandle) return E_FAIL;


	SkillDesc.eType = DRAGON_SKILL::GASI;
	auto GasiHandle = CGameInstance::Get().AddGameObjectToLayer(LevelTag, PROTO_GAMEOBJECT::Prototype_GameObject_Dragon_Gasi, "03.Gasi", &SkillDesc);
	if (!GasiHandle) return E_FAIL;

	///////////

	m_SkillHandle[ETOUI(DRAGON_SKILL::FIREBALL)] = EDG_SKILL_INFO{ .bPool = false, .iBoneIndex = iHeadBoneIndex,
	.LevelTag = LevelTag, .ProtoTag = PROTO_GAMEOBJECT::Prototype_GameObject_Dragon_FireBall, .NameTag = "03.FireBall",.iOffsetBoneIndex = iOffsetBoneIndex,
	.eType = DRAGON_SKILL::FIREBALL};
	
	m_SkillHandle[ETOUI(DRAGON_SKILL::THREEBALL)] = EDG_SKILL_INFO{ .bPool = false, .iBoneIndex = iHeadBoneIndex,
	.LevelTag = LevelTag, .ProtoTag = PROTO_GAMEOBJECT::Prototype_GameObject_Dragon_FireBall, .NameTag = "03.ThreeBall",.iOffsetBoneIndex = iOffsetBoneIndex,
	.eType = DRAGON_SKILL::THREEBALL };

	m_SkillHandle[ETOUI(DRAGON_SKILL::BLACKBALL)] = EDG_SKILL_INFO{ .bPool = false, .iBoneIndex = iHeadBoneIndex,
	.LevelTag = LevelTag, .ProtoTag = PROTO_GAMEOBJECT::Prototype_GameObject_Dragon_FireBall, .NameTag = "03.BlackBall",.iOffsetBoneIndex = iOffsetBoneIndex,
	.eType = DRAGON_SKILL::BLACKBALL };

	

	int32_t iBallBone = m_pComModelInstance->GetModel()->Get_BoneIndex("chest_Main");
	m_SkillHandle[ETOUI(DRAGON_SKILL::RANDOMBALL)] = EDG_SKILL_INFO{ .bPool = false, .iBoneIndex = iBallBone,
	.LevelTag = LevelTag, .ProtoTag = PROTO_GAMEOBJECT::Prototype_GameObject_Dragon_RandomBall, .NameTag = "03.RandomBall", };


	m_SkillHandle[ETOUI(DRAGON_SKILL::LONGBREATH)] = EDG_SKILL_INFO{ .handle = BreathHandle.value(),.bPool = true };
	m_SkillHandle[ETOUI(DRAGON_SKILL::TURNBREATH)] = EDG_SKILL_INFO{ .handle = BreathHandle.value(),.bPool = true};
	m_SkillHandle[ETOUI(DRAGON_SKILL::BREATH)]	   = EDG_SKILL_INFO{ .handle = BreathHandle.value(),.bPool = true,};
	m_SkillHandle[ETOUI(DRAGON_SKILL::GASIBREATH)] = EDG_SKILL_INFO{ .handle = BreathHandle.value(),.bPool = true, };

	m_SkillHandle[ETOUI(DRAGON_SKILL::PULSE)]  = EDG_SKILL_INFO{.handle = PulseHandle.value() , .bPool = true,};
	m_SkillHandle[ETOUI(DRAGON_SKILL::GASI)]   = EDG_SKILL_INFO{ .handle = GasiHandle.value() , .bPool = true,};
	return S_OK;
}
void CEnderDragon::Ready_BBKeyValue()
{
	auto pBB = Get_BlackBoard();
	pBB->Set_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE, m_ePhase);
	pBB->Set_Value<MOVE>(EDG_KEY::EPATROL, MOVE::LEFT);
	pBB->Set_Value<_bool>(EDG_KEY::EDGEFFECT, false);

	
}
void CEnderDragon::PriorityUpdate(E::_float fTimeDelta)
{
	if (m_bEndGame)
	{
		SetPendingDestroy();
		return;
	}
	if (CGameInstance::Get().KeyPressing(DIK_LSHIFT) && CGameInstance::Get().KeyDown(DIK_H))
		m_bDebug = !m_bDebug;
	
	if (!m_bDebug) return;

	Check_Phase();
	m_pFsm->PriorityUpdate(fTimeDelta);
	Update_BBToFsm();
	__super::PriorityUpdate(fTimeDelta);
	m_pFsm->Update(fTimeDelta);
}

void CEnderDragon::Update(E::_float fTimeDelta)
{
	if (m_bEndGame) return;
	if (!m_bDebug) return;
	__super::Update(fTimeDelta);
	Update_EnvironmentParticles(fTimeDelta);
	Update_WingParticles(fTimeDelta);
}

void CEnderDragon::Update_EnvironmentParticles(_float fTimeDelta)
{
	m_fBlobEnvSpawnAcc += fTimeDelta;
	m_fSwirlEnvSpawnAcc += fTimeDelta;

	if (m_fBlobEnvSpawnAcc >= m_fBlobEnvSpawnInterval)
	{
		m_fBlobEnvSpawnAcc = 0.f;
		m_fBlobEnvSpawnInterval = 0.5f;
		Spawn_EnvironmentParticles(0, 1);
	}

	if (m_fSwirlEnvSpawnAcc >= m_fSwirlEnvSpawnInterval)
	{
		m_fSwirlEnvSpawnAcc = 0.f;
		m_fSwirlEnvSpawnInterval = 1.5f;
		const uint32_t iSwirlIndex = 1u + static_cast<uint32_t>(Randf(0.f, 4.999f));
		Spawn_EnvironmentParticles(iSwirlIndex, 1);
	}
}

void CEnderDragon::Spawn_EnvironmentParticles(uint32_t iParticleIndex, uint32_t iCount)
{
	struct ENV_PARTICLE_RESOURCE
	{
		const _char* pQueueName;
	};

	static constexpr ENV_PARTICLE_RESOURCE PARTICLE_RESOURCES[] =
	{
		{ "BlobEnv.json" },
		{ "SwirlEnv1.json" },
		{ "SwirlEnv2.json" },
		{ "SwirlEnv3.json" },
		{ "SwirlEnv4.json" },
		{ "SwirlEnv5.json" }
	};

	if (iParticleIndex >= std::size(PARTICLE_RESOURCES) || 0u == iCount)
		return;

	const _float3 vBossPosition = GetTransform().GetPosition();
	const ENV_PARTICLE_RESOURCE& resource = PARTICLE_RESOURCES[iParticleIndex];

	for (uint32_t i = 0; i < iCount; ++i)
	{
		const _float fAngle = Randf(0.f, XM_2PI);
		const _float fRadiusRatio = Randf(0.f, 1.f);
		const _float fMinRadius = 10.f;
		const _float fMaxRadius = 70.f;
		const _float fRadius = sqrtf(fMinRadius * fMinRadius + fRadiusRatio * (fMaxRadius * fMaxRadius - fMinRadius * fMinRadius));
		const _float3 vSpawnPosition = _float3(vBossPosition.x + cosf(fAngle) * fRadius, vBossPosition.y + Randf(-20.f, 20.f), vBossPosition.z + sinf(fAngle) * fRadius);

		_float4x4 spawnWorld{};
		XMStoreFloat4x4(&spawnWorld, XMMatrixTranslation(vSpawnPosition.x, vSpawnPosition.y, vSpawnPosition.z));
		CGameInstance::Get().Spawn(resource.pQueueName, spawnWorld);
	}
}

void CEnderDragon::Update_WingParticles(_float fTimeDelta)
{
	m_fWingParticleSpawnAcc += fTimeDelta;
	if (m_fWingParticleSpawnAcc < m_fWingParticleSpawnInterval)
		return;

	m_fWingParticleSpawnAcc = std::fmod(m_fWingParticleSpawnAcc, m_fWingParticleSpawnInterval);
	Spawn_WingParticle(m_iLeft1WingParticleBoneIndex);
	Spawn_WingParticle(m_iRight1WingParticleBoneIndex);
	Spawn_WingParticle(m_iLeft2WingParticleBoneIndex);
	Spawn_WingParticle(m_iRight2WingParticleBoneIndex);
}

void CEnderDragon::Spawn_WingParticle(int32_t iBoneIndex)
{
	if (nullptr == m_pComModelInstance || iBoneIndex < 0)
		return;

	const auto& combinedBoneMatrices = m_pComModelInstance->Get_CombinedBoneMatrices();
	if (static_cast<size_t>(iBoneIndex) >= combinedBoneMatrices.size())
		return;

	const _matrix matBoneWorld = XMLoadFloat4x4(&combinedBoneMatrices[static_cast<size_t>(iBoneIndex)]) * GetTransform().GetLoadedWorldMatrix();
	const _vector vSpawnPosition = matBoneWorld.r[3];
	_float4x4 spawnWorld{};
	XMStoreFloat4x4(&spawnWorld, XMMatrixTranslationFromVector(vSpawnPosition));
	CGameInstance::Get().Spawn("RanrokBodySmoke.json", spawnWorld);
}

void CEnderDragon::Stuck()
{
	if (nullptr == m_pFsm) return;

	m_pFsm->Request_State(MON_STATE::GODAE);
}

void CEnderDragon::FixedUpdate(E::_float fTimeDelta)
{
	if (m_bEndGame) return;
	if (!m_bDebug) return;
	m_pCharacterMotor->FixedUpdate(fTimeDelta);
}
void CEnderDragon::LateUpdate(E::_float fTimeDelta)
{
	if (!m_bDebug) return;
	m_pFsm->LateUpdate(fTimeDelta);
	__super::LateUpdate(fTimeDelta);

}
/*----------- 광윤 추가 -----------*/
HRESULT	CEnderDragon::Render_Instanced(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx, const E::MODEL_INSTANCE_BATCH& Batch) {
	{
		if (!pContext || !m_pResVertexCPUSkinningInstancedShader || !m_pResDragonBodyPixelShader)	return E_FAIL;

		SPtr<CResModel> pModel{};
		uint32_t iInstanceCount = 0;
		
		HRESULT hr = Prepare_DragonInstancing(pContext, Batch, pModel, iInstanceCount);
		if (FAILED(hr)) return hr;
		if (S_FALSE == hr) return S_OK;

		ComPtr<ID3D11RasterizerState> pPreviousRasterizer;
		pContext->RSGetState(pPreviousRasterizer.GetAddressOf());

		auto NoCullRasterizer = E::CGameInstance::GetConst().GetResourceFirst<CResRasterizerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_NOCULL);
		auto BackCullRasterizer = E::CGameInstance::GetConst().GetResourceFirst<CResRasterizerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_BACKCULL);

		m_pComModelInstance->Bind_Materials(pContext, m_fEMissiveColor, m_fIntensive, { 1.f, 1.f, 1.f }, m_fDissolve, 1.f);

		for (uint32_t iMeshIndex = 0; iMeshIndex < pModel->Get_NumMeshes(); ++iMeshIndex) {
			auto& mesh = pModel->GetMeshes()[iMeshIndex];
			if (!mesh) continue;

			const uint32_t iMaterialIndex = mesh->Get_MaterialIndex();
			if (iMaterialIndex != 1 && iMaterialIndex != 3 && iMaterialIndex != 4)	continue;

			if (FAILED(Bind_SkinnedMeshConstantBuffer(pContext, pModel, mesh, iMeshIndex))) return E_FAIL;

			ID3D11ShaderResourceView* pMaterialMaskSRV = nullptr;

			switch (iMaterialIndex) {
				case 1: {
					pContext->PSSetShader(m_pResDragonEyePixelShader->GetPixelShader().Get(), nullptr, 0);

					pContext->PSSetShaderResources(2, 1, m_pWingsMROTexture->GetSRV().GetAddressOf());

					pMaterialMaskSRV = m_pWingsMaskTexture->GetSRV().Get();

					pContext->RSSetState(NoCullRasterizer->GetRasterizerState().Get());
					break;
				}
				case 3: {
					pContext->PSSetShader(m_pResDragonWingPixelShader->GetPixelShader().Get(), nullptr, 0);

					pContext->PSSetShaderResources(2, 1, m_pWingsMROTexture->GetSRV().GetAddressOf());

					pMaterialMaskSRV = m_pWingsMaskTexture->GetSRV().Get();

					pContext->RSSetState(NoCullRasterizer->GetRasterizerState().Get());
					break;
				}
				case 4: {
					pContext->PSSetShader(m_pResDragonBodyPixelShader->GetPixelShader().Get(), nullptr, 0);

					pContext->PSSetShaderResources(2, 1, m_pBodyMROTexture->GetSRV().GetAddressOf());

					pMaterialMaskSRV = m_pBodyMaskTexture->GetSRV().Get();

					pContext->RSSetState(BackCullRasterizer->GetRasterizerState().Get());
					break;
				}
				default: continue;
			}

			if (!pMaterialMaskSRV)	return E_FAIL;
			pContext->PSSetShaderResources(4, 1, &pMaterialMaskSRV);

			ID3D11Buffer* vertexBuffer = mesh->GetVertexBuffer().Get();
			const UINT stride = mesh->GetVertexStride();
			const UINT offset = 0;

			pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
			pContext->IASetIndexBuffer(mesh->GetIndexBuffer().Get(), mesh->GetIndexFormat(), 0);
			pContext->IASetPrimitiveTopology(mesh->GetPrimitiveType());

			m_pComModelInstance->Bind_Textures(pContext, iMeshIndex);

			pContext->DrawIndexedInstanced(mesh->GetNumIndices(), iInstanceCount, 0, 0, 0);
		}
		pContext->RSSetState(pPreviousRasterizer.Get());

		ID3D11ShaderResourceView* nullVSSRVs[3]{};
		pContext->VSSetShaderResources(6, 3, nullVSSRVs);
		ID3D11ShaderResourceView* pNullPSSRV[5]{};
		pContext->PSSetShaderResources(4, 5, pNullPSSRV);
	}

	// Wing -> Render_Alpha()
	m_pPendingWingFXBatch = &Batch;
	if (!m_bWingFXQueued) {
		if (FAILED(CGameInstance::Get().AddRenderObject(RENDERGROUP::BLEND, this))) {
			m_pPendingWingFXBatch = nullptr;
			return E_FAIL;
		}
		m_bWingFXQueued = true;
	}

	return S_OK;
}
HRESULT CEnderDragon::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) {	// Called Render_Alpha()
	if (m_bHide) return  S_OK;
	HRESULT Result = S_OK;

	if (m_pPendingWingFXBatch) {
		const E::MODEL_INSTANCE_BATCH* pBatch = m_pPendingWingFXBatch;
		Result = Render_WingFXForward(pContext, *pBatch);

		m_pPendingWingFXBatch = nullptr;

		CGameInstance::Get().Reset_DefaultShader(RENDERGROUP::BLEND);
	}

	CGameInstance::Get().Remove_Instance(GetHandle());
	
	const auto& pModel = m_pComOutlineModelInstance->GetModel();
	if (pModel || pModel->GetAnimations().empty())
	
	m_pComOutlineModelInstance->Set_CombinedBoneMatrices(m_pComModelInstance->Get_CombinedBoneMatrices());

	CGameInstance::Get().Add_Instance(m_pComOutlineModelInstance, m_pModelAnimator, *GetTransform().GetCombinedWorldMatrix());

	m_bWingFXQueued = false;

	return Result;
}
HRESULT CEnderDragon::Render_WingFXForward(ID3D11DeviceContext* pContext, const E::MODEL_INSTANCE_BATCH& Batch) {
	if (m_bHide) return S_OK;
	SPtr<CResModel> pModel{};
	uint32_t iInstanceCount = 0;

	HRESULT hr = Prepare_DragonInstancing(pContext, Batch, pModel, iInstanceCount);
	if (FAILED(hr))	return hr;
	if (S_FALSE == hr)	return S_OK;

	ComPtr<ID3D11RasterizerState> pPrevRS;
	ComPtr<ID3D11DepthStencilState> pPrevDSS;
	UINT iPrevStencilRef = 0;
	
	pContext->RSGetState(pPrevRS.GetAddressOf());
	pContext->OMGetDepthStencilState(pPrevDSS.GetAddressOf(), &iPrevStencilRef);

	pContext->PSSetShader(m_pResDragonWingFXPixelShader->GetPixelShader().Get(), nullptr, 0);

	pContext->RSSetState(m_pResWingFXRasterizer->GetRasterizerState().Get());
	pContext->OMSetDepthStencilState(m_pResWingFXDSS->GetDepthStencilState().Get(), 0);

	ID3D11ShaderResourceView* FXSRV[4] = {
		m_pMarbleNoiseTexture->GetSRV().Get(),
		m_pRiverNoiseTexture->GetSRV().Get(),
		m_pCausticNoiseTexture->GetSRV().Get(),
		m_pDetailNoiseTexture->GetSRV().Get()
	};

	pContext->PSSetShaderResources(5, 4, FXSRV);

	pContext->PSSetShaderResources(4, 1, m_pEtherealWingsTexture->GetSRV().GetAddressOf());
	for (uint32_t iMeshIndex = 0; iMeshIndex < pModel->Get_NumMeshes(); ++iMeshIndex) {
		auto& mesh = pModel->GetMeshes()[iMeshIndex];
		if (!mesh || mesh->Get_MaterialIndex() != 2)	continue;

		if (FAILED(Bind_SkinnedMeshConstantBuffer(pContext, pModel, mesh, iMeshIndex))) return E_FAIL;
		
		ID3D11Buffer* pVB = mesh->GetVertexBuffer().Get();
		const UINT iStride = mesh->GetVertexStride();
		const UINT iOffset = 0;
		pContext->IASetVertexBuffers(0, 1, &pVB, &iStride, &iOffset);
		pContext->IASetIndexBuffer(mesh->GetIndexBuffer().Get(), mesh->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(mesh->GetPrimitiveType());

		pContext->DrawIndexedInstanced(mesh->GetNumIndices(), iInstanceCount, 0, 0, 0);
	}

	pContext->RSSetState(pPrevRS.Get());

	pContext->OMSetDepthStencilState(pPrevDSS.Get(), iPrevStencilRef);

	ID3D11ShaderResourceView* pNullVSSRV[2]{};
	pContext->VSSetShaderResources(7, 2, pNullVSSRV);

	ID3D11ShaderResourceView* pNullPSSRV[5]{};
	pContext->PSSetShaderResources(4, 5, pNullPSSRV);

	return S_OK;
}
HRESULT CEnderDragon::Prepare_DragonInstancing(ID3D11DeviceContext* pContext, const E::MODEL_INSTANCE_BATCH& Batch, SPtr<E::CResModel>& pOutModel, uint32_t& iOutInstanceCount) {
	if (!pContext || !m_pResVertexCPUSkinningInstancedShader || !m_pResSkinMeshCBuffer)	return E_FAIL;

	iOutInstanceCount = static_cast<uint32_t>(Batch.Instances.size());
	if (0 == iOutInstanceCount)	return S_FALSE;

	if (iOutInstanceCount > 512 || Batch.CombinedBoneMatrices.size() != iOutInstanceCount)	return E_FAIL;

	pContext->IASetInputLayout(m_pResVertexCPUSkinningInstancedShader->GetInputLayout().Get());
	pContext->VSSetShader(m_pResVertexCPUSkinningInstancedShader->GetVertexShader().Get(), nullptr, 0);

	if (FAILED(Update_InstanceBuffer(pContext, Batch.Instances)))	return E_FAIL;

	pOutModel = CGameInstance::Get().GetResourceFirst<E::CResModel>(Batch.Key.modelGroup, Batch.Key.modelTag);
	if (nullptr == pOutModel)	return E_FAIL;

	auto pCPUBonePaletteBuffer = CGameInstance::Get().GetResourceFirst<E::CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_CPU_BONEMATRIX");
	if (nullptr == pCPUBonePaletteBuffer) return E_FAIL;

	constexpr uint32_t MAX_BONES_PER_INSTANCE = 512;

	_float4x4 IdentityMatrix{};
	XMStoreFloat4x4(&IdentityMatrix, XMMatrixIdentity());

	std::vector<_float4x4> CombinedPalette(static_cast<size_t>(iOutInstanceCount) * MAX_BONES_PER_INSTANCE, IdentityMatrix);

	for (uint32_t iInstanceIndex = 0; iInstanceIndex < iOutInstanceCount; ++iInstanceIndex) {
		const auto& CombinedMatrices = Batch.CombinedBoneMatrices[iInstanceIndex];

		if (CombinedMatrices.empty() || CombinedMatrices.size() > MAX_BONES_PER_INSTANCE)	return E_FAIL;

		for (uint32_t iBoneIndex = 0; iBoneIndex < static_cast<uint32_t>(CombinedMatrices.size()); ++iBoneIndex) {
			const size_t iDestinationIndex = static_cast<size_t>(iInstanceIndex) * MAX_BONES_PER_INSTANCE + iBoneIndex;
			XMStoreFloat4x4(&CombinedPalette[iDestinationIndex], XMMatrixTranspose(XMLoadFloat4x4(&CombinedMatrices[iBoneIndex])));
		}
	}

	ID3D11ShaderResourceView* pNullSRV = nullptr;
	pContext->VSSetShaderResources(7, 1, &pNullSRV);

	if (FAILED(pCPUBonePaletteBuffer->UpdateData(CombinedPalette.data(), static_cast<uint32_t>(CombinedPalette.size() * sizeof(_float4x4)))))	return E_FAIL;

	if (FAILED(Bind_InstanceBuffer(pContext))) return E_FAIL;

	ID3D11ShaderResourceView* pCPUBonePaletteSRV = pCPUBonePaletteBuffer->GetSRV().Get();
	ID3D11ShaderResourceView* pSkinBoneSRV = pOutModel->Get_GPUSkinBoneSRV();
	if (nullptr == pCPUBonePaletteSRV || nullptr == pSkinBoneSRV)	return E_FAIL;

	pContext->VSSetShaderResources(7, 1, &pCPUBonePaletteSRV);
	pContext->VSSetShaderResources(8, 1, &pSkinBoneSRV);

	return S_OK;
}
HRESULT CEnderDragon::Bind_SkinnedMeshConstantBuffer(ID3D11DeviceContext* pContext, SPtr<E::CResModel>& pModel, SPtr<CResModelMesh>& pMesh, uint32_t iMeshIndex) {
	const auto& skinRange = pModel->Get_GPUMeshSkinRange(iMeshIndex);
	if (skinRange.iSkinBoneCount == 0) return E_FAIL;

	auto SkinningBuffer = m_pResSkinMeshCBuffer->GetCBuffer().Get();
	D3D11_MAPPED_SUBRESOURCE MRES{};
	if (FAILED(pContext->Map(SkinningBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))	return E_FAIL;

	GPU_SKIN_MESH_CONSTANTS SkinningConstants{};

	SkinningConstants.iSkinBoneOffset	 = skinRange.iSkinBoneOffset;
	SkinningConstants.iVertexCount		 = pMesh->GetNumVertices();
	SkinningConstants.iSkinBoneCount	 = skinRange.iSkinBoneCount;
	SkinningConstants.iBonePaletteStride = 512;

	memcpy(MRES.pData, &SkinningConstants, sizeof(GPU_SKIN_MESH_CONSTANTS));
	pContext->Unmap(SkinningBuffer, 0);

	pContext->VSSetConstantBuffers(5, 1, &SkinningBuffer);
	
	return S_OK;
}
/*---------------------------------*/
void CEnderDragon::Set_StateFinished(_bool bFinished)
{
	//스테이트가 완료된 판정에 대해서 다시 초기회
	auto pBB = Get_BlackBoard();
	if (nullptr == pBB) return;

	pBB->Set_Value(EDG_KEY::BSTATE_FINISHED, bFinished);
}
_bool CEnderDragon::Is_StateFinished()
{
	//스테이트가 끝났는지 확인
	auto pBB = Get_BlackBoard();
	if (nullptr == pBB) return false;

	auto pbFinished = pBB->Get_Value<_bool>(EDG_KEY::BSTATE_FINISHED);
	if (nullptr == pbFinished) return false;
	
	return *pbFinished;
}
_string CEnderDragon::Get_SkillName(ATTMON SkillNode)
{
	auto pValue = m_MonSkillLists.find(SkillNode);

	if (pValue == m_MonSkillLists.end())
		return "";

	if (pValue->second >= ETOUI(DRAGON_SKILL::END))
		return "";

	return MagicEnumToStringView(static_cast<DRAGON_SKILL>(pValue->second)).data();
}

_bool CEnderDragon::Check_Table(PLAYER_SKILL_TYPE eType)
{
	if (eType == PLAYER_SKILL_TYPE::END || eType == PLAYER_SKILL_TYPE::DEFAULT)
		return false;

	Damaged(eType);
	if (eType == PLAYER_SKILL_TYPE::ATTACK)
	{
		const auto hUIController = GET_SINGLE(UIManager)->GetUIController();
		if (hUIController.has_value())
		{
			if (auto* pUIController = CGameInstance::Get().GetGameObjectByHandleT<CUIController>(*hUIController))
			{
				pUIController->AddFinisher(2.f);
			}
		}
	}

	if (true == BreakSkillType(eType) && false == m_bIsBreak)
	{
		m_pFsm->Request_State(MON_STATE::HIT);
		m_bIsBreak = true;

		m_PendingMonTable.eAttType = m_eAttType;
		m_PendingMonTable.eHitType = PLAYER_SKILL_TYPE::DESTORY;
	
		m_bPending = true;
	}
	
	return true;

}
void CEnderDragon::Check_Phase()
{
	if (m_pFsm->GetCurState() != MON_STATE::COMBAT)
		return;
	auto pBB = Get_BlackBoard();
	if (nullptr == pBB) return;

	auto pSrc = Get_Target();
	if (nullptr == pSrc) return;

	_float3 DestPos = pSrc->GetTransform().GetPosition();
	_float3 SrcPos = GetTransform().GetPosition();

	_float fDis = XMVectorGetX(XMVector3Length(XMLoadFloat3(&DestPos) - XMLoadFloat3(&SrcPos)));

	auto pFinished = pBB->Get_Value<_bool>(EDG_KEY::BSTATE_FINISHED);
	if (m_iHp <= 0.f)
	{
		m_pFsm->Request_State(MON_STATE::DEAD);
		return;
	}
	if (nullptr == pFinished) return;

	if (false == m_bPhaseLock[ETOUI(DRAGON_PHASE::PHASE2)] && m_iHp <= m_iMaxHp - m_iMaxHp / 8.f)
	{
		//피 조금 까이고 도망
		m_bPhaseLock[ETOUI(DRAGON_PHASE::PHASE2)] = true;
		m_pFsm->Request_State(MON_STATE::PHASE_CHANGE);

		pBB->Set_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE,DRAGON_PHASE::PHASE2);
		return;
	}
	else if (true == m_bPhaseLock[ETOUI(DRAGON_PHASE::PHASE2)] && false == m_bPhaseLock[ETOUI(DRAGON_PHASE::PHASE3)] && fDis <= 40.f)
	{
		//도망간 후 파이어볼 잠깐 쏘다 거리 가까워지면 다시 run
		m_bPhaseLock[ETOUI(DRAGON_PHASE::PHASE3)] = true;
		m_pFsm->Request_State(MON_STATE::PHASE_CHANGE);

		pBB->Set_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE, DRAGON_PHASE::PHASE3);
		return;
	}
	else if (true == m_bPhaseLock[ETOUI(DRAGON_PHASE::PHASE3)] && false == m_bPhaseLock[ETOUI(DRAGON_PHASE::PHASE4)] && m_iHp <= m_iMaxHp / 2.f)
	{
		//대충 날다 두드려 맞고 도망
		m_bPhaseLock[ETOUI(DRAGON_PHASE::PHASE4)] = true;
		m_pFsm->Request_State(MON_STATE::PHASE_CHANGE);

		pBB->Set_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE, DRAGON_PHASE::PHASE4);
		return;
	}
	else if (true == m_bPhaseLock[ETOUI(DRAGON_PHASE::PHASE4)] &&false == m_bPhaseLock[ETOUI(DRAGON_PHASE::PHASE5)] && m_iHp <= m_iMaxHp / 3.f)
	{
		//대충 땅바닥 진입전 마지막 비행
		m_bPhaseLock[ETOUI(DRAGON_PHASE::PHASE5)] = true;
		m_pFsm->Request_State(MON_STATE::PHASE_CHANGE);

		pBB->Set_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE, DRAGON_PHASE::PHASE5);
		return;
	}
}
void CEnderDragon::Set_AttTable(ATTMON eType, _float2 fSkillRatio)
{
	if (eType == ATTMON::END)
		return;

	auto pbEffect = Get_BlackBoard()->Get_Value<_bool>(EDG_KEY::EDGEFFECT);
	uint32_t iSkillNum = Find_SkillNum(eType);
	if (iSkillNum == UINT_MAX || iSkillNum >= ETOUI(DRAGON_SKILL::END))
		return;

	EDG_ACSKT_DESC ACTable{};
	ACTable.SkillName = m_EffectNames[iSkillNum];
	ACTable.fLifeTime = fSkillRatio.y;
	ACTable.eType = static_cast<DRAGON_SKILL>(iSkillNum);
	if (*pbEffect)
	{
		CGameInstance::Get().PlayEffect(ACTable.SkillName, *GetTransform().GetWorldMatrix(), _vector{});
		Get_BlackBoard()->Set_Value<_bool>(EDG_KEY::EDGEFFECT, false);
	}
	else if (true == m_SkillHandle[iSkillNum].bPool)
	{
		auto pSkill = CGameInstance::Get().GetGameObjectByHandleT<CDragonSkill>(m_SkillHandle[iSkillNum].handle);
		if (nullptr == pSkill)
			return;
		pSkill->Active(ACTable);
	}
	else
	{
		EDG_SKILL_INFO SkillInfo = m_SkillHandle[iSkillNum];
		CDragonSkill::EDG_SKILL_DESC SkillDesc{};
		SkillDesc.hOwner = GetHandle();
		SkillDesc.iBoneIndex = SkillInfo.iBoneIndex;
		SkillDesc.iOffsetBoneIndex = SkillInfo.iOffsetBoneIndex;
		SkillDesc.eType = SkillInfo.eType;
		auto SkillHandle = CGameInstance::Get().AddGameObjectToLayer(SkillInfo.LevelTag,
			SkillInfo.ProtoTag, SkillInfo.NameTag, &SkillDesc);
		if (!SkillHandle) return;

		auto pSkill = CGameInstance::Get().GetGameObjectByHandleT<CDragonSkill>(SkillHandle.value());
		if (nullptr == pSkill)
			return;
		pSkill->Active(ACTable);
	}
	m_CurEffectName.clear();
	m_eAttType = ATTMON::END;
	m_eLastSkillTable = m_eAttType = eType;

}
void CEnderDragon::Flag_Check(_float fTimeDelta)
{
	//드래곤은 개별로 제어해서 체크해야겠네
	//사실 필요 없을지도~
	//플래그 왜만든거지
	//아아..
}
void CEnderDragon::Update_BBToFsm()
{
	auto pBB = Get_BlackBoard();

	if (nullptr == pBB)
		return;

	pBB->Set_Value(EDG_KEY::STATE, m_pFsm->GetCurState());
}
_bool CEnderDragon::BreakSkillType(PLAYER_SKILL_TYPE eType)
{
	uint32_t iSkillNumber = Find_SkillNum(m_eAttType);
	//파훼 됨?
	switch (eType)
	{
	case PLAYER_SKILL_TYPE::ACCIO:
		//if (ETOUI(DRAGON_SKILL::FIREBALL) == iSkillNumber)
		//	return true;
		break;
	case PLAYER_SKILL_TYPE::DEPULSO:
		break;

	case PLAYER_SKILL_TYPE::DESCENDO:
		break;

	case PLAYER_SKILL_TYPE::ACIENT_LIGHTNING:
		return true;
		break;
	case PLAYER_SKILL_TYPE::DESTORY:
		return true;
		break;
	case PLAYER_SKILL_TYPE::ABRA:
		return true;
		break;
	}
	return false;
}
void CEnderDragon::Phase_Debug()
{
	uint32_t i = 0;
	for (auto& iter : m_DebugPoint)
	{
		auto pDbgLineRender = CGameInstance::Get().GetDbgLineRender();

		const auto vPreviousColor = pDbgLineRender->GetColor();
		const auto ePreviousDepthMode = pDbgLineRender->GetDepthMode();
		pDbgLineRender->SetColor({ 0.f, 1.f, 1.f, 1.f });
		pDbgLineRender->SetDepthTest(true);
		pDbgLineRender->AddSphere(1.2f, XMMatrixTranslation(iter.x, iter.y, iter.z));
		pDbgLineRender->SetColor(vPreviousColor);
		pDbgLineRender->SetDepthMode(ePreviousDepthMode);
		Picking(iter, 0x44524750 + i++);
	}
	auto pBB = Get_BlackBoard();
	if (nullptr == pBB) return;
	//if (CGameInstance::Get().KeyPressing(DIK_LSHIFT) && CGameInstance::Get().KeyDown(DIK_Q))
	//{
	//	m_pFsm->Request_State(MON_STATE::PHASE_CHANGE);
	//	pBB->Set_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE, DRAGON_PHASE::PHASE1);
	//
	//}
	//else if (CGameInstance::Get().KeyPressing(DIK_LSHIFT) && CGameInstance::Get().KeyDown(DIK_W))
	//{
	//	m_pFsm->Request_State(MON_STATE::PHASE_CHANGE);
	//	pBB->Set_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE, DRAGON_PHASE::PHASE2);
	//}
	//else if (CGameInstance::Get().KeyPressing(DIK_LSHIFT) && CGameInstance::Get().KeyDown(DIK_E))
	//{
	//	m_pFsm->Request_State(MON_STATE::PHASE_CHANGE);
	//	pBB->Set_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE, DRAGON_PHASE::PHASE3);
	//}
	//else if (CGameInstance::Get().KeyPressing(DIK_LSHIFT) && CGameInstance::Get().KeyDown(DIK_R))
	//{
	//	m_pFsm->Request_State(MON_STATE::PHASE_CHANGE);
	//	pBB->Set_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE, DRAGON_PHASE::PHASE4);
	//}
	//else if (CGameInstance::Get().KeyPressing(DIK_LSHIFT) && CGameInstance::Get().KeyDown(DIK_T))
	//{
	//	m_pFsm->Request_State(MON_STATE::PHASE_CHANGE);
	//	pBB->Set_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE, DRAGON_PHASE::PHASE4);
	//}
}
void CEnderDragon::Picking(_float3& vPos, uint32_t iID)
{
	auto pCamera =CGameInstance::Get().GetActiveCamera();

	ImGuiViewport* pViewport =ImGui::GetMainViewport();

	if (!pCamera || !pViewport)
		return;

	_float4x4 View{};
	_float4x4 Projection{};
	_float4x4 World{};

	XMStoreFloat4x4(&View,	pCamera->GetView());
	XMStoreFloat4x4(&Projection,pCamera->GetProj());

	XMStoreFloat4x4(&World,XMMatrixTranslation(	vPos.x,	vPos.y,vPos.z));

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList(pViewport));

	ImGuizmo::SetRect(pViewport->Pos.x,pViewport->Pos.y,pViewport->Size.x,pViewport->Size.y);;
	ImGuizmo::SetID(iID);
	if (!ImGuizmo::Manipulate(	&View._11,&Projection._11,ImGuizmo::TRANSLATE,ImGuizmo::WORLD,&World._11))
		return;
	
	vPos ={ World._41, World._42, World._43};

	return ;
}
void CEnderDragon::InitializeEffects()
{

	{
		auto a = CGameInstance::Get().GetParticle("RanrokTrail1", "RanrokTrail1");
		static_cast<CTrail_CPU*>(a)->SetColor(_float4(182 / 255.f, 1.f, 241 / 255.f, 255 / 255.f));
		static_cast<CTrail_CPU*>(a)->SetEmissive(_float4(255 / 255.f, 45 / 255.f, 45 / 255.f, 15.f));

	}
	{
		auto a = CGameInstance::Get().GetParticle("RanrokTrail2", "RanrokTrail2");
		static_cast<CTrail_CPU*>(a)->SetColor(_float4(182 / 255.f, 1.f, 241 / 255.f, 255 / 255.f));
		static_cast<CTrail_CPU*>(a)->SetEmissive(_float4(255 / 255.f, 45 / 255.f, 45 / 255.f, 15.f));

	}
	{
		auto a = CGameInstance::Get().GetParticle("RanrokTrail3", "RanrokTrail3");
		static_cast<CTrail_CPU*>(a)->SetColor(_float4(182 / 255.f, 1.f, 241 / 255.f, 255 / 255.f));
		static_cast<CTrail_CPU*>(a)->SetEmissive(_float4(255 / 255.f, 45 / 255.f, 45 / 255.f, 15.f));

	}
	{
		auto a = CGameInstance::Get().GetParticle("RanrokTrail4", "RanrokTrail4");
		static_cast<CTrail_CPU*>(a)->SetColor(_float4(182 / 255.f, 1.f, 241 / 255.f, 255 / 255.f));
		static_cast<CTrail_CPU*>(a)->SetEmissive(_float4(255 / 255.f, 45 / 255.f, 45 / 255.f, 15.f));
	}
	{
		auto a = CGameInstance::Get().GetParticle("RanrokTrail5", "RanrokTrail5");
		static_cast<CTrail_CPU*>(a)->SetColor(_float4(182 / 255.f, 1.f, 241 / 255.f, 255 / 255.f));
		static_cast<CTrail_CPU*>(a)->SetEmissive(_float4(255 / 255.f, 0.f, 0 / 255.f, 15.f));
	}
	{
		auto a = CGameInstance::Get().GetParticle("SpitTrail", "SpitTrail");
		static_cast<CTrail_CPU*>(a)->SetColor(_float4(1.0f,0.35f,0.35f,1.f));
		static_cast<CTrail_CPU*>(a)->SetEmissive(_float4(1.0f,0.28f,0.28f,100.f));
	}
	{
		auto a = CGameInstance::Get().GetParticle("DragonProj2Trail", "DragonProj2Trail");
		static_cast<CTrail_CPU*>(a)->SetColor(_float4(0.0f, 0.0f, 0.0f,1.f));
		static_cast<CTrail_CPU*>(a)->SetEmissive(_float4(1.0f, 0.28f, 0.28f, 10.f));

	}
}
E::UPtr<CEnderDragon> CEnderDragon::Create()
{
	auto pInstance = E::ToUPtr(new CEnderDragon{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CEnderDragon");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CEnderDragon::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CEnderDragon{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CEnderDragon");
		return nullptr;
	}

	return pInstance;
}
