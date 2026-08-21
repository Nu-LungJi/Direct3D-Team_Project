#include "pch.h"
#include "Player.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "Level_Defines.h"
#include "ComModelInstance.h"
#include "ComAnimator.h"
#include "Resources.h"
#include "GameInstance.h"
#include "ComSocket.h"
#include "DebugPlayer.h"
#include "Collider.h"
#include "CollBox.h"
#include "ComPxRigidBody.h"
#include "ComPxBoxCollider.h"
#include "ComPxSphereCollider.h"
#include "ComPxCapsuleCollider.h"
#include "ComPxCharacterController.h"
#include "ComCharacterMoveIntent.h"
#include "ComCharacterMotor.h"
#include "PlayerRagdollController.h"
#include "Player_BombardaController.h"
#include "Player_ConfringoController.h"
#include "Player_AvadaKedavraController.h"
#include "Player_Stupefy_Bullet.h"
#include "PlayerThirdPersonCamera.h"
#include "DbgLineRender.h"
#include "Player_StateMachine.h"
#include "Player_Locomotion_State.h"
#include "Player_Fly_State.h"
#include "Player_Jump_State.h"
#include "Player_Roll_State.h"
#include "Player_Attack_State.h"
#include "Player_Hit_State.h"
#include "Player_Knockdown_State.h"
#include "PlayerAnimationRatioGuard.h"
#include "Player_DashSkill_State.h"
#include "Player_AcientAttack_State.h"
#include "Player_AccioSkill_State.h"
#include "Player_DepulsoSkill_State.h"
#include "Player_DescendoSkill_State.h"
#include "Player_BombardaSkill_State.h"
#include "Player_TransformationSkill_State.h"
#include "Player_ConfringoSkill_State.h"
#include "Player_AvadaKedavraSkill_State.h"
#include "Player_ProtegoSkill_State.h"
#include "Player_StupefySkill_State.h"
#include "Player_LumosSkill_State.h"
#include "Player_RepairoSkill_State.h"
#include "Player_Potion_State.h"
#include "Monster.h"
#include "ComSound.h"
#include "ClientEvents.h"

#include "Player_RevelioSkill_State.h"
#include "Player_Magic_Bullet.h"
#include "Player_Weapon.h"
#include "Player_Broom.h"
#include "WiggenweldPotion.h"
#include "PropBarrel.h"
#include "Light.h"
#include "Trail_CPU.h"
#include "UIController.h"
#include "UIManager.h"
NS_USING(Client)

namespace
{
	_bool IsAncientThrowTargetInCameraView(
		const _float3& worldPosition,
		const _matrix& view,
		const _matrix& projection)
	{
		const _vector clip = XMVector4Transform(
			XMVectorSet(worldPosition.x, worldPosition.y, worldPosition.z, 1.f),
			view * projection);
		const _float w = XMVectorGetW(clip);
		if (w <= FLT_EPSILON)
			return false;

		const _float inverseW = 1.f / w;
		const _float ndcX = XMVectorGetX(clip) * inverseW;
		const _float ndcY = XMVectorGetY(clip) * inverseW;
		const _float ndcZ = XMVectorGetZ(clip) * inverseW;
		return std::abs(ndcX) <= 1.f && std::abs(ndcY) <= 1.f &&
			ndcZ >= 0.f && ndcZ <= 1.f;
	}
}



void CPlayer::UpdateGUI()
{
	
	__super::UpdateGUI();


	ImGui::Text(
		"Control Hold Time: %.3f",
		m_fControlHoldTime);

	ImGui::Text(
		"Dash Triggered: %s",
		m_bDashTriggered ? "True" : "False");

	ImGui::ProgressBar(
		std::clamp(
			m_fControlHoldTime / DASH_HOLD_TIME,
			0.f,
			1.f),
		ImVec2(-1.f, 0.f),
		"Dash Hold");

	
	ImGui::Text("HP : %d", m_iHp);

	ImGui::DragInt("HP : ",&m_iHp,0.1f,0,100000);
	ImGui::Text("RMB Wand Ready: %s (Anim Index: %d)",
		m_bDebugWandReadyPlaying ? "Playing" : "Stopped",
		m_iDebugWandReadyUpperAnim);
	if (m_pModelAnimator)
	{
		ImGui::Text("Upper Layer: %s / Weight: %.2f / Animator: %s",
			m_pModelAnimator->HasUpperAnimation() ? "Loaded" : "Empty",
			m_pModelAnimator->GetUpperLayerWeight(),
			m_pModelAnimator->GetPlay() ? "Playing" : "Paused");
	}
	if (ImGui::CollapsingHeader("[KMS] Lumos Attach Debug", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text("Lumos: %s", m_bLumosActive ? "ON" : "OFF");
		ImGui::DragFloat3(
			"Wand Local Offset",
			&m_vLumosLocalOffset.x,
			0.005f,
			-2.f,
			2.f,
			"%.3f");
		ImGui::TextDisabled("X: right / Y: up / Z: wand forward");
		ImGui::Text(
			"World: %.3f, %.3f, %.3f",
			m_vLumosDebugWorldPosition.x,
			m_vLumosDebugWorldPosition.y,
			m_vLumosDebugWorldPosition.z);
		if (ImGui::Button("Reset Lumos Offset"))
			m_vLumosLocalOffset = {};
	}
	if (ImGui::Button("Open Player in Animation Editor"))
		CGameInstance::Get().SetAnimationEditorTarget(GetHandle());
	ImGui::SameLine();
	ImGui::TextDisabled("SampleClient-compatible");

	if (m_pConfringoController)
		m_pConfringoController->UpdateGUI();

	UpdateStupefyDebugGUI();
	if (m_pRagdollController)
		m_pRagdollController->UpdateGUI();




}

CPlayer::CPlayer()
	: CAnimationObject{}
{
}

CPlayer::CPlayer(const CPlayer& Prototype)
	: CAnimationObject{ Prototype }
	, m_pResPixelShader{ Prototype.m_pResPixelShader }
	, m_pResVertexCPUSkinningInstancedShader{
		Prototype.m_pResVertexCPUSkinningInstancedShader }
	, m_pResSkinMeshCBuffer{
		Prototype.m_pResSkinMeshCBuffer }
{
}

CPlayer::~CPlayer()
{
}


HRESULT CPlayer::InitializePrototype(void* pArg)
{

	m_pResVertexCPUSkinningInstancedShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelAnim_CPU_Skinning_Instanced");
	if (!m_pResVertexCPUSkinningInstancedShader || FAILED(m_pResVertexCPUSkinningInstancedShader->Load()))
	{
		return E_FAIL;
	}
	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelAnim");
	if (FAILED(m_pResPixelShader->Load()))
	{
		return E_FAIL;
	}
	m_pResSkinMeshCBuffer = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_GPU_SKIN_MESH");
	if (!m_pResSkinMeshCBuffer)
	{
		return E_FAIL;
	}



	return S_OK;
}

HRESULT CPlayer::Initialize(void* pArg)
{
	auto		pDesc = static_cast<DESC*>(pArg);
	if (!pDesc)
		return E_FAIL;

	if (FAILED(CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	}
	GetTransform().SetPosition(pDesc->vInitialPosition);
	GetTransform().SetRotationEuler(pDesc->vInitialRotation);
	GetTransform().Update();
	m_LevelTag = pDesc->LevelTag;
	m_vInitialPosition = pDesc->vInitialPosition;
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
		Desc.sGroupTag = pDesc->LevelTag;
		Desc.sResTag =	 "PLAYER_MODEL_RESROUCE";

		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ModelInstance", "ComCModelIntance", &Desc, &m_pComModelInstance)))
		{
			return E_FAIL;
		};

		m_iHurtBoxBoneIndex =
			m_pComModelInstance->GetModel()->Get_BoneIndex("Spine1");
		m_iLeftFootBoneIndex =
			m_pComModelInstance->GetModel()->Get_BoneIndex("LeftFoot");
		m_iRightFootBoneIndex =
			m_pComModelInstance->GetModel()->Get_BoneIndex("RightFoot");
	}

	{
		CComAnimator::DESC DescAnim{};
		DescAnim.sComTag = "ComCModelIntance";

		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_Animator", "ComCModelAnimator", &DescAnim, &m_pModelAnimator)))
		{
			return E_FAIL;
		};

		// TestModel은 생성 직후부터 CPU pose + VS skinning 경로를 사용한다.
		m_pModelAnimator->SetEvaluationMode(CComAnimator::EVALUATION_MODE::CPU_GPU);
		if (!m_pModelAnimator->Set_UpperBodyRootBone("RightArm", 1))
		{
			MSG_BOX("FAILED to Set UpperBodyRootBone");
		}

		const auto& animations = m_pComModelInstance->GetModel()->GetAnimations();
		constexpr _string_view debugWandReadyAnimation =
			"AN_ProfessorSharp_MasterRig_Hu_BM_Wand_Ready_RArmReplace_anm.bin";
		for (size_t i = 0; i < animations.size(); ++i)
		{
			if (animations[i] && animations[i]->GetAnimName() == debugWandReadyAnimation)
			{
				m_iDebugWandReadyUpperAnim = (int32_t)(i);
				break;
			}
		}

	}

	{
		CComPxRigidBody::DESC Desc{};
		Desc.eType = CComPxRigidBody::TYPE::KINEMATIC;
		Desc.vPosition = pDesc->vInitialPosition;
		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxRigidBody, "ComHitboxRigidbody", &Desc, &m_pComPxRigidBody)))
		{
			return E_FAIL;
		};
	}

	{
		CComPxSphereCollider::DESC Desc{};
		Desc.pComPxRigidBody = m_pComPxRigidBody;
		Desc.pResSphereGeo = CResPhysXSphereGeometry::CreateAndLoad({ .fRadius = 0.16f });
		Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});
		Desc.iShapeSubIndex = ETOUI(PLAYER_COLLISIONS::PLAYER_LEFT_FOOT);
		Desc.bIsTrigger = true;
		Desc.tFilter.iLayer = ETOUI(COLLISION_LAYER::SENSOR);
		Desc.tFilter.iQueryMask = 0;
		Desc.tFilter.iSimulationMask =
			ETOUI(COLLISION_LAYER::WORLD_STATIC) |
			ETOUI(COLLISION_LAYER::WORLD_DYNAMIC) |
			ETOUI(COLLISION_LAYER::MOVING_PLATFORM);
		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxSphereCollider, "ComPxLeftFootCollider", &Desc, &m_pComPxLeftFootCollider)))
		{
			return E_FAIL;
		}
	}

	{
		CComPxSphereCollider::DESC Desc{};
		Desc.pComPxRigidBody = m_pComPxRigidBody;
		Desc.pResSphereGeo = CResPhysXSphereGeometry::CreateAndLoad({ .fRadius = 0.16f });
		Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});
		Desc.iShapeSubIndex = ETOUI(PLAYER_COLLISIONS::PLAYER_RIGHT_FOOT);
		Desc.bIsTrigger = true;
		Desc.tFilter.iLayer = ETOUI(COLLISION_LAYER::SENSOR);
		Desc.tFilter.iQueryMask = 0;
		Desc.tFilter.iSimulationMask =
			ETOUI(COLLISION_LAYER::WORLD_STATIC) |
			ETOUI(COLLISION_LAYER::WORLD_DYNAMIC) |
			ETOUI(COLLISION_LAYER::MOVING_PLATFORM);
		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxSphereCollider, "ComPxRightFootCollider", &Desc, &m_pComPxRightFootCollider)))
		{
			return E_FAIL;
		}
	}

	{
		CComPxBoxCollider::DESC Desc{};
		Desc.pComPxRigidBody = m_pComPxRigidBody;
		Desc.pResBoxGeo = CResPhysXBoxGeometry::CreateAndLoad({ .vHalfExtents = {2.f, 1.f, 1.f} });
		Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});
		Desc.iShapeSubIndex = ETOUI(PLAYER_COLLISIONS::PLAYER_SHAPE_HURTBOX);
		Desc.tFilter.iLayer = ETOUI(COLLISION_LAYER::PLAYER_HURTBOX);
		Desc.tFilter.iQueryMask = ETOUI(COLLISION_LAYER::ENEMY_PROJECTILE);
		Desc.tFilter.iSimulationMask = ETOUI(COLLISION_LAYER::ENEMY_PROJECTILE);
		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxBoxCollider, "ComPxBoxCollider", &Desc, &m_pComPxBoxCollider)))
		{
			return E_FAIL;
		};
	}

	{
		CComPxCharacterController::DESC Desc{};
		Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad(CResPhysXMaterial::DESC{});
		Desc.fHeight = pDesc->fCCTHeight;
		Desc.fRadius = pDesc->fCCTRadius;
		Desc.fStepOffset = pDesc->fCCTStepOffset;
		Desc.tFilter = pDesc->tFilter;
		Desc.vPosition = {
			pDesc->vInitialPosition.x + pDesc->vCCTCenterOffset.x,
			pDesc->vInitialPosition.y + pDesc->vCCTCenterOffset.y,
			pDesc->vInitialPosition.z + pDesc->vCCTCenterOffset.z };
		Desc.iShapeSubIndex = ETOUI(PLAYER_COLLISIONS::CCT_CAPSULE);
		// 오르막은 CCT 기본 경사 제한을 사용한다.
		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxCharacterController,"ComPxCharacterController", &Desc, &m_pComCharacterController)))
		{
			return E_FAIL;
		};
	}

	{
		CComCharacterMoveIntent::DESC Desc{};
		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::PERMANENT,ES_EngineProtoComponent::Prototype_Component_ComCharacterMoveIntent,"ComCharacterMoveIntent", &Desc, &m_pComMoveIntent)))
		{
			return E_FAIL;
		}
	}

	{
		CComCharacterMotor::DESC Desc{};
		Desc.pMoveIntent = m_pComMoveIntent;
		Desc.pCharacterController = m_pComCharacterController;
		// 기존 -9.81/7 조합은 체공 시간이 길어 달에서 뛰는 느낌이 강했다.
		// 강한 중력은 유지하고 초속도만 높여 체공감은 억제하면서 점프 높이를 확보한다.
		Desc.fGravity = -16.f;
		Desc.fJumpVelocity = 9.f;
		Desc.vControllerCenterOffset = pDesc->vCCTCenterOffset;
		Desc.bUseGravity = true;
		Desc.bSyncTransform = true;
		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::PERMANENT,ES_EngineProtoComponent::Prototype_Component_ComCharacterMotor,"ComCharacterMotor", &Desc, &m_pComCharacterMotor)))
		{
			return E_FAIL;
		}
	}

	{
		CPlayer_StateMachine::DESC Desc{};
		if (FAILED(AddComponentFromProto(
			"PLAYER_STATEMACHINE",
			"Prototype_Component_Player_StateMachine",
			"Player_StateMachine",
			&Desc,
			&m_pStateMachine)) ||
			!m_pStateMachine)
		{
			return E_FAIL;
		}

		if (!m_pStateMachine->AddPlayerState(
			PLAYER_STATE::LOCOMOTION,
			CPlayer_Locomotion_State::Create()))
		{
			return E_FAIL;
		}
		if (!m_pStateMachine->AddPlayerState(
			PLAYER_STATE::ROLL,
			CPlayer_Roll_State::Create()))
		{
			return E_FAIL;
		}
		if (!m_pStateMachine->AddPlayerState(
			PLAYER_STATE::JUMP,
			CPlayer_Jump_State::Create()))
		{
			return E_FAIL;
		}
		if (!m_pStateMachine->AddPlayerState(
			PLAYER_STATE::ATTACK,
			CPlayer_Attack_State::Create()))
		{
			return E_FAIL;
		}
		if (!m_pStateMachine->AddPlayerState(
			PLAYER_STATE::HIT,
			CPlayer_Hit_State::Create()))
		{
			return E_FAIL;
		}
		if (!m_pStateMachine->AddPlayerState(
			PLAYER_STATE::KNOCKDOWN,
			CPlayer_Knockdown_State::Create()))
		{
			return E_FAIL;
		}
		if (!m_pStateMachine->AddPlayerState(
			PLAYER_STATE::DASH_SKILL,
			CPlayer_DashSkill_State::Create()))
		{
			return E_FAIL;
		}
		if (!m_pStateMachine->AddPlayerState(
			PLAYER_STATE::ACIENTATTACK_SKILL,
			CPlayer_AcientAttack_State::Create()))
		{
			return E_FAIL;
		}
		if (!m_pStateMachine->AddPlayerState(
			PLAYER_STATE::ACCIO_SKILL,
			CPlayer_AccioSkill_State::Create()))
		{
			return E_FAIL;
		}
		if (!m_pStateMachine->AddPlayerState(
			PLAYER_STATE::DEPULSO_SKILL,
			CPlayer_DepulsoSkill_State::Create()))
		{
			return E_FAIL;
		}
		if (!m_pStateMachine->AddPlayerState(
			PLAYER_STATE::DESCENDO_SKILL,
			CPlayer_DescendoSkill_State::Create()))
		{
			return E_FAIL;
		}
		if (!m_pStateMachine->AddPlayerState(
			PLAYER_STATE::BOMBARDA_SKILL,
			CPlayer_BombardaSkill_State::Create()))
		{
			return E_FAIL;
		}
		if (!m_pStateMachine->AddPlayerState(
			PLAYER_STATE::TRANSFORMATION_SKILL,
			CPlayer_TransformationSkill_State::Create()))
		{
			return E_FAIL;
		}
		if (!m_pStateMachine->AddPlayerState(
			PLAYER_STATE::CONFRINGO_SKILL,
			CPlayer_ConfringoSkill_State::Create()))
		{
			return E_FAIL;
		}
		if (!m_pStateMachine->AddPlayerState(
			PLAYER_STATE::AVADA_KEDAVRA_SKILL,
			CPlayer_AvadaKedavraSkill_State::Create()))
		{
			return E_FAIL;
		}
		if (!m_pStateMachine->AddPlayerState(
			PLAYER_STATE::PROTEGO_SKILL,
			CPlayer_ProtegoSkill_State::Create()))
		{
			return E_FAIL;
		}
		if (!m_pStateMachine->AddPlayerState(
			PLAYER_STATE::STUPEFY_SKILL,
			CPlayer_StupefySkill_State::Create()))
		{
			return E_FAIL;
		}
		if (!m_pStateMachine->AddPlayerState(
			PLAYER_STATE::LUMOS_SKILL,
			CPlayer_LumosSkill_State::Create()))
		{
			return E_FAIL;
		}
		if (!m_pStateMachine->AddPlayerState(
			PLAYER_STATE::REVELIO_SKILL,
			CPlayer_RevelioSkill_State::Create()))
		{
			return E_FAIL;
		}
		if (!m_pStateMachine->AddPlayerState(
			PLAYER_STATE::REPAIRO_SKILL,
			CPlayer_RepairoSkill_State::Create()))
		{
			return E_FAIL;
		}
		if (!m_pStateMachine->AddPlayerState(
			PLAYER_STATE::POTION,
			CPlayer_Potion_State::Create()))
		{
			return E_FAIL;
		}
		if (!m_pStateMachine->AddPlayerState(
			PLAYER_STATE::FLY,
			CPlayer_Fly_State::Create()))
		{
			return E_FAIL;
		}

		if (!m_pStateMachine->SetInitialState(PLAYER_STATE::LOCOMOTION))
		{
			return E_FAIL;
		}
	}//Spine1

	// 초기 State가 재생할 애니메이션을 선택한 뒤 0초 포즈를 만든다.
	// 이 포즈로 키네마틱 랙돌을 첫 PhysX 스텝 전부터 본에 맞춘다.
	if (FAILED(m_pModelAnimator->Update(0.f)) ||
		FAILED(InitializeRagdoll()))
	{
		return E_FAIL;
	}

	m_pComMoveIntent->RequestWarp(pDesc->vInitialPosition);

	CPlayer_Weapon::WEAPON_DESC WeaponDesc{};
	WeaponDesc.sObjectTag = "Weapon";
	WeaponDesc.LevelTag = pDesc->LevelTag.GetDbgStr();
	WeaponDesc.WeaponName = "PLAYER_WEAPON_SKELETON_RESOURCE";
	WeaponDesc.iBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("RightHandWandSocket");
	WeaponDesc.iSpawnBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("WandSocketTip");
	WeaponDesc.ParentHandle = GetHandle();

	
	auto Weapon = E::CGameInstance::Get().AddGameObjectToLayer(pDesc->LevelTag, PROTO_GAMEOBJECT::Prototype_GameObject_PlayerWeapon, "Weapon", &WeaponDesc);
	if (!Weapon.has_value())
	{
		MSG_BOX("Create Failed Weapon");
		return E_FAIL;
	}

	m_Partes[ETOUI(PARTES::WEAPON)] = Weapon.value();

	CPlayer_Broom::BROOM_DESC BroomDesc{};
	BroomDesc.sObjectTag = "Broom";
	BroomDesc.sLevelTag = pDesc->LevelTag.GetDbgStr();
	BroomDesc.sResourceTag = "PLAYER_BROOM_RESOURCE";
	BroomDesc.iSocketBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("SKT_BroomCollision");
	BroomDesc.hParent = GetHandle();
	// UEModel -> static FBX 변환 과정에서 센티미터 단위가 1/100로 축소되어
	// 기존 장비 모델 스케일(4배)에 원본 단위 복원(100배)을 함께 적용한다.
	BroomDesc.vScale = { 4.f, 4.f, 4.f };
	BroomDesc.bVisible = false;

	auto Broom = E::CGameInstance::Get().AddGameObjectToLayer(
		pDesc->LevelTag,
		PROTO_GAMEOBJECT::Prototype_GameObject_PlayerBroom,
		"Broom",
		&BroomDesc);
	if (!Broom.has_value())
	{
		MSG_BOX("Create Failed Broom");
		return E_FAIL;
	}
	m_Partes[ETOUI(PARTES::BROOM)] = Broom.value();

	{
		{
			auto a = CGameInstance::Get().GetParticle("PlayerAttackTrail_CPU", "PlayerAttackTrail_CPU");
			static_cast<CTrail_CPU*>(a)->SetBehaviorMode(CTrail_CPU::TRAIL_BEHAVIOR_MODE::LEGACY);
			static_cast<CTrail_CPU*>(a)->SetColor(_float4(1.f, 113/255.f, 113 / 255.f, 1.f));
			static_cast<CTrail_CPU*>(a)->SetEmissive(_float4(1.f, 44 / 255.f, 44 / 255.f, 5.f));
		}
		{
			auto a = CGameInstance::Get().GetParticle("PlayerDashTrail1_CPU", "PlayerDashTrail1_CPU");
			static_cast<CTrail_CPU*>(a)->SetColor(_float4(182 / 255.f, 1.f, 241 / 255.f, 140 / 255.f));
			static_cast<CTrail_CPU*>(a)->SetEmissive(_float4(182 / 255.f, 1.f, 241 / 255.f, 2.f));
		}
		{
			auto a = CGameInstance::Get().GetParticle("Repairo_Trail", "Repairo_Trail");
			static_cast<CTrail_CPU*>(a)->SetColor(_float4(255 / 255.f, 255 / 255.f, 40 / 255.f, 1.f));
			static_cast<CTrail_CPU*>(a)->SetEmissive(_float4(255 / 255.f, 255 / 255.f, 40 / 255.f, 4.f));
		}

	}

	{
		CComSound::DESC Desc{};

		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoComponent::Prototype_Component_ComSound,
			"Com_Sound",
			&Desc,
			&m_pComSound)))
		{
			return E_FAIL;
		}
	}

	m_hAutoTarget = CHandle{};
	if (FAILED(InitializeBombarda()))
		return E_FAIL;
	if (FAILED(InitializeConfringo()))
		return E_FAIL;
	if (FAILED(InitializeAvadaKedavra()))
		return E_FAIL;

	return S_OK;
}

#pragma region RAGDOLL
HRESULT CPlayer::InitializeRagdoll()
{
	// [LSY] 플레이어별 런타임 상태를 공유하지 않도록 Clone 초기화 단계에서 새로 생성한다.
	m_pRagdollController = CPlayerRagdollController::Create(*this);
	return m_pRagdollController ? S_OK : E_FAIL;
}

_bool CPlayer::RequestRagdollActivation(
	const _float3& vLinearVelocity, const _float3& vAngularVelocityRadians)
{
	if (!m_pRagdollController)
		return false;

	return m_pRagdollController->RequestActivation(vLinearVelocity, vAngularVelocityRadians);
}

_bool CPlayer::ResetRagdoll()
{
	if (!m_pRagdollController)
		return false;

	return m_pRagdollController->Reset();
}

_bool CPlayer::IsRagdollActive() const
{
	if (!m_pRagdollController)
		return false;

	return m_pRagdollController->IsActive();
}

_bool CPlayer::TryGetRagdollFollowPosition(_float3& OutPosition) const
{
	if (!m_pRagdollController)
		return false;

	return m_pRagdollController->TryGetFollowPosition(OutPosition);
}

_bool CPlayer::IsRagdollTransitioning() const
{
	if (!m_pRagdollController)
		return false;

	return m_pRagdollController->IsTransitioning();
}
#pragma endregion

#pragma region AVADA_KEDAVRA
HRESULT CPlayer::InitializeAvadaKedavra()
{
	// [LSY] 플레이어별 아바다 케다브라 런타임 연출 상태를 Clone 초기화에서 생성한다.
	m_pAvadaKedavraController =
		CPlayer_AvadaKedavraController::Create(*this);
	return m_pAvadaKedavraController ? S_OK : E_FAIL;
}

void CPlayer::StartAvadaKedavraCastEffect()
{
	if (m_pAvadaKedavraController)
		m_pAvadaKedavraController->StartCastEffect();
}

void CPlayer::StopAvadaKedavraCastEffect()
{
	if (m_pAvadaKedavraController)
		m_pAvadaKedavraController->StopCastEffect();
}

_bool CPlayer::ReleaseAvadaKedavraSpell()
{
	if (!m_pAvadaKedavraController)
		return false;

	return m_pAvadaKedavraController->ReleaseSpell();
}
#pragma endregion

#pragma region BOMBARDA
HRESULT CPlayer::InitializeBombarda()
{
	// [LSY] 플레이어별 봄바르다 런타임 상태를 Clone 초기화에서 생성한다.
	m_pBombardaController = CPlayer_BombardaController::Create(*this);
	return m_pBombardaController ? S_OK : E_FAIL;
}

void CPlayer::StartBombardaCastEffect()
{
	if (m_pBombardaController)
		m_pBombardaController->StartCastEffect();
}

void CPlayer::StopBombardaCastEffect()
{
	if (m_pBombardaController)
		m_pBombardaController->StopCastEffect();
}

_bool CPlayer::FireBombardaProjectile()
{
	if (!m_pBombardaController)
		return false;

	return m_pBombardaController->FireProjectile();
}
#pragma endregion

#pragma region CONFRINGO
HRESULT CPlayer::InitializeConfringo()
{
	// [LSY] 플레이어별 콘프링고 런타임 상태를 Clone 초기화에서 생성한다.
	m_pConfringoController = CPlayer_ConfringoController::Create(*this);
	return m_pConfringoController ? S_OK : E_FAIL;
}

void CPlayer::StartConfringoCastEffect()
{
	if (m_pConfringoController)
		m_pConfringoController->StartCastEffect();
}

void CPlayer::StopConfringoCastEffect()
{
	if (m_pConfringoController)
		m_pConfringoController->StopCastEffect();
}

_bool CPlayer::FireConfringoProjectile()
{
	if (!m_pConfringoController)
		return false;

	return m_pConfringoController->FireProjectile();
}
#pragma endregion


void CPlayer::PriorityUpdate(E::_float fTimeDelta)
{
	if (m_bProtegoActive)
	{
		m_fProtegoRemainTime = std::max(0.f, m_fProtegoRemainTime - fTimeDelta);
		if (m_fProtegoRemainTime <= 0.f)
		{
			m_bProtegoActive = false;
			if (m_iProtegoShieldEffectID != INVALID_EFFECT_INSTANCE_ID)
			{
				CGameInstance::Get().StopEffect(m_iProtegoShieldEffectID);
				m_iProtegoShieldEffectID = INVALID_EFFECT_INSTANCE_ID;
			}
		}
	}
	m_fParryCounterRemainTime = std::max(0.f, m_fParryCounterRemainTime - fTimeDelta);
	if (m_fParryCounterRemainTime <= 0.f)
		m_bStupefyCounterRequested = false;

	// [LSY] 랙돌 전환 중에는 입력과 상태 머신이 새로운 이동 명령을 만들지 않게 한다.
	if (m_pRagdollController)
	{
		if (m_pRagdollController->PrePriorityUpdate())
		{
			if (m_pModelAnimator && m_bDebugWandReadyPlaying)
			{
				m_pModelAnimator->Stop_UpperAnim(0.1f);
				m_bDebugWandReadyPlaying = false;
			}
			return;
		}
	}

	if (CGameInstance::Get().IsAnimationEditorTarget(GetHandle()))
	{
		if (m_pModelAnimator && m_bDebugWandReadyPlaying)
		{
			m_pModelAnimator->Stop_UpperAnim(0.1f);
			m_bDebugWandReadyPlaying = false;
		}
		m_bRawMoveInput = false;
		m_bSprintRequested = false;
		m_bWalkRequested = false;
		m_vRawMoveDirection = {};
		m_fCurrentMoveSpeed = 0.f;
		m_bRootMotionTranslationActive = false;
		m_bRootMotionRotationActive = false;
		if (m_pComMoveIntent)
			m_pComMoveIntent->ClearMoveIntent();
		return;
	}

	// 비행 상태는 아래에서 일반 입력 처리를 조기 종료하므로,
	// 탑승/하차 토글은 상태 머신 갱신보다 먼저 처리해야 한다.
	if (m_pStateMachine && CGameInstance::Get().KeyDown(DIK_O))
	{
		SetFlyRequested(!m_bFlyRequested);
	}

	if (m_pStateMachine)
		m_pStateMachine->PriorityUpdate(fTimeDelta);

	// 비행 중 이동 명령은 FlyState가 카메라 기준 3차원 방향으로 직접 만든다.
	// 아래 지상 입력 코드가 Y 성분을 0으로 덮어쓰면 상승/하강이 사라지므로 여기서 분리한다.
	if (m_pStateMachine &&
		m_pStateMachine->GetCurrentState() == PLAYER_STATE::FLY)
	{
		m_bRawMoveInput = false;
		m_bSprintRequested = false;
		m_bWalkRequested = false;
		m_vRawMoveDirection = {};
		return;
	}

	if (m_pModelAnimator && m_iDebugWandReadyUpperAnim >= 0)
	{
		constexpr _float debugUpperBodyFadeDuration = 0.2f;
		const _bool bWandReadyRequested =
			CGameInstance::Get().MousePressing(MOUSEKEYSTATE::RB) &&
			m_pStateMachine &&
			m_pStateMachine->GetCurrentState() == PLAYER_STATE::LOCOMOTION;
		if (bWandReadyRequested &&
			(!m_bDebugWandReadyPlaying ||
				!m_pModelAnimator->HasUpperAnimation()))
		{
			m_bDebugWandReadyPlaying = PlayUpperBodyAnimation(
				m_iDebugWandReadyUpperAnim,
				"RightArm",
				1,
				true,
				debugUpperBodyFadeDuration);
		}
		else if (!bWandReadyRequested && m_bDebugWandReadyPlaying)
		{
			m_pModelAnimator->Stop_UpperAnim(debugUpperBodyFadeDuration);
			m_bDebugWandReadyPlaying = false;
		}
	}

	if (m_pStateMachine &&
		m_pStateMachine->GetCurrentState() == PLAYER_STATE::ACIENTATTACK_SKILL)
	{
		m_bRawMoveInput = false;
		m_bSprintRequested = false;
		m_bWalkRequested = false;
		m_vRawMoveDirection = {};
		m_fCurrentMoveSpeed = 0.f;
		m_fControlHoldTime = 0.f;
		m_bDashTriggered = false;
		m_pComMoveIntent->ClearMoveIntent();
		return;
	}

	auto* pPlayerCamera = CGameInstance::Get().GetActiveCamera("PlayerCamera");
	if (!pPlayerCamera)
	{
		m_bRawMoveInput = false;
		m_bSprintRequested = false;
		m_bWalkRequested = false;
		m_vRawMoveDirection = {};
		m_pComMoveIntent->ClearMoveIntent();
		return;
	}


	// 실제 콘텐츠에서는 BT가 이 입력 코드 대신 이동 의도만 Locomotion에 전달한다.
	_float fForwardIntent{};
	_float fRightIntent{};
	if (CGameInstance::Get().KeyPressing(DIK_W) || CGameInstance::Get().KeyPressing(DIK_UP))
		fForwardIntent += 1.f;
	if (CGameInstance::Get().KeyPressing(DIK_S) || CGameInstance::Get().KeyPressing(DIK_DOWN))
		fForwardIntent -= 1.f;
	if (CGameInstance::Get().KeyPressing(DIK_D) || CGameInstance::Get().KeyPressing(DIK_RIGHT))
		fRightIntent += 1.f;
	if (CGameInstance::Get().KeyPressing(DIK_A) || CGameInstance::Get().KeyPressing(DIK_LEFT))
		fRightIntent -= 1.f;

	_float3 vCameraForward{};
	_float3 vCameraRight{};
	XMStoreFloat3(&vCameraForward, pPlayerCamera->GetTransform().GetState(STATE::LOOK));
	XMStoreFloat3(&vCameraRight, pPlayerCamera->GetTransform().GetState(STATE::RIGHT));
	vCameraForward.y = 0.f;
	vCameraRight.y = 0.f;

	const _float fForwardLengthSq = vCameraForward.x * vCameraForward.x + vCameraForward.z * vCameraForward.z;
	const _float fRightLengthSq = vCameraRight.x * vCameraRight.x + vCameraRight.z * vCameraRight.z;
	if (fForwardLengthSq > std::numeric_limits<_float>::epsilon())
	{
		const _float fInvLength = 1.f / std::sqrt(fForwardLengthSq);
		vCameraForward.x *= fInvLength;
		vCameraForward.z *= fInvLength;
	}
	if (fRightLengthSq > std::numeric_limits<_float>::epsilon())
	{
		const _float fInvLength = 1.f / std::sqrt(fRightLengthSq);
		vCameraRight.x *= fInvLength;
		vCameraRight.z *= fInvLength;
	}

	const _float3 vMoveDirection{ vCameraForward.x * fForwardIntent + vCameraRight.x * fRightIntent, 0.f, vCameraForward.z * fForwardIntent + vCameraRight.z * fRightIntent };
	m_bRawMoveInput = vMoveDirection.x != 0.f || vMoveDirection.z != 0.f;
	m_bSprintRequested =m_bRawMoveInput &&CGameInstance::Get().KeyPressing(DIK_LSHIFT);
	// 원작처럼 C를 누르는 동안만 걷는다. Shift가 함께 눌리면 Sprint를 우선한다.
	m_bWalkRequested = m_bRawMoveInput && !m_bSprintRequested &&
		CGameInstance::Get().KeyPressing(DIK_C);
	m_vRawMoveDirection = m_bRawMoveInput ? vMoveDirection : _float3{};
	
	if (m_bRawMoveInput)
	{
		m_vLastMoveDirection = vMoveDirection;

		const _vector vTargetDirection = XMVector3Normalize(XMLoadFloat3(&m_vRawMoveDirection));

		if (m_bMovementLocked || m_fCurrentMoveSpeed <= std::numeric_limits<_float>::epsilon())
		{
			XMStoreFloat3(&m_vSmoothedMoveDirection, vTargetDirection);
		}
		else
		{
			const _float fDirectionResponse = m_bSprintRequested
				? m_fSprintDirectionResponse
				: m_fJogDirectionResponse;
			const _float fDirectionBlend = 1.f - std::exp(-fDirectionResponse * fTimeDelta);
			const _vector vSmoothedDirection = XMVector3Normalize(XMVectorLerp(
					XMLoadFloat3(&m_vSmoothedMoveDirection),
					vTargetDirection,
					std::clamp(fDirectionBlend, 0.f, 1.f)));
			XMStoreFloat3(&m_vSmoothedMoveDirection,vSmoothedDirection);
		}
	}

	if (m_bMovementLocked)
	{
		m_fCurrentMoveSpeed = 0.f;
		m_pComMoveIntent->ClearMoveIntent();
	}
	else
	{
		const _float fTargetSpeed =m_bRawMoveInput
				? (m_bSprintRequested ? m_fSprintSpeed : (m_bWalkRequested ? m_fWalkSpeed : m_fJogSpeed))
				: 0.f;
		const _float fSpeedChange = (m_bRawMoveInput ? m_fAcceleration : m_fDeceleration) * fTimeDelta;

		if (m_fCurrentMoveSpeed < fTargetSpeed)
		{
			m_fCurrentMoveSpeed = std::min(m_fCurrentMoveSpeed + fSpeedChange,fTargetSpeed);
		}
		else
		{
			m_fCurrentMoveSpeed = std::max(m_fCurrentMoveSpeed - fSpeedChange,fTargetSpeed);
		}

		if (m_fCurrentMoveSpeed > std::numeric_limits<_float>::epsilon())
		{
			m_pComMoveIntent->SetMoveIntent(m_bRawMoveInput
					? m_vSmoothedMoveDirection
					: m_vLastMoveDirection,m_fCurrentMoveSpeed);
		}
		else
		{
			m_fCurrentMoveSpeed = 0.f;
			m_pComMoveIntent->ClearMoveIntent();
		}
	}

	if (CGameInstance::Get().KeyPressing(DIK_LSHIFT) && CGameInstance::Get().KeyDown(DIK_R))
	{
		//m_pComMoveIntent->RequestWarp({ -6.f, -215.f, 156.f });
		m_pComMoveIntent->RequestWarp(m_vInitialPosition);
	}

	if (m_pStateMachine &&m_pComCharacterMotor &&m_pStateMachine->GetCurrentState() == PLAYER_STATE::LOCOMOTION &&m_pComCharacterMotor->IsGrounded() &&CGameInstance::Get().KeyDown(DIK_SPACE))
	{
		m_pStateMachine->RequestState(PLAYER_STATE::JUMP);
	}

	if (m_pStateMachine && m_pComCharacterMotor &&
		m_pStateMachine->GetCurrentState() == PLAYER_STATE::LOCOMOTION &&
		m_pComCharacterMotor->IsGrounded() &&
		m_iHp > 0 && !m_bProtegoActive &&
		CGameInstance::Get().KeyDown(DIK_Q))
	{
		m_pStateMachine->RequestState(PLAYER_STATE::PROTEGO_SKILL);
	}

	if (m_pStateMachine &&
		m_pStateMachine->GetCurrentState() == PLAYER_STATE::LOCOMOTION &&
		m_iHp > 0 && !m_bFlyRequested &&
		CGameInstance::Get().KeyDown(DIK_G))
	{
		m_pStateMachine->RequestState(PLAYER_STATE::POTION);
	}

	// 프로테고가 실제 공격을 막은 뒤에도 Q를 유지하고 있을 때만
	// 스투피파이 반격 애니메이션을 요청한다. 단순 방어 후 Q를 놓으면 방어만 종료된다.
	if (m_pStateMachine &&
		m_fParryCounterRemainTime > 0.f &&
		!m_bStupefyCounterRequested &&
		CGameInstance::Get().KeyPressing(DIK_Q))
	{
		m_bStupefyCounterRequested = true;
		if (m_pStateMachine->GetCurrentState() != PLAYER_STATE::STUPEFY_SKILL &&
			!m_pStateMachine->RequestState(PLAYER_STATE::STUPEFY_SKILL))
			m_bStupefyCounterRequested = false;
	}

	if (m_pStateMachine &&CGameInstance::Get().MouseDown(MOUSEKEYSTATE::LB))
	{
		const PLAYER_STATE eCurrentState =m_pStateMachine->GetCurrentState();
		const _bool bCanRequestAttack =
			eCurrentState != PLAYER_STATE::JUMP &&
			(eCurrentState != PLAYER_STATE::ROLL ||
			(m_pModelAnimator &&
				PlayerAnimationRatioGuard::Sanitize(
					m_pModelAnimator->GetPlayAnimRatio()) >=
				CPlayer_Roll_State::ATTACK_CANCEL_RATIO));

		if (bCanRequestAttack)
			m_pStateMachine->RequestState(PLAYER_STATE::ATTACK);
	}
	if (m_pStateMachine && CGameInstance::Get().MousePressing(MOUSEKEYSTATE::RB))
	{
		//  가까이 있는거 한번 더 감지 
		auto ori = m_pComTransform->GetPosition();


		std::vector<PX_OVERLAP_RESULT> results{};
		if (CGameInstance::Get().GetPhysXManager()->OverlapMultiple(PX_OVERLAP_DESC{ .tGeometry = {.eType = PX_QUERY_GEOMETRY_TYPE::SPHERE, .fRadius = 25.f}, .tPose = {.vPosition = ori},.tFilter = {.iQueryMask = ETOUI(COLLISION_LAYER::ENEMY_BODY)} }, results))
		{
			auto* pCamera = CGameInstance::Get().GetActiveCamera("PlayerCamera");
			CGameObject* pBestTarget = nullptr;
			_float fBestAlignment = -1.f;

			if (pCamera)
			{
				const _vector vCameraPosition =
					pCamera->GetTransform().GetState(STATE::POSITION);
				const _vector vCameraLook = XMVector3Normalize(
					pCamera->GetTransform().GetState(STATE::LOOK));

				for (const auto& result : results)
				{
					auto* pCandidate = result.pGameObject;
					if (!pCandidate || pCandidate->GetPendingDestroy() ||
						nullptr == dynamic_cast<CSkillTarget*>(pCandidate)) // 창준변경
						continue;

					_vector vToTarget =
						pCandidate->GetTransform().GetState(STATE::POSITION) -
						vCameraPosition;
					if (XMVectorGetX(XMVector3LengthSq(vToTarget)) <=
						std::numeric_limits<_float>::epsilon())
						continue;

					vToTarget = XMVector3Normalize(vToTarget);
					const _float fAlignment = XMVectorGetX(
						XMVector3Dot(vCameraLook, vToTarget));

					if (fAlignment > fBestAlignment)
					{
						fBestAlignment = fAlignment;
						pBestTarget = pCandidate;
					}
				}
			}

			if (pBestTarget)
			{
				const CHandle hDetectedTarget = pBestTarget->GetHandle();
				if (!(hDetectedTarget == m_hAutoTarget))
				{
					m_hPrevAutoTarget = m_hAutoTarget;
					m_hAutoTarget = hDetectedTarget;
				}
			}
		}
		else
		{
			//m_hPrevAutoTarget = m_hAutoTarget;
			//m_hAutoTarget = CHandle{};
			
		}
	}
	else {
		auto* pUIController = CGameInstance::Get().GetGameObjectByHandleT<CUIController>(m_UIHandle);
		CGameObject* pTarget = CGameInstance::Get().GetGameObjectByHandle(m_hAutoTarget);
		//  그냥 일상시 타깃 감지
		if (!pTarget) {
			auto ori = m_pComTransform->GetPosition();
		

			std::vector<PX_OVERLAP_RESULT> results{};
			if (CGameInstance::Get().GetPhysXManager()->OverlapMultiple(PX_OVERLAP_DESC{ .tGeometry = {.eType = PX_QUERY_GEOMETRY_TYPE::SPHERE, .fRadius = 40.f}, .tPose = {.vPosition = ori},.tFilter = {.iQueryMask = ETOUI(COLLISION_LAYER::ENEMY_BODY)} }, results))
			{

				const auto& result = results.front();
				const CHandle hDetectedTarget = result.pGameObject->GetHandle();

				if (!(hDetectedTarget == m_hAutoTarget)) {
					m_hPrevAutoTarget = m_hAutoTarget;
					m_hAutoTarget = hDetectedTarget;
				}
			}
		}

		if (pTarget) {
			auto ori = m_pComTransform->GetPosition();


			std::vector<PX_OVERLAP_RESULT> results{};

			const bool bOverlapped =CGameInstance::Get().GetPhysXManager()->OverlapMultiple(PX_OVERLAP_DESC{.tGeometry = {.eType = PX_QUERY_GEOMETRY_TYPE::SPHERE,.fRadius = 40.f},.tPose = {.vPosition = ori},.tFilter = {.iQueryMask =ETOUI(COLLISION_LAYER::ENEMY_BODY)}},results);

			const bool bTargetStillInRange =bOverlapped &&std::ranges::any_of(results,[this](const PX_OVERLAP_RESULT& result){return result.pGameObject &&result.pGameObject->GetHandle() == m_hAutoTarget;});

			if (!bTargetStillInRange)
			{
				m_hPrevAutoTarget = m_hAutoTarget;
				m_hAutoTarget = CHandle{};
				m_bDistanceUI = false;
		
			}
		}
	}

	// 충돌/타겟 판정은 그대로 두고, 감지 상태가 바뀌는 순간에만 UI를 토글한다.
	if (auto* pUIController = CGameInstance::Get().GetGameObjectByHandleT<CUIController>(m_UIHandle))
	{
		CHandle hDetectedTarget{};

		if (CGameInstance::Get().GetGameObjectByHandleT<CMonster>(m_hAutoTarget))
			hDetectedTarget = m_hAutoTarget;

		// 감지 여부뿐 아니라 실제 대상 핸들이 바뀌었는지 검사한다.
		if (hDetectedTarget != m_hMonsterHPUITarget)
		{
			if (CGameInstance::Get().GetGameObjectByHandleT<CMonster>(hDetectedTarget))
				pUIController->TargetMonsterHP(hDetectedTarget);
			else
				pUIController->DeleteMonsterHP();

			m_hMonsterHPUITarget = hDetectedTarget;
		}
	}
	// 타겟 봐야 하는 곳 -----------------------------------------------------------------------------------------------------------
	if (auto* pOutlineTarget =
		CGameInstance::Get().GetGameObjectByHandleT<CMonster>(m_hAutoTarget);
		pOutlineTarget && !pOutlineTarget->GetPendingDestroy())
	{
		CGameInstance::Get().Apply_OutlineEffect(
			std::optional<CHandle>{ m_hAutoTarget });
	}
	else
	{
		CGameInstance::Get().Apply_OutlineEffect(std::nullopt);
	}

	if (CGameInstance::Get().KeyDown(DIK_LCONTROL))
	{
		m_fControlHoldTime = 0.f;
		m_bDashTriggered = false;
	}

	if (CGameInstance::Get().KeyPressing(DIK_LCONTROL))
	{
		m_fControlHoldTime += fTimeDelta;

		if (!m_bDashTriggered &&m_fControlHoldTime >= DASH_HOLD_TIME)
		{
			if (m_pStateMachine->RequestState(PLAYER_STATE::DASH_SKILL))
			{
				m_bDashTriggered = true;
			}
		}
	}

	if (CGameInstance::Get().KeyUp(DIK_LCONTROL))
	{
		if (!m_bDashTriggered && m_fControlHoldTime < DASH_HOLD_TIME)
		{
			m_pStateMachine->RequestState(PLAYER_STATE::ROLL);
		}

		m_fControlHoldTime = 0.f;
		m_bDashTriggered = false;
	}

	if (CGameInstance::Get().KeyDown(DIK_X) &&
		CPlayer_SkillStateBase::HasValidTarget(*this))
	{
		if (auto* pUIController =
			CGameInstance::Get().GetGameObjectByHandleT<CUIController>(m_UIHandle))
		{
			pUIController->AddFinisher(-100.f / 3.f);
		}
		m_pStateMachine->RequestState(PLAYER_STATE::ACIENTATTACK_SKILL);
	}

	if (m_pStateMachine && CGameInstance::Get().KeyDown(DIK_E))
	{
		auto* pEnemyTarget = CGameInstance::Get().GetGameObjectByHandle(m_hAutoTarget);
		if (pEnemyTarget && !pEnemyTarget->GetPendingDestroy())
		{
			m_hPendingAncientThrowTarget = FindAncientThrowTarget();
			if (m_hPendingAncientThrowTarget &&
				!m_pStateMachine->RequestState(PLAYER_STATE::ACIENTATTACK_SKILL))
			{
				m_hPendingAncientThrowTarget.reset();
			}
		}
	}

	 // 임시
	if (m_bCoolTime_Num1 == true) {
		if (m_fCoolTime_Num1 > 3.f) {
			m_bCoolTime_Num1 = false;
			m_fCoolTime_Num1 = 0.f;
		}
		else {
			m_fCoolTime_Num1 += fTimeDelta;
		}
	}
	if (m_bCoolTime_Num2 == true) {
		if (m_fCoolTime_Num2 > 3.f) {
			m_bCoolTime_Num2 = false;
			m_fCoolTime_Num2 = 0.f;
		}
		else {
			m_fCoolTime_Num2 += fTimeDelta;
		}
	}
	if (m_bCoolTime_Num3 == true) {
		if (m_fCoolTime_Num3 > 3.f) {
			m_bCoolTime_Num3 = false;
			m_fCoolTime_Num3 = 0.f;
		}
		else {
			m_fCoolTime_Num3 += fTimeDelta;
		}
	}
	if (m_bCoolTime_Num4 == true) {
		if (m_fCoolTime_Num4 > 3.f) {
			m_bCoolTime_Num4 = false;
			m_fCoolTime_Num4 = 0.f;
		}
		else {
			m_fCoolTime_Num4 += fTimeDelta;
		}
	}


	if (!m_bFlyRequested) {
		if (CGameInstance::Get().KeyDown(DIK_1) && !m_bCoolTime_Num1) {
			//if (TryUseSkillSlot(1))
			if (m_pStateMachine->RequestState(PLAYER_STATE::ACCIO_SKILL))
				m_bCoolTime_Num1 = true;
		}

		if (CGameInstance::Get().KeyDown(DIK_2) && !m_bCoolTime_Num2)
		{
			//if (TryUseSkillSlot(2))
			if (m_pStateMachine->RequestState(PLAYER_STATE::DEPULSO_SKILL))
				m_bCoolTime_Num2 = true;
		}
		if (CGameInstance::Get().KeyDown(DIK_3) && !m_bCoolTime_Num3)
		{
			//if (TryUseSkillSlot(3))
			if (m_pStateMachine->RequestState(PLAYER_STATE::DESCENDO_SKILL))
				m_bCoolTime_Num3 = true;
		}

		if (CGameInstance::Get().KeyDown(DIK_4) && !m_bCoolTime_Num4) {
			if (m_pStateMachine->RequestState(PLAYER_STATE::REPAIRO_SKILL))
				m_bCoolTime_Num4 = true;
		}

		// [LSY] 스킬 슬롯에서 CONFRINGO를 연결하기 전까지 5번 키로 직접 테스트한다.
		if (CGameInstance::Get().KeyDown(DIK_5))
			m_pStateMachine->RequestState(PLAYER_STATE::CONFRINGO_SKILL);

		// 봄바르다 애니메이션 및 이펙트 큐 타이밍 확인용 임시 입력.
		if (CGameInstance::Get().KeyDown(DIK_6))
			m_pStateMachine->RequestState(PLAYER_STATE::BOMBARDA_SKILL);

		// 변신 스킬 상태와 캐스팅 애니메이션 확인용 임시 입력.
		if (CGameInstance::Get().KeyDown(DIK_7))
			m_pStateMachine->RequestState(PLAYER_STATE::TRANSFORMATION_SKILL);

		// L 키는 빌드 구성과 무관한 정식 루모스 토글 입력이다.
		// Lumos 상태가 현재 활성 여부에 따라 Start/Hold 또는 Stop을 선택한다.
		if (CGameInstance::Get().KeyDown(DIK_L))
			m_pStateMachine->RequestState(PLAYER_STATE::LUMOS_SKILL);

		// [LSY] 아바다 케다브라 애니메이션과 이펙트 연결 확인용 임시 입력.
		if (CGameInstance::Get().KeyDown(DIK_U))
			m_pStateMachine->RequestState(PLAYER_STATE::AVADA_KEDAVRA_SKILL);

	}
	


	if (m_pStateMachine && CGameInstance::Get().KeyDown(DIK_H))
	{
		// Protego HIT debug: sample the entire sphere uniformly so front, side,
		// back, top, and bottom visibility can all be checked with repeated hits.
		const _float fZ = Randf(-1.f, 1.f);
		const _float fAzimuth = Randf(0.f, XM_2PI);
		const _float fPlanarRadius = sqrtf(std::max(0.f, 1.f - fZ * fZ));
		const _float3 vRandomDirection{
			fPlanarRadius * cosf(fAzimuth),
			fZ,
			fPlanarRadius * sinf(fAzimuth)
		};

		_float3 vHitPosition = GetTransform().GetPosition();
		vHitPosition.y += 1.f;
		vHitPosition.x += vRandomDirection.x * 2.48f;
		vHitPosition.y += vRandomDirection.y * 2.48f;
		vHitPosition.z += vRandomDirection.z * 2.48f;
		OnQueryHit(20, vHitPosition);
	}

#ifdef _DEBUG
	if (m_pStateMachine && CGameInstance::Get().KeyDown(DIK_K))
	{
		// Knockdown debug: treat a point in front of the player as the attacker.
		// RequestKnockdown does not change HP, so the full sequence can be tested repeatedly.
		_vector vAttackPosition = GetTransform().GetState(STATE::POSITION);
		_vector vLook = XMVectorSetY(GetTransform().GetState(STATE::LOOK), 0.f);
		if (XMVectorGetX(XMVector3LengthSq(vLook)) > FLT_EPSILON)
			vAttackPosition += XMVector3Normalize(vLook) * 2.f;

		_float3 vAttackPositionValue{};
		XMStoreFloat3(&vAttackPositionValue, vAttackPosition);
		RequestKnockdown(vAttackPositionValue);
	}
#endif
}



void CPlayer::InitializeSkillSlotUI()
{
	if (m_bSkillSlotUIInitialized)
		return;

	auto* pUIController =
		CGameInstance::Get().GetGameObjectByHandleT<CUIController>(m_UIHandle);
	if (!pUIController)
		return;

	// 테스트용 코드 나중에 실제 프로토타입 시연회 때는 지워야 함 ---------------------------------------
	uint32_t level = E::CGameInstance::Get().GetCurrentLevelID();

	if (level == ETOUI(LEVEL::CHARLES_ROOKWOOD))
	{
		pUIController->SetSpellType(1, ETOUI(SPELL_TYPE::NONE));
		pUIController->SetSpellType(2, ETOUI(SPELL_TYPE::NONE));
		pUIController->SetSpellType(3, ETOUI(SPELL_TYPE::NONE));

		// SPELL_TYPE에 REVELIO가 추가되기 전까지 4번 슬롯은 빈 슬롯으로 둔다.
		pUIController->SetSpellType(4, ETOUI(SPELL_TYPE::NONE));
	}
	else if (level == ETOUI(LEVEL::BOSS_CHARLES_ROOKWOOD))
	{
		pUIController->SetSpellType(1, ETOUI(SPELL_TYPE::ASSIO));
		pUIController->SetSpellType(2, ETOUI(SPELL_TYPE::DEPULSO));
		pUIController->SetSpellType(3, ETOUI(SPELL_TYPE::DESENDO));

		// SPELL_TYPE에 REVELIO가 추가되기 전까지 4번 슬롯은 빈 슬롯으로 둔다.
		pUIController->SetSpellType(4, ETOUI(SPELL_TYPE::REPARO));
	}


	m_bSkillSlotUIInitialized = true;
}

_bool CPlayer::TryUseSkillSlot(uint32_t iSlotNumber)
{
	auto* pUIController =
		CGameInstance::Get().GetGameObjectByHandleT<CUIController>(m_UIHandle);
	if (!pUIController || !m_pStateMachine)
		return false;

	const SPELL_TYPE eSpellType = static_cast<SPELL_TYPE>(
		pUIController->GetSpellType(iSlotNumber));
	PLAYER_STATE eSkillState = PLAYER_STATE::NONE;
	switch (eSpellType)
	{
	case SPELL_TYPE::ASSIO:
		eSkillState = PLAYER_STATE::ACCIO_SKILL;
		break;

	case SPELL_TYPE::DEPULSO:
		eSkillState = PLAYER_STATE::DEPULSO_SKILL;
		break;

	case SPELL_TYPE::DESENDO:
		eSkillState = PLAYER_STATE::DESCENDO_SKILL;
		break;

	case SPELL_TYPE::REPARO:
		eSkillState = PLAYER_STATE::REPAIRO_SKILL;
		break;

	case SPELL_TYPE::LUMOS:
		eSkillState = PLAYER_STATE::LUMOS_SKILL;
		break;

	default:
		// 플레이어에 구현되지 않았거나 비어 있는 스킬 슬롯이다.
		return false;
	}

	if (!m_pStateMachine->RequestState(eSkillState))
		return false;

	if (eSpellType != SPELL_TYPE::LUMOS)
		pUIController->UseSpell(iSlotNumber);
	return true;
}

void CPlayer::SetLumosActive(_bool bActive)
{
	if (m_bLumosActive == bActive)
		return;

	m_bLumosActive = bActive;
	m_bHasPreviousLumosAttachPosition = false;
	if (!bActive)
	{
		if (m_pModelAnimator && m_pModelAnimator->HasUpperAnimation())
			m_pModelAnimator->Stop_UpperAnim(0.08f);

		if (m_iLumosEffectID != INVALID_EFFECT_INSTANCE_ID)
		{
			CGameInstance::Get().StopEffect(m_iLumosEffectID);
			m_iLumosEffectID = INVALID_EFFECT_INSTANCE_ID;
		}

		if (m_hLumosLight)
		{
			if (auto* pLight = CGameInstance::Get().GetGameObjectByHandleT<CLight>(*m_hLumosLight))
				pLight->Reset_Light();
			m_hLumosLight.reset();
		}
		return;
	}

	// The glow is created in UpdateAttachedEffects() after animation finishes,
	// using the exact same finalized spawn matrix as player projectiles.
}

void CPlayer::UpdateLumosLight()
{
	if (!m_bLumosActive)
	{
		if (m_iLumosEffectID != INVALID_EFFECT_INSTANCE_ID)
		{
			CGameInstance::Get().StopEffect(m_iLumosEffectID);
			m_iLumosEffectID = INVALID_EFFECT_INSTANCE_ID;
		}
		if (m_hLumosLight)
		{
			if (auto* pLight = CGameInstance::Get().GetGameObjectByHandleT<CLight>(*m_hLumosLight))
				pLight->Reset_Light();
			m_hLumosLight.reset();
		}
		return;
	}

	UpdateLumosHoldAnimation();

	_float4x4 matGlowWorld{};
	if (!TryGetLumosGlowWorldMatrix(matGlowWorld))
		return;

	const _float3 vPosition{ matGlowWorld._41, matGlowWorld._42, matGlowWorld._43 };
	m_vLumosDebugWorldPosition = vPosition;
	_float3 vPredictedPosition = vPosition;
	if (m_bHasPreviousLumosAttachPosition)
	{
		_vector vFrameDelta =
			XMLoadFloat3(&vPosition) - XMLoadFloat3(&m_vPreviousLumosAttachPosition);
		const _float fDeltaLength = XMVectorGetX(XMVector3Length(vFrameDelta));
		constexpr _float MAX_PREDICTION_DISTANCE = 0.35f;
		if (fDeltaLength > MAX_PREDICTION_DISTANCE)
			vFrameDelta *= MAX_PREDICTION_DISTANCE / fDeltaLength;
		XMStoreFloat3(&vPredictedPosition, XMLoadFloat3(&vPosition) + vFrameDelta);
	}
	m_vPreviousLumosAttachPosition = vPosition;
	m_bHasPreviousLumosAttachPosition = true;

	_float4x4 matPredictedGlowWorld{};
	XMStoreFloat4x4(&matPredictedGlowWorld, XMMatrixTranslation(
		vPredictedPosition.x, vPredictedPosition.y, vPredictedPosition.z));
	if (m_iLumosEffectID == INVALID_EFFECT_INSTANCE_ID)
	{
		m_iLumosEffectID = CGameInstance::Get().PlayEffect(
			"KMS_Lumos_WandGlow", matPredictedGlowWorld);
	}
	else
	{
		CGameInstance::Get().SetEffectWorldMatrix(
			m_iLumosEffectID, matPredictedGlowWorld);
	}

	if (!m_hLumosLight)
	{
		m_hLumosLight = CGameInstance::Get().Allocate_EffectLight(
			XMLoadFloat3(&vPosition), 220.f, { 1.f, 0.94f, 0.78f }, 6.f, 13.f,
			99999.f, { 0.f, 0.f, 0.f });
		return;
	}

	if (auto* pLight = CGameInstance::Get().GetGameObjectByHandleT<CLight>(*m_hLumosLight))
		pLight->Set_LightPosition(vPosition);
	else
		m_hLumosLight.reset();
}

void CPlayer::UpdateLumosHoldAnimation()
{
	if (!m_bLumosActive || !m_pModelAnimator || m_iLumosHoldAnimation < 0)
		return;

	if (m_pModelAnimator->HasUpperAnimation() &&
		!m_pModelAnimator->IsUpperAnimationFinished())
	{
		return;
	}

	// The Lumos state returns to locomotion immediately. Once its one-shot
	// raise animation finishes, keep only the right arm in the looping hold
	// pose so movement, turning, and jumping continue underneath it.
	PlayUpperBodyAnimation(
		m_iLumosHoldAnimation, "RightArm", 1, true, 0.18f);
}

_bool CPlayer::TryGetLumosGlowWorldMatrix(_float4x4& outWorld) const
{
	auto* pWeapon = CGameInstance::Get().GetGameObjectByHandleT<CPlayer_Weapon>(
		m_Partes[ETOUI(PARTES::WEAPON)]);
	if (!pWeapon)
		return false;

	const _float4x4 matWandTip = pWeapon->GetSpawnWorldMatrix();
	const _float3 vRightAxis{ matWandTip._11, matWandTip._12, matWandTip._13 };
	const _float3 vUpAxis{ matWandTip._21, matWandTip._22, matWandTip._23 };
	const _float3 vForwardAxis{ matWandTip._31, matWandTip._32, matWandTip._33 };
	_vector vRight = XMVector3Normalize(XMLoadFloat3(&vRightAxis));
	_vector vUp = XMVector3Normalize(XMLoadFloat3(&vUpAxis));
	_vector vForward = XMVector3Normalize(XMLoadFloat3(&vForwardAxis));
	_vector vAttachPosition = XMVectorSet(
		matWandTip._41, matWandTip._42, matWandTip._43, 1.f);
	vAttachPosition += vRight * m_vLumosLocalOffset.x;
	vAttachPosition += vUp * m_vLumosLocalOffset.y;
	vAttachPosition += vForward * m_vLumosLocalOffset.z;
	_float3 vPosition{};
	XMStoreFloat3(&vPosition, vAttachPosition);
	XMStoreFloat4x4(&outWorld, XMMatrixTranslation(vPosition.x, vPosition.y, vPosition.z));
	return true;
}

void CPlayer::FixedUpdate(_float fTimeDelta)
{
	m_fFootstepSoundCooldown =
		std::max(0.f, m_fFootstepSoundCooldown - fTimeDelta);

	// [LSY] 랙돌 전환 중에는 CCT와 캐릭터 모터의 일반 물리 갱신을 중단한다.
	if (m_pRagdollController)
	{
		if (m_pRagdollController->PreFixedUpdate())
			return;
	}

	if (m_bMovementLocked)
	{
		m_pComMoveIntent->ClearMoveIntent();

		_float3 vVelocity = m_pComCharacterMotor->GetVelocity();
		if (!m_pComCharacterMotor->IsPreservingHorizontalVelocity())
		{
			vVelocity.x = 0.f;
			vVelocity.z = 0.f;
			m_pComCharacterMotor->SetVelocity(vVelocity);
		}
	}

	// 프로테고 반동은 상태 전환과 독립적으로 유지한다. 같은 프레임에
	// 스투페파이 반격 상태로 넘어가도 남은 시간 동안 CCT에 계속 적용된다.
	if (m_fProtegoRecoilRemainTime > 0.f && m_pComMoveIntent)
	{
		const _float fRecoilRatio = std::clamp(
			m_fProtegoRecoilRemainTime / PROTEGO_RECOIL_DURATION, 0.f, 1.f);
		const _float fRecoilSpeed = PROTEGO_RECOIL_SPEED * fRecoilRatio;
		m_pComMoveIntent->AddExternalDisplacement({
			m_vProtegoRecoilDirection.x * fRecoilSpeed * fTimeDelta,
			0.f,
			m_vProtegoRecoilDirection.z * fRecoilSpeed * fTimeDelta });
		m_fProtegoRecoilRemainTime = std::max(
			0.f, m_fProtegoRecoilRemainTime - fTimeDelta);
	}

	ApplyGroundFollow(fTimeDelta);
	m_pComCharacterMotor->FixedUpdate(fTimeDelta);
	// Motor가 갱신한 루트 위치를 본 월드 행렬과 동일한 프레임으로 맞춘다.
	GetTransform().Update();

	const _float3 vPlayerPosition = GetTransform().GetPosition();
	const _float4 vPlayerRotation = GetTransform().GetQuaternion();
	m_pComPxRigidBody->SetKinematicTarget(vPlayerPosition, vPlayerRotation);

	if (m_pComModelInstance)
	{
		const auto& CombinedBones =
			m_pComModelInstance->Get_CombinedBoneMatrices();
		const _matrix PlayerPhysicsWorld =
			XMMatrixRotationQuaternion(XMLoadFloat4(&vPlayerRotation)) *
			XMMatrixTranslation(vPlayerPosition.x, vPlayerPosition.y, vPlayerPosition.z);
		const _matrix InversePlayerPhysicsWorld =
			XMMatrixInverse(nullptr, PlayerPhysicsWorld);

		const auto UpdateColliderLocalPose =
			[&](CComPxCollider* pCollider, int32_t iBoneIndex,
				_float fVerticalOffset = 0.f)
		{
			if (!pCollider || iBoneIndex < 0 ||
				static_cast<size_t>(iBoneIndex) >= CombinedBones.size())
				return;

			const _matrix ColliderWorld =
				XMLoadFloat4x4(&CombinedBones[static_cast<size_t>(iBoneIndex)]) *
				GetTransform().GetLoadedCombinedWorldMatrix();
			const _matrix ColliderLocal =
				ColliderWorld * InversePlayerPhysicsWorld;

			_vector vScale{};
			_vector vRotation{};
			_vector vTranslation{};
			if (XMMatrixDecompose(
				&vScale,
				&vRotation,
				&vTranslation,
				ColliderLocal))
			{
				_float3 vLocalPosition{};
				_float4 vLocalRotation{};
				XMStoreFloat3(&vLocalPosition, vTranslation);
				vLocalPosition.y += fVerticalOffset;
				XMStoreFloat4(&vLocalRotation,
					XMQuaternionNormalize(vRotation));
				pCollider->SetLocalPosition(vLocalPosition);
				pCollider->SetLocalRotation(vLocalRotation);
			}
		};

		UpdateColliderLocalPose(m_pComPxBoxCollider, m_iHurtBoxBoneIndex);
		UpdateColliderLocalPose(
			m_pComPxLeftFootCollider, m_iLeftFootBoneIndex, -0.12f);
		UpdateColliderLocalPose(
			m_pComPxRightFootCollider, m_iRightFootBoneIndex, -0.12f);
	}

	if (m_pRagdollController)
		m_pRagdollController->PostFixedUpdate();

#ifdef _DEBUG
	UpdateStandingGameObjectDebugLog();
#endif

}

void CPlayer::ApplyGroundFollow(_float fFixedTimeDelta)
{
	if (!m_pComCharacterController ||!m_pComCharacterMotor ||!m_pComMoveIntent ||!m_pStateMachine ||fFixedTimeDelta <= 0.f)
	{
		return;
	}

	if (m_pStateMachine->GetCurrentState() != PLAYER_STATE::LOCOMOTION ||!m_pComCharacterMotor->IsGrounded() ||m_pComMoveIntent->HasJumpRequest())
	{
		return;
	}

	const CComCharacterMoveIntent::OUTPUT& tMoveOutput = m_pComMoveIntent->GetOutput();
	if (!tMoveOutput.bMoveRequested || tMoveOutput.fMoveSpeed <= std::numeric_limits<_float>::epsilon())
	{
		return;
	}

	const _float3 vFootPosition = m_pComCharacterController->GetFootPosition();
	const _float fPredictionDistance =
		tMoveOutput.fMoveSpeed * fFixedTimeDelta *
		static_cast<_float>(m_iGroundFollowPredictionFrames);
	const _float3 vPredictedFootPosition{
		vFootPosition.x +
			tMoveOutput.vMoveDirection.x *
			fPredictionDistance,
		vFootPosition.y,
		vFootPosition.z +
			tMoveOutput.vMoveDirection.z *
			fPredictionDistance
	};

	CPhysXManager* pPhysXManager =CGameInstance::Get().GetPhysXManager();
	if (!pPhysXManager)
		return;

	PX_SWEEP_DESC tSweepDesc{};
	tSweepDesc.tGeometry.eType = PX_QUERY_GEOMETRY_TYPE::SPHERE;
	tSweepDesc.tGeometry.fRadius = m_fGroundFollowProbeRadius;
	tSweepDesc.tPose.vPosition = {
		vPredictedFootPosition.x,
		vPredictedFootPosition.y +
			m_fGroundFollowProbeStartHeight +
			m_fGroundFollowProbeRadius,
		vPredictedFootPosition.z
	};
	tSweepDesc.vDirection = { 0.f, -1.f, 0.f };
	tSweepDesc.fMaxDistance =
		m_fGroundFollowProbeStartHeight +
		m_fGroundFollowProbeRadius +
		m_fGroundFollowMaxStepDown;
	tSweepDesc.tFilter.iQueryMask =
		m_pComCharacterController->GetFilter().iQueryMask;
	tSweepDesc.tFilter.hIgnoreGameObject = GetHandle();
	tSweepDesc.tFilter.bQueryStatic = true;
	tSweepDesc.tFilter.bQueryDynamic = true;
	tSweepDesc.tFilter.bIncludeTrigger = false;
//
//#ifdef _DEBUG
//	if (auto* pDebugLine = CGameInstance::Get().GetDbgLineRender())
//	{
//		const _float4 vPreviousColor = pDebugLine->GetColor();
//		const DBG_LINE_DEPTH_MODE ePreviousDepthMode =
//			pDebugLine->GetDepthMode();
//
//		const _float3 vSweepStart = tSweepDesc.tPose.vPosition;
//		const _float3 vSweepEnd{
//			vSweepStart.x,
//			vSweepStart.y - tSweepDesc.fMaxDistance,
//			vSweepStart.z
//		};
//
//		pDebugLine->SetDepthTest(false);
//
//		// 노란 구: Sweep 시작 위치
//		pDebugLine->SetColor({ 1.f, 1.f, 0.f, 1.f });
//		pDebugLine->AddSphere(
//			m_fGroundFollowProbeRadius,
//			XMMatrixTranslation(
//				vSweepStart.x,
//				vSweepStart.y,
//				vSweepStart.z));
//
//		// 하늘색 구: 충돌이 없을 때의 Sweep 종료 위치
//		pDebugLine->SetColor({ 0.f, 1.f, 1.f, 1.f });
//		pDebugLine->AddSphere(
//			m_fGroundFollowProbeRadius,
//			XMMatrixTranslation(
//				vSweepEnd.x,
//				vSweepEnd.y,
//				vSweepEnd.z));
//		pDebugLine->AddLine(vSweepStart, vSweepEnd);
//
//		pDebugLine->SetColor(vPreviousColor);
//		pDebugLine->SetDepthMode(ePreviousDepthMode);
//	}
//#endif

	// 이동 경로를 여러 구간으로 나눠 검사한다. 끝점 한 곳만 검사하면 좁은 턱이나
	// 급경사를 건너뛸 수 있으므로 각 샘플의 노멀과 인접 높이 차를 모두 확인한다.
	const int32_t iProbeCount = std::max(1, m_iGroundFollowProbeCount);
	const _float fSlopeLimit =
		m_pComCharacterController->GetSlopeLimit();
	_float fPreviousGroundHeight = vFootPosition.y;
	PX_SWEEP_RESULT tGroundHit{};
	for (int32_t iProbe = 1; iProbe <= iProbeCount; ++iProbe)
	{
		const _float fProbeRatio =
			static_cast<_float>(iProbe) /
			static_cast<_float>(iProbeCount);
		tSweepDesc.tPose.vPosition.x =
			vFootPosition.x +
			(vPredictedFootPosition.x - vFootPosition.x) * fProbeRatio;
		tSweepDesc.tPose.vPosition.z =
			vFootPosition.z +
			(vPredictedFootPosition.z - vFootPosition.z) * fProbeRatio;

		PX_SWEEP_RESULT tProbeHit{};
		if (!pPhysXManager->Sweep(tSweepDesc, tProbeHit) ||
			!tProbeHit.bHit)
		{
			// 경로 중간에 지면이 없으면 낭떠러지로 보고 강제 지면 추종을 중단한다.
			return;
		}

		if (tProbeHit.vHitNormal.y < fSlopeLimit)
		{
			// PhysX slopeLimit보다 급한 면에는 캐릭터를 아래로 붙이지 않는다.
			return;
		}

		const _float fHeightDelta =
			tProbeHit.vHitpos.y - fPreviousGroundHeight;
		if (std::abs(fHeightDelta) >
			m_fGroundFollowMaxHeightDeltaPerProbe)
		{
			// 인접 샘플의 높이가 갑자기 변하면 계단/절벽으로 판단한다.
			return;
		}

		fPreviousGroundHeight = tProbeHit.vHitpos.y;
		if (iProbe == iProbeCount)
			tGroundHit = tProbeHit;
	}

#ifdef _DEBUG
	if (auto* pDebugLine = CGameInstance::Get().GetDbgLineRender())
	{
		const _float4 vPreviousColor = pDebugLine->GetColor();
		const DBG_LINE_DEPTH_MODE ePreviousDepthMode =
			pDebugLine->GetDepthMode();

		// 초록 십자: Sweep이 검출한 실제 지면 접촉점
		pDebugLine->SetDepthTest(false);
		pDebugLine->SetColor({ 0.f, 1.f, 0.f, 1.f });
		pDebugLine->AddCross(tGroundHit.vHitpos, 0.08f);

		pDebugLine->SetColor(vPreviousColor);
		pDebugLine->SetDepthMode(ePreviousDepthMode);
	}
#endif

	const _float fStepDown =
		tGroundHit.vHitpos.y - vFootPosition.y;
	if (fStepDown < 0.f &&
		fStepDown >= -m_fGroundFollowMaxStepDown)
	{
		// 검출된 높이 차를 한 프레임에 전부 적용하면 경계에서 튀므로
		// 최대 추종 속도로 제한해 완만하게 지면에 붙인다.
		const _float fCorrection = std::max(
			fStepDown,
			-m_fGroundFollowMaxCorrectionSpeed * fFixedTimeDelta);
		m_pComMoveIntent->AddExternalDisplacement(
			{ 0.f, fCorrection, 0.f });
	}
}

#ifdef _DEBUG
void CPlayer::UpdateStandingGameObjectDebugLog()
{
	const std::optional<CHandle> hStandingGameObject =
		m_pComCharacterController->GetStandingGameObjectHandle();

	if (hStandingGameObject != m_hDebugStandingGameObject)
	{
		if (hStandingGameObject)
		{
			if (const auto* pStandingGameObject =
				CGameInstance::Get().GetGameObjectByHandle(*hStandingGameObject))
			{
				std::string sLog = "[CPlayer][CCT] Standing On: ";
				const std::string_view sObjectTag = pStandingGameObject->GetObjectTag();
				sLog.append(sObjectTag.data(), sObjectTag.size());
				sLog += " (Handle Index: ";
				sLog += std::to_string(hStandingGameObject->GetIndex());
				sLog += ", Generation: ";
				sLog += std::to_string(hStandingGameObject->GetGeneration());
				sLog += ")\n";
				//DEBUG_LOG_STR(sLog);
			}
			else
			{
				//DEBUG_LOG("[CPlayer][CCT] Standing handle is no longer valid.\n");
			}
		}
		else
		{
			//DEBUG_LOG("[CPlayer][CCT] Standing On: None\n");
		}

		m_hDebugStandingGameObject = hStandingGameObject;
	}
}
#endif

void CPlayer::ApplyAttackForwardMovement(_float fSpeed, _float fTimeDelta)
{
	if (!m_pComMoveIntent ||
		fSpeed <= 0.f ||
		fTimeDelta <= 0.f)
	{
		return;
	}

	_vector vForward = XMVectorSetY(
		GetTransform().GetState(STATE::LOOK),
		0.f);

	if (XMVectorGetX(XMVector3LengthSq(vForward)) <=
		std::numeric_limits<_float>::epsilon())
	{
		return;
	}

	vForward = XMVector3Normalize(vForward);

	_float3 vDisplacement{};
	XMStoreFloat3(
		&vDisplacement,
		vForward * fSpeed * fTimeDelta);

	m_pComMoveIntent->AddExternalDisplacement(vDisplacement);
}

void CPlayer::ApplyDirectionalMovement(const _float3& vDirection,_float fSpeed,_float fTimeDelta)
{
	if (!m_pComMoveIntent ||fSpeed <= 0.f ||fTimeDelta <= 0.f)
	{
		return;
	}

	_vector vMoveDirection = XMVectorSetY(XMLoadFloat3(&vDirection),0.f);

	if (XMVectorGetX(XMVector3LengthSq(vMoveDirection)) <=std::numeric_limits<_float>::epsilon())
	{
		return;
	}

	vMoveDirection = XMVector3Normalize(vMoveDirection);

	_float3 vDisplacement{};
	XMStoreFloat3(
		&vDisplacement,
		vMoveDirection * fSpeed * fTimeDelta);

	m_pComMoveIntent->AddExternalDisplacement(vDisplacement);
}

void CPlayer::PrepareLocomotionResume()
{
	m_fCurrentMoveSpeed = m_bRawMoveInput
		? (m_bSprintRequested ? m_fSprintSpeed : (m_bWalkRequested ? m_fWalkSpeed : m_fJogSpeed))
		: 0.f;

	if (m_bRawMoveInput)
	{
		const _vector vDirection = XMVector3Normalize(
			XMLoadFloat3(&m_vRawMoveDirection));
		XMStoreFloat3(&m_vSmoothedMoveDirection, vDirection);
		m_vLastMoveDirection = m_vSmoothedMoveDirection;
	}
}



void CPlayer::Update(E::_float fTimeDelta)
{
	ZoneScopedN("Update TestModel");
	{


	}


	if (nullptr == CGameInstance::Get().GetGameObjectByHandleT<CUIController>(m_UIHandle))
	{
		m_bSkillSlotUIInitialized = false;

		const auto hUIController = GET_SINGLE(UIManager)->GetUIController();
		if (hUIController.has_value())
			m_UIHandle = *hUIController;
	}
	InitializeSkillSlotUI();

	_bool bApplyRootMotionTranslation{};
	_float3 vRootMotionWorldDisplacement{};

	for (auto iter = m_Projectiles.begin(); iter != m_Projectiles.end();)
	{
		auto* pProjectile = CGameInstance::Get().GetGameObjectByHandle(iter->hProjectile);
		if (!pProjectile)
		{
			iter = m_Projectiles.erase(iter);
			continue;
		}

		iter->fRemainingTime -= fTimeDelta;
		if (iter->fRemainingTime <= 0.f)
		{
			pProjectile->SetPendingDestroyCascade();
			iter = m_Projectiles.erase(iter);
			continue;
		}

		++iter;
	}

	//const _bool bRagdollActiveAtUpdateStart = IsRagdollActive();
	if (!IsRagdollActive() &&
		m_pComModelInstance->GetModel()->GetAnimations().size() != 0) {

		m_pModelAnimator->Update(fTimeDelta);
		bApplyRootMotionTranslation = m_bRootMotionTranslationActive;
		const _float3 vRootMotionDelta =
			m_pModelAnimator->GetRootMotionDelta();

		// Local 이동은 이번 프레임의 Root 회전을 적용하기 전 방향을
		// 기준으로 월드 변환해야 Turn 중 이동 방향이 회전 후 방향으로
		// 한 프레임 먼저 꺾이지 않는다.
		const _vector vWorldDelta = XMVector3Rotate(
			XMLoadFloat3(&vRootMotionDelta) * m_fRootMotionTranslationScale,
			GetTransform().GetLoadedQuaternion());
		XMStoreFloat3(
			&vRootMotionWorldDisplacement,
			vWorldDelta);

		if (m_bRootMotionRotationActive)
		{
			const _float4 vRootMotionRotationDelta =
				m_pModelAnimator->GetRootMotionRotationDelta();
			const _vector qCurrent =
				GetTransform().GetLoadedQuaternion();
			const _vector qDelta =
				XMLoadFloat4(&vRootMotionRotationDelta);

			GetTransform().SetQuaternion(
				XMQuaternionNormalize(
					XMQuaternionMultiply(qDelta, qCurrent)));
		}
	}

	UpdateWiggenweldPotion();

	// Animator를 먼저 진행해야 Locomotion State가 현재 프레임의
	// 재생 비율과 종료 상태로 Turn 회전을 맞출 수 있다.
	if (!CGameInstance::Get().IsAnimationEditorTarget(GetHandle()) &&
		m_pStateMachine &&
		!IsRagdollTransitioning())
		m_pStateMachine->Update(fTimeDelta);

	// Turn 시작 당시 활성 상태를 보관했기 때문에 종료 프레임의
	// 마지막 RootMotionDelta도 빠뜨리지 않고 적용한다.
	if (bApplyRootMotionTranslation &&
		m_pComMoveIntent &&
		!IsRagdollTransitioning())
	{
		m_pComMoveIntent->AddExternalDisplacement(
			vRootMotionWorldDisplacement);
	}

	// [LSY] FixedUpdate에서 변경된 CCT 위치와 Update에서 적용한 회전을
	// [LSY] 최신 World 행렬에 반영한 뒤 랙돌 포즈를 교환한다.
	GetTransform().Update();

	if (m_pRagdollController)
		m_pRagdollController->UpdatePoseBridge();

}

_bool CPlayer::StartWiggenweldPotionUse()
{
	if (!m_pComModelInstance || !m_pComModelInstance->GetModel())
		return false;

	if (auto* pPreviousPotion = CGameInstance::Get()
		.GetGameObjectByHandleT<CWiggenweldPotion>(m_hWiggenweldPotion))
	{
		pPreviousPotion->Drop();
	}

	m_iWiggenweldPotionBoneIndex =
		m_pComModelInstance->GetModel()->Get_BoneIndex("SKT_PotionBottles");
	if (m_iWiggenweldPotionBoneIndex < 0)
		m_iWiggenweldPotionBoneIndex =
			m_pComModelInstance->GetModel()->Get_BoneIndex("prop_LeftHand_potion");
	if (m_iWiggenweldPotionBoneIndex < 0)
		m_iWiggenweldPotionBoneIndex =
			m_pComModelInstance->GetModel()->Get_BoneIndex("SKT_LeftHandSocket_YUp");
	if (m_iWiggenweldPotionBoneIndex < 0)
		m_iWiggenweldPotionBoneIndex =
			m_pComModelInstance->GetModel()->Get_BoneIndex("LeftHand");
	if (m_iWiggenweldPotionBoneIndex < 0)
		return false;

	CWiggenweldPotion::DESC desc{};
	desc.sObjectTag = "WiggenweldPotion";
	desc.sResourceGroup = m_LevelTag;
	desc.vInitialPosition = GetTransform().GetPosition();
	desc.vInitialScale = { 1.f, 1.f, 1.f };
	desc.vConvexScale = desc.vInitialScale;
	const auto handle = CGameInstance::Get().AddGameObjectToLayer(
		m_LevelTag,
		PROTO_GAMEOBJECT::Prototype_GameObject_WiggenweldPotion,
		"PlayerPotion",
		&desc);
	if (!handle.has_value())
		return false;

	m_hWiggenweldPotion = *handle;
	m_bWiggenweldPotionDropped = false;
	DEBUG_LOG("[PlayerPotion] Wiggenweld potion object created.\n");
	UpdateWiggenweldPotion();
	return true;
}

void CPlayer::UpdateWiggenweldPotion()
{
	auto* pPotion = CGameInstance::Get()
		.GetGameObjectByHandleT<CWiggenweldPotion>(m_hWiggenweldPotion);
	if (!pPotion || m_bWiggenweldPotionDropped ||
		!m_pComModelInstance || !m_pModelAnimator)
		return;

	const auto& boneMatrices = m_pComModelInstance->Get_CombinedBoneMatrices();
	if (m_iWiggenweldPotionBoneIndex < 0 ||
		static_cast<size_t>(m_iWiggenweldPotionBoneIndex) >= boneMatrices.size())
		return;

	_matrix socketMatrix = XMLoadFloat4x4(
		&boneMatrices[static_cast<size_t>(m_iWiggenweldPotionBoneIndex)]);
	for (uint32_t axis = 0; axis < 3; ++axis)
		socketMatrix.r[axis] = XMVector3Normalize(socketMatrix.r[axis]);

	const _matrix potionPivotOffset = XMMatrixTranslation(
		-0.120f,
		-0.305f,
		0.105f) *
		socketMatrix;
	socketMatrix = potionPivotOffset;
	if (!pPotion->SetHeldPose(
		socketMatrix * GetTransform().GetLoadedWorldMatrix()))
	{
		DEBUG_LOG("[PlayerPotion] Failed to update the held potion pose.\n");
		return;
	}

	// 팔을 내리는 마지막 구간까지 재생한 뒤에만 포션을 드롭한다.
	if (m_pModelAnimator->HasUpperAnimation() &&
		!m_pModelAnimator->IsUpperAnimationFinished())
		return;

	_float3 vLook{};
	XMStoreFloat3(&vLook, XMVector3Normalize(GetTransform().GetState(STATE::LOOK)));
	const _float3 vImpulse{
		vLook.x * 0.35f,
		0.12f,
		vLook.z * 0.35f
	};
	if (pPotion->Drop(vImpulse, { 0.08f, 0.14f, -0.1f }))
		m_bWiggenweldPotionDropped = true;
}

void CPlayer::LateUpdate(E::_float fTimeDelta)
{
	if (!CGameInstance::Get().IsAnimationEditorTarget(GetHandle()) &&
		m_pStateMachine &&
		!IsRagdollTransitioning())
		m_pStateMachine->LateUpdate(fTimeDelta);


	//m_pComPhysX->UpdateSyncedDataToTransform(m_pComTransform);
	GetTransform().Update();

	// 플레이어 Transform을 먼저 확정한 뒤 같은 프레임의 카메라 View를 갱신한다.


	if (auto* pCamera = Cast<CPlayerThirdPersonCamera>(CGameInstance::Get().GetActiveCamera("PlayerCamera")))
	{
		pCamera->UpdateFollow(CGameInstance::Get().GetUnscaledDelta());
	}

	// PhysX render buffer와 무관하게 현재 게임오브젝트 Transform을 즉시 시각화한다.
	if(false)
	if (auto* pDbgLineRender = CGameInstance::Get().GetDbgLineRender())
	{
		const auto vPreviousColor = pDbgLineRender->GetColor();
		const auto ePreviousDepthMode = pDbgLineRender->GetDepthMode();
		const _float3 vPosition = GetTransform().GetPosition();

		pDbgLineRender->SetColor({ 1.f, 1.f, 1.f, 1.f });
		pDbgLineRender->SetDepthTest(true);
		pDbgLineRender->AddCapsule(
			0.5f,
			1.f,
			XMMatrixTranslation(vPosition.x, vPosition.y, vPosition.z));
		pDbgLineRender->AddCross(vPosition, 0.15f);

		pDbgLineRender->SetColor(vPreviousColor);
		pDbgLineRender->SetDepthMode(ePreviousDepthMode);
	}

	if (m_pBombardaController)
		m_pBombardaController->Update();

	if (m_pConfringoController)
		m_pConfringoController->Update(fTimeDelta);

	if (m_pAvadaKedavraController)
		m_pAvadaKedavraController->Update(fTimeDelta);
	UpdateAttachedEffects();

	const auto& pModel = m_pComModelInstance->GetModel();

	if (!pModel)
		return;
	DelayFinish(fTimeDelta);
	if (!pModel->GetAnimations().empty())
	{
		CGameInstance::Get().Add_Instance(m_pComModelInstance, m_pModelAnimator, *GetTransform().GetCombinedWorldMatrix());
		return;
	}

	/// 이펙트 위치 갱신
	if (m_pComSound)
		m_pComSound->Update();
	CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
}

void CPlayer::UpdateAttachedEffects()
{
	// Bone matrices are finalized by the animator before this LateUpdate path.
	// Updating Lumos here prevents the wand-tip glow from trailing by one frame.
	UpdateLumosLight();

	if (m_iDashBodyEffectID != INVALID_EFFECT_INSTANCE_ID)
	{
		CGameInstance::Get().SetEffectWorldMatrix(
			m_iDashBodyEffectID,
			*GetTransform().GetWorldMatrix());
	}
	

	// 캐릭터의 이동과 회전이 모두 확정된 LateUpdate 시점에 보호막을 붙인다.
	// PriorityUpdate에서 갱신하면 이전 프레임 위치를 따라가 외곽선이 떨릴 수 있다.
	if (m_bProtegoActive &&
		m_iProtegoShieldEffectID != INVALID_EFFECT_INSTANCE_ID)
	{
		const _float3 vPlayerPosition = GetTransform().GetPosition();
		_float4x4 shieldWorld{};
		XMStoreFloat4x4(&shieldWorld,
			XMMatrixScaling(1.2f, 1.2f, 1.2f) *
			XMMatrixTranslation(
				vPlayerPosition.x, vPlayerPosition.y + 1.f, vPlayerPosition.z));
		CGameInstance::Get().SetEffectWorldMatrix(
			m_iProtegoShieldEffectID, shieldWorld);
	}

	const _float3 vPlayerPosition = GetTransform().GetPosition();
	const _matrix playerTranslation = XMMatrixTranslation(
		vPlayerPosition.x, vPlayerPosition.y, vPlayerPosition.z);
	for (const auto& hitEffect : m_ProtegoHitEffects)
	{
		_float4x4 hitWorld{};
		XMStoreFloat4x4(
			&hitWorld,
			XMLoadFloat4x4(&hitEffect.matLocal) *
			playerTranslation);
		CGameInstance::Get().SetEffectWorldMatrix(
			hitEffect.iEffectID, hitWorld);
	}
}		

// CPU + GPU 버전
HRESULT CPlayer::Render_Instanced(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx, const E::MODEL_INSTANCE_BATCH& Batch)
{
	if (m_bRenderInfluence)
		return S_OK;

	if (!pContext || !m_pResVertexCPUSkinningInstancedShader || !m_pResPixelShader)
		return E_FAIL;

	const auto& vs = m_pResVertexCPUSkinningInstancedShader;
	const auto& ps = m_pResPixelShader;
	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	if (ctx.pass == RENDERPASS::DEFAULT) {
		pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);
	}
	else if (ctx.pass == RENDERPASS::DEPTH) {
		pContext->PSSetShader(nullptr, nullptr, 0);
	}

	const uint32_t iInstanceCount = static_cast<uint32_t>(Batch.Instances.size());
	if (iInstanceCount == 0 || iInstanceCount > 512 || Batch.CombinedBoneMatrices.size() != iInstanceCount)
		return E_FAIL;

	if (FAILED(Update_InstanceBuffer(pContext, Batch.Instances)))
		return E_FAIL;

	auto pModel = CGameInstance::Get().GetResourceFirst<CResModel>(Batch.Key.modelGroup, Batch.Key.modelTag);
	auto pCPUBonePaletteBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_CPU_BONEMATRIX");
	if (!pModel || !pCPUBonePaletteBuffer)
		return E_FAIL;

	_float4x4 identity{};
	XMStoreFloat4x4(&identity, XMMatrixIdentity());
	std::vector<_float4x4> combinedPalette(iInstanceCount * 512, identity);
	for (uint32_t instanceIndex = 0; instanceIndex < iInstanceCount; ++instanceIndex)
	{
		const auto& combinedMatrices = Batch.CombinedBoneMatrices[instanceIndex];
		if (combinedMatrices.empty() || combinedMatrices.size() > 512)
			return E_FAIL;

		// DirectXMath로 계산한 CPU Combined 행렬을 VS의 t7 행렬 규약에 맞춘다.
		// CPU 원본은 다른 CPU 기능에서도 사용하므로 업로드 복사본만 전치한다.
		for (uint32_t boneIndex = 0; boneIndex < static_cast<uint32_t>(combinedMatrices.size()); ++boneIndex)
		{
			XMStoreFloat4x4(
				&combinedPalette[instanceIndex * 512 + boneIndex],
				XMMatrixTranspose(
					XMLoadFloat4x4(&combinedMatrices[boneIndex])));
		}
	}

	// CPU가 계산한 CombinedBone palette는 batch당 한 번만 갱신한다.
	ID3D11ShaderResourceView* nullPaletteSRV = nullptr;
	pContext->VSSetShaderResources(7, 1, &nullPaletteSRV);
	if (FAILED(pCPUBonePaletteBuffer->UpdateData(
		combinedPalette.data(),
		static_cast<uint32_t>(combinedPalette.size() * sizeof(_float4x4)))))
		return E_FAIL;



	if (FAILED(Bind_InstanceBuffer(pContext)))
		return E_FAIL;
	ID3D11ShaderResourceView* cpuBonePaletteSRV = pCPUBonePaletteBuffer->GetSRV().Get();
	if (!cpuBonePaletteSRV)
		return E_FAIL;

	ID3D11ShaderResourceView* skinBonesSRV = pModel->Get_GPUSkinBoneSRV();
	if (!skinBonesSRV)
		return E_FAIL;

	pContext->VSSetShaderResources(7, 1, &cpuBonePaletteSRV);
	pContext->VSSetShaderResources(8, 1, &skinBonesSRV);

	for (uint32_t iMeshIndex = 0; iMeshIndex < pModel->Get_NumMeshes(); ++iMeshIndex)
	{
		const auto& mesh = pModel->GetMeshes()[iMeshIndex];
		if (!mesh)
			continue;

		

		const auto& skinRange = pModel->Get_GPUMeshSkinRange(iMeshIndex);
		if (skinRange.iSkinBoneCount == 0)
			return E_FAIL;

		E::GPU_SKIN_MESH_CONSTANTS skinningConstants{};
		skinningConstants.iSkinBoneOffset = skinRange.iSkinBoneOffset;
		skinningConstants.iVertexCount = mesh->GetNumVertices();
		skinningConstants.iSkinBoneCount = skinRange.iSkinBoneCount;
		skinningConstants.iMorphTargetCount = mesh->GetMorphTargetCount();
		skinningConstants.iMorphVertexCount = mesh->GetNumVertices();

		D3D11_MAPPED_SUBRESOURCE mapped{};
		if (FAILED(pContext->Map(m_pResSkinMeshCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
			return E_FAIL;
		memcpy(mapped.pData, &skinningConstants, sizeof(skinningConstants));
		pContext->Unmap(m_pResSkinMeshCBuffer->GetCBuffer().Get(), 0);
		ID3D11Buffer* skinningCB = m_pResSkinMeshCBuffer->GetCBuffer().Get();
		pContext->VSSetConstantBuffers(5, 1, &skinningCB);
		ID3D11ShaderResourceView* morphSRV = nullptr;
		if (const auto& morphBuffer = mesh->GetMorphDeltaBuffer())
			morphSRV = morphBuffer->GetSRV().Get();
		pContext->VSSetShaderResources(9, 1, &morphSRV);
		ID3D11Buffer* vertexBuffer = mesh->GetVertexBuffer().Get();
		const UINT stride = mesh->GetVertexStride();
		const UINT offset = 0;

		pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		pContext->IASetIndexBuffer(mesh->GetIndexBuffer().Get(), mesh->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(mesh->GetPrimitiveType());
		m_pComModelInstance->Bind_Textures(pContext, iMeshIndex);
		m_pComModelInstance->Bind_Materials(pContext, { 1.f, 1.f, 1.f }, 0.f, { 1.f, 0.f, 1.f }, 0.f, 1.f);
		pContext->DrawIndexedInstanced(mesh->GetNumIndices(), iInstanceCount, 0, 0, 0);
	}

	ID3D11ShaderResourceView* nullVSSRVs[4]{};
	pContext->VSSetShaderResources(6, 4, nullVSSRVs);

	return S_OK;
}

HRESULT CPlayer::Update_InstanceBuffer(ID3D11DeviceContext* pContext, const std::vector<GPU_ANIM_INSTANCE_DATA>& Instances)
{

	m_iCurrentInstanceCount = static_cast<uint32_t>(Instances.size());



	if (Instances.empty())
		return S_OK;

	constexpr uint32_t MAX_INSTANCE_COUNT = 512;

	if (m_iCurrentInstanceCount > MAX_INSTANCE_COUNT)
		return E_FAIL;

	auto pStructuredBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_ANIMAITON");

	if (!pStructuredBuffer)
		return E_FAIL;

	ID3D11Buffer* pBuffer = pStructuredBuffer->GetBuffer().Get();

	if (!pBuffer)
		return E_FAIL;

	ID3D11ShaderResourceView* pNullSRV = nullptr;


	pContext->VSSetShaderResources(6, 1, &pNullSRV);

	const size_t iCopySize = sizeof(GPU_ANIM_INSTANCE_DATA) * m_iCurrentInstanceCount;


	D3D11_BOX updateBox{};
	updateBox.left = 0;
	updateBox.right = static_cast<UINT>(iCopySize);
	updateBox.top = 0;
	updateBox.bottom = 1;
	updateBox.front = 0;
	updateBox.back = 1;

	pContext->UpdateSubresource(pBuffer, 0, &updateBox, Instances.data(), 0, 0);

	return S_OK;

}
HRESULT CPlayer::Bind_InstanceBuffer(ID3D11DeviceContext* pContext)
{

	auto pStructuredBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_ANIMAITON");

	if (!pStructuredBuffer)
		return E_FAIL;

	ID3D11ShaderResourceView* pSRV = pStructuredBuffer->GetSRV().Get();

	if (!pSRV)
		return E_FAIL;

	// VS의 t6 슬롯에 InstanceData 연결
	pContext->VSSetShaderResources(6, 1, &pSRV);

	return S_OK;
}

HRESULT CPlayer::Hit_Player_HurtBox(CGameObject* pAttacker, const PX_ON_COLLISION_DATA& info)
{
	if (!pAttacker)
		return E_INVALIDARG;
	if (m_bProtegoActive)
	{
		_float3 vHitPosition = GetTransform().GetPosition();
		vHitPosition.y += 1.f;
		if (info.iContactCount > 0)
			vHitPosition = info.Contacts[0].vWorldPosition;
		const _float3 vAttackPosition = pAttacker->GetTransform().GetPosition();
		TriggerProtegoHit(vHitPosition, 0, &vAttackPosition);
		return S_OK;
	}
	if (m_bInvincible)
		return S_OK;

	if (info.iSelfShapeSubIndex == std::numeric_limits<uint32_t>::max())
	{
		return S_FALSE;
	}

	const auto ePlayerCollision =static_cast<PLAYER_COLLISIONS>(info.iSelfShapeSubIndex);

	switch (ePlayerCollision)
	{
	case PLAYER_COLLISIONS::CCT_CAPSULE:
		// 이동을 담당하는 CCT 충돌이므로 피격으로 처리하지 않는다.
		return S_FALSE;

	case PLAYER_COLLISIONS::PLAYER_SHAPE_HURTBOX:
	{
		_float3 vHitPosition{};
		_float3 vHitNormal{};
		if (info.iContactCount > 0)
		{
			vHitPosition = info.Contacts[0].vWorldPosition;
			vHitNormal = info.Contacts[0].vWorldNormal;
		}

		DEBUG_LOG_STR(
			std::string("[PX][Player] HurtBox Hit : ") +
			std::string{ pAttacker->GetObjectTag() } +
			", ContactCount=" +
			std::to_string(info.iContactCount) + "\n");

		// TODO: 공격자의 데미지, 넉백, 속성 정보를 받아 HP에 반영한다.
		// vHitPosition과 vHitNormal은 피격 이펙트/넉백 방향에 사용할 수 있다.
		(void)vHitPosition;
		(void)vHitNormal;

		if (m_pStateMachine)
			m_pStateMachine->RequestState(PLAYER_STATE::HIT);

		return S_OK;
	}

	case PLAYER_COLLISIONS::END:
	default:
		return S_FALSE;
	}
}

_bool CPlayer::OnQueryHit(CGameObject* pAttacker,const PX_OVERLAP_RESULT& tHit,int32_t iDamage,const _float3& vHitPosition)
{
	if (m_bProtegoActive)
	{
		const _float3 vAttackPosition = pAttacker
			? pAttacker->GetTransform().GetPosition()
			: vHitPosition;
		TriggerProtegoHit(vHitPosition, iDamage, &vAttackPosition);
		return false;
	}
	if (m_bInvincible)
		return false;

	if (!tHit.bHit ||
		tHit.pGameObject != this ||
		tHit.iShapeSubIndex != ETOUI(PLAYER_COLLISIONS::PLAYER_SHAPE_HURTBOX) ||
		iDamage <= 0 ||
		m_iHp <= 0)
	{
		return false;
	}

	const int32_t iAppliedDamage = std::min(iDamage, m_iHp);
	m_iHp -= iAppliedDamage;
	m_vLastHitPosition = vHitPosition;

	if (auto* pUIController =
		CGameInstance::Get().GetGameObjectByHandleT<CUIController>(m_UIHandle))
	{
		pUIController->AddHP(-static_cast<_float>(iAppliedDamage));
	}



	if (m_iHp <= 0)
		HandleDeath();
	else if (m_pStateMachine)
	{
		if (iDamage >= KNOCKDOWN_DAMAGE_THRESHOLD)
			RequestKnockdown(pAttacker ? pAttacker->GetTransform().GetPosition() : vHitPosition);
		else
			m_pStateMachine->RequestState(PLAYER_STATE::HIT);
	}

	return true;
}

_bool CPlayer::OnQueryHit(int32_t iDamage, const _float3& vHitPosition)
{
	if (iDamage <= 0 || m_iHp <= 0)
		return false;
	if (m_bProtegoActive)
	{
		TriggerProtegoHit(vHitPosition, iDamage);
		return false;
	}
	if (m_bInvincible)
		return false;

	const int32_t iAppliedDamage = std::min(iDamage, m_iHp);
	m_iHp -= iAppliedDamage;
	m_vLastHitPosition = vHitPosition;

	if (auto* pUIController =
		CGameInstance::Get().GetGameObjectByHandleT<CUIController>(m_UIHandle))
	{
		pUIController->AddHP(-static_cast<_float>(iAppliedDamage));
	}
	if (m_iHp <= 0)
		HandleDeath();
	else if (m_pStateMachine)
	{
		if (iDamage >= KNOCKDOWN_DAMAGE_THRESHOLD)
			RequestKnockdown(vHitPosition);
		else
			m_pStateMachine->RequestState(PLAYER_STATE::HIT);
	}
	return true;
}

_bool CPlayer::OnQueryHit(int32_t iDamage)
{
	if (iDamage <= 0 || m_iHp <= 0)
		return false;
	if (m_bProtegoActive)
	{
		// 위치 정보가 없는 레거시 공격은 현재 타깃 위치를 사용한다.
		// 타깃도 없으면 캐릭터 정면 공격으로 안전하게 처리한다.
		_vector vHit = GetTransform().GetState(STATE::POSITION) +
			XMVector3Normalize(GetTransform().GetState(STATE::LOOK)) * 2.48f;
		if (auto* pTarget = CGameInstance::Get().GetGameObjectByHandle(m_hAutoTarget))
			vHit = pTarget->GetTransform().GetState(STATE::POSITION);
		_float3 vHitPosition{};
		XMStoreFloat3(&vHitPosition, vHit);
		TriggerProtegoHit(vHitPosition, iDamage);
		return false;
	}
	if (m_bInvincible)
		return false;

	const int32_t iAppliedDamage = std::min(iDamage, m_iHp);
	m_iHp -= iAppliedDamage;
	

	if (auto* pUIController = CGameInstance::Get().GetGameObjectByHandleT<CUIController>(m_UIHandle))
	{
		pUIController->AddHP(-static_cast<_float>(iAppliedDamage));
	}
	if (m_iHp <= 0)
		HandleDeath();
	else if (m_pStateMachine)
	{
		if (iDamage >= KNOCKDOWN_DAMAGE_THRESHOLD)
		{
			_float3 vAttackPosition{};
			if (auto* pTarget = CGameInstance::Get().GetGameObjectByHandle(m_hAutoTarget))
				vAttackPosition = pTarget->GetTransform().GetPosition();
			else
			{
				_vector vFallback = GetTransform().GetState(STATE::POSITION) +
					XMVector3Normalize(GetTransform().GetState(STATE::LOOK)) * 2.f;
				XMStoreFloat3(&vAttackPosition, vFallback);
			}
			RequestKnockdown(vAttackPosition);
		}
		else
			m_pStateMachine->RequestState(PLAYER_STATE::HIT);
	}
	return true;
}

_bool CPlayer::RequestKnockdown(const _float3& vAttackPosition)
{
	if (!m_pStateMachine || m_iHp <= 0 || m_bInvincible || m_bProtegoActive)
		return false;

	m_vKnockdownAttackPosition = vAttackPosition;
	return m_pStateMachine->RequestState(PLAYER_STATE::KNOCKDOWN);
}

void CPlayer::TriggerProtegoHit(
	const _float3& vHitPosition, int32_t iDamage,
	const _float3* pAttackPosition)
{
	const _bool bHeavyReaction =
		iDamage >= PROTEGO_HEAVY_DAMAGE_THRESHOLD;
	CGameInstance::Get().EventPublish(FRequestPlayerCameraShake{
		.fIntensity = bHeavyReaction ? 0.55f : 0.3f,
		.fDuration = bHeavyReaction ? 0.3f : 0.18f,
		.fFrequency = bHeavyReaction ? 34.f : 28.f });

	_float3 vShieldCenter = GetTransform().GetPosition();
	vShieldCenter.y += 1.f;
	m_vLastProtegoAttackPosition = pAttackPosition
		? *pAttackPosition
		: vHitPosition;

	_vector vNormal = XMLoadFloat3(&vHitPosition) - XMLoadFloat3(&vShieldCenter);
	if (XMVectorGetX(XMVector3LengthSq(vNormal)) <= FLT_EPSILON)
		vNormal = XMVectorSet(0.f, 0.f, 1.f, 0.f);
	else
		vNormal = XMVector3Normalize(vNormal);

	// Sweep 접촉점, Overlap 투사체 중심 등 입력 의미가 달라도
	// 최종 충돌 위치는 보호막 구 표면으로 통일한다.
	constexpr _float PROTEGO_SHIELD_RADIUS = 2.5f;
	const _vector vShieldSurfacePosition =
		XMLoadFloat3(&vShieldCenter) + vNormal * PROTEGO_SHIELD_RADIUS;
	XMStoreFloat3(&m_vLastProtegoHitPosition, vShieldSurfacePosition);

	if (auto* pSoundManager = CGameInstance::Get().GetSoundManager())
	{
		pSoundManager->Play3D(
			"./Resources/SampleClient/Sound/Player/SkillEffect/Protego/Protego_Block.wav",
			SOUND_3D_DESC{
				.vPosition = m_vLastProtegoHitPosition,
				.fMinDistance = 2.f,
				.fMaxDistance = 80.f,
				.eRolloff = SOUND_3D_ROLLOFF::LINEAR
			},
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::SFX,
				.fVolume = 1.5f,
				.fPitch = 1.f,
				.iPriority = 86,
				.bLoop = false
			});
	}

	++m_iProtegoParrySequence;
	m_fParryCounterRemainTime = PARRY_COUNTER_WINDOW;
	m_bProtegoReactionRequested = true;
	m_bProtegoHeavyReaction = bHeavyReaction;
	if (!m_bProtegoHeavyReaction)
		m_fProtegoRecoilRemainTime = 0.f;
	if (m_pStateMachine)
	{
		m_pStateMachine->RequestState(PLAYER_STATE::STUPEFY_SKILL);
	}

	_vector vReferenceUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	if (fabsf(XMVectorGetX(XMVector3Dot(vNormal, vReferenceUp))) > 0.96f)
		vReferenceUp = XMVectorSet(1.f, 0.f, 0.f, 0.f);

	const _vector vBaseRight = XMVector3Normalize(XMVector3Cross(vReferenceUp, vNormal));
	const _vector vBaseUp = XMVector3Normalize(XMVector3Cross(vNormal, vBaseRight));

	// Rotate every impact around its surface normal. This keeps an asymmetric
	// burst texture from appearing in exactly the same orientation every time.
	static uint32_t s_iProtegoHitSequence = 0;
	const _float fSurfaceRotation =
		static_cast<_float>((s_iProtegoHitSequence++ * 137u) % 360u) * XM_PI / 180.f;
	const _float fCos = cosf(fSurfaceRotation);
	const _float fSin = sinf(fSurfaceRotation);
	const _vector vRight = vBaseRight * fCos + vBaseUp * fSin;
	const _vector vUp = vBaseUp * fCos - vBaseRight * fSin;
	const _vector vPosition = XMLoadFloat3(&vShieldCenter);

	_float4x4 hitWorld{};
	XMStoreFloat4x4(&hitWorld, XMMatrixIdentity());
	XMStoreFloat3(reinterpret_cast<_float3*>(&hitWorld._11), vRight);
	XMStoreFloat3(reinterpret_cast<_float3*>(&hitWorld._21), vUp);
	XMStoreFloat3(reinterpret_cast<_float3*>(&hitWorld._31), vNormal);
	XMStoreFloat3(reinterpret_cast<_float3*>(&hitWorld._41), vPosition);

	const _float3 vPlayerPosition = GetTransform().GetPosition();
	const _matrix playerTranslation = XMMatrixTranslation(
		vPlayerPosition.x, vPlayerPosition.y, vPlayerPosition.z);
	_float4x4 hitLocalMatrix{};
	XMStoreFloat4x4(
		&hitLocalMatrix,
		XMLoadFloat4x4(&hitWorld) * XMMatrixInverse(nullptr, playerTranslation));

	const EFFECT_INSTANCE_ID hitEffectID = CGameInstance::Get().PlayEffect(
		"Protego_Shield_Hit_Layered", hitWorld, XMVectorZero(),
		[this](EFFECT_INSTANCE_ID effectId, EFFECT_FINISH_REASON)
		{
			std::erase_if(m_ProtegoHitEffects,
				[effectId](const PROTEGO_HIT_EFFECT& hitEffect)
				{
					return hitEffect.iEffectID == effectId;
				});
		});

	if (hitEffectID != INVALID_EFFECT_INSTANCE_ID)
		m_ProtegoHitEffects.push_back({ hitEffectID, hitLocalMatrix });
}

_bool CPlayer::PlayUpperBodyAnimation(
	int32_t iAnimation, const _char* pRootBoneName,
	uint32_t iBlendDepth, _bool bLoop, _float fFadeDuration)
{
	if (!m_pModelAnimator || !pRootBoneName ||
		!m_pModelAnimator->Set_UpperBodyRootBone(pRootBoneName, iBlendDepth))
	{
		return false;
	}

	m_pModelAnimator->Play_UpperAnim(iAnimation, bLoop, fFadeDuration);
	return true;
}

void CPlayer::ActivateProtego(_float fDuration)
{
	m_bProtegoActive = true;
	m_fProtegoRemainTime = std::max(m_fProtegoRemainTime, fDuration);

	if (m_iProtegoShieldEffectID == INVALID_EFFECT_INSTANCE_ID)
	{
		const _float3 vPlayerPosition = GetTransform().GetPosition();
		_float4x4 shieldWorld{};
		XMStoreFloat4x4(&shieldWorld,
			XMMatrixScaling(1.5f, 1.5f, 1.5f) *
			XMMatrixTranslation(
				vPlayerPosition.x, vPlayerPosition.y + 1.3f, vPlayerPosition.z));
		m_iProtegoShieldEffectID = CGameInstance::Get().PlayEffect(
			"Protego_Shield", shieldWorld, XMVectorZero());
	}
}

_bool CPlayer::ConsumeParryCounter(_float3& outAttackPosition)
{
	// 패링 성공만으로는 소비하지 않는다. Q 유지로 스투피파이가 명시적으로
	// 요청된 경우에만 Attack State가 카운터 애니메이션을 가져간다.
	if (!m_bStupefyCounterRequested || m_fParryCounterRemainTime <= 0.f)
		return false;

	m_bStupefyCounterRequested = false;
	m_fParryCounterRemainTime = 0.f;
	outAttackPosition = m_vLastProtegoAttackPosition;
	return true;
}

_bool CPlayer::ConsumeProtegoReaction(
	_float3& outAttackPosition, _bool& outHeavyReaction)
{
	if (!m_bProtegoReactionRequested)
		return false;

	m_bProtegoReactionRequested = false;
	outAttackPosition = m_vLastProtegoAttackPosition;
	outHeavyReaction = m_bProtegoHeavyReaction;
	m_bProtegoHeavyReaction = false;
	return true;
}

void CPlayer::StartProtegoRecoil(const _float3& vHitPosition)
{
	_vector vPushDirection = GetTransform().GetState(STATE::POSITION) -
		XMLoadFloat3(&vHitPosition);
	vPushDirection = XMVectorSetY(vPushDirection, 0.f);
	if (XMVectorGetX(XMVector3LengthSq(vPushDirection)) <= FLT_EPSILON)
		vPushDirection = -XMVectorSetY(GetTransform().GetState(STATE::LOOK), 0.f);
	if (XMVectorGetX(XMVector3LengthSq(vPushDirection)) <= FLT_EPSILON)
		vPushDirection = XMVectorSet(0.f, 0.f, -1.f, 0.f);
	else
		vPushDirection = XMVector3Normalize(vPushDirection);

	XMStoreFloat3(&m_vProtegoRecoilDirection, vPushDirection);
	m_fProtegoRecoilRemainTime = PROTEGO_RECOIL_DURATION;
}

void CPlayer::HandleDeath()
{
	if (m_bDeathEventPublished)
		return;

	m_bDeathEventPublished = true;

	if (m_pRagdollController)
		m_pRagdollController->RequestFromCurrentMotion();

	CGameInstance::Get().EventPublish(FPlayerDied{ .hPlayer = GetHandle(), .fLevelBgmFadeDuration = 3.f });
}


void CPlayer::Attack_Magic_Bullet()
{



	auto* pWeapon = CGameInstance::Get().GetGameObjectByHandleT<CPlayer_Weapon>(m_Partes[ETOUI(PARTES::WEAPON)]);

	if (!pWeapon)
		return;
	if (m_pComSound)
	{
		static constexpr const char* BASIC_ATTACK_VOICES[] =
		{
			"./Resources/SampleClient/Sound/Player/Voice/Attack/Player_AttackVoice_01.wav",
			"./Resources/SampleClient/Sound/Player/Voice/Attack/Player_AttackVoice_02.wav",

		};

		const int iVoiceIndex = Engine::RandInt(
			0, static_cast<int>(std::size(BASIC_ATTACK_VOICES)) - 1);
		m_pComSound->PlaySlot2D(
			E::StringID{ "PLAYER_BASIC_ATTACK_VOICE" },
			BASIC_ATTACK_VOICES[iVoiceIndex],
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::VOICE,
				.fVolume = 0.12f,
				.fPitch = 1.05f,
				.iPriority = 80,
				.bLoop = false
			},
			SOUND_SLOT_PLAY_MODE::OVERLAP);

		static constexpr const char* BASIC_ATTACK_SOUNDS[] =
		{
			"./Resources/SampleClient/Sound/Player/SkillEffect/"
			"BasicAttack/BasicAttack_SpellShot_01.wav",

		};

		constexpr int SOUND_COUNT =
			static_cast<int>(std::size(BASIC_ATTACK_SOUNDS));

		const int iSoundIndex =
			Engine::RandInt(0, SOUND_COUNT - 1);

		m_pComSound->PlaySlot2D(
			E::StringID{ "PLAYER_BASIC_ATTACK" },
			BASIC_ATTACK_SOUNDS[iSoundIndex],
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::SFX,
				.fVolume = 0.15f,
				.fPitch = 1.f,
				.iPriority = 64,
				.bLoop = false
			},
			SOUND_SLOT_PLAY_MODE::OVERLAP);
	}
	// 무기 발사 위치
	const _float4x4 spawnWorld = pWeapon->GetSpawnWorldMatrix();

	CPlayer_Magic_Bullet::MAGIC_BULLET_DESC desc{};
	desc.vStartPosition = { spawnWorld._41, spawnWorld._42, spawnWorld._43 };
	
	auto* pTarget = CGameInstance::Get().GetGameObjectByHandleT<CMonster>(m_hAutoTarget);

	if (pTarget)
	{
		desc.vEndPosition = pTarget->GetHurtBoxPosition();
		// 타깃이 있으면 타깃을 향해 발사
	}
	else
	{
		// 타깃이 없으면 플레이어 전방 일정 거리로 발사
		const _vector start = XMLoadFloat3(&desc.vStartPosition);

		const _vector look = XMVector3Normalize(XMVectorSetY(GetTransform().GetState(STATE::LOOK), 0.f));

		XMStoreFloat3(&desc.vEndPosition, start + look * 20.f);
	}

	desc.fSpeed = 70.f;
	desc.fCurveHeight = 2.f;
	desc.iSampleCount = 10;

	CGameInstance::Get().AddGameObjectToLayer(m_LevelTag,PROTO_GAMEOBJECT::Prototype_GameObject_PlayerMagicBullet,"PlayerMagicBullet",&desc);

	{
		auto a = CGameInstance::Get().GetParticle("PlayerAttackTrail_CPU", "PlayerAttackTrail_CPU");
		if (a == nullptr) {
			return;
		}
		static_cast<CTrail_CPU*>(a)->Clear();
	}
}

_bool CPlayer::FireStupefyProjectile()
{
	auto* pWeapon = CGameInstance::Get().GetGameObjectByHandleT<CPlayer_Weapon>(
		m_Partes[ETOUI(PARTES::WEAPON)]);
	if (!pWeapon)
		return false;

	const _float4x4 spawnWorld = pWeapon->GetSpawnWorldMatrix();
	const _float3 vStartPosition{ spawnWorld._41, spawnWorld._42, spawnWorld._43 };
	_float3 vEndPosition{};
	if (auto* pTarget = CGameInstance::Get().GetGameObjectByHandleT<CMonster>(m_hAutoTarget))
	{
		vEndPosition = pTarget->GetHurtBoxPosition();
	}
	else
	{
		_vector vLook = XMVectorSetY(GetTransform().GetState(STATE::LOOK), 0.f);
		if (XMVectorGetX(XMVector3LengthSq(vLook)) <= FLT_EPSILON)
			vLook = XMVectorSet(0.f, 0.f, 1.f, 0.f);
		else
			vLook = XMVector3Normalize(vLook);
		XMStoreFloat3(&vEndPosition, XMLoadFloat3(&vStartPosition) + vLook * m_StupefyDebug.fRange);
	}

	CPlayer_Stupefy_Bullet::DESC Desc{};
	Desc.sObjectTag = "PlayerStupefyProjectile";
	Desc.vStartPosition = vStartPosition;
	Desc.vEndPosition = vEndPosition;
	Desc.hOwner = GetHandle();
	Desc.eSkillType = PLAYER_SKILL_TYPE::ATTACK;
	Desc.fSpeed = m_StupefyDebug.fSpeed;
	Desc.fLifeTime = m_StupefyDebug.fLifeTime;
	Desc.fRadius = m_StupefyDebug.fRadius;
	Desc.fCurveAmplitude = m_StupefyDebug.fCurveAmplitude;
	Desc.fCurveFrequency = m_StupefyDebug.fCurveFrequency;
	Desc.iPathSampleCount = static_cast<uint32_t>(m_StupefyDebug.iPathSampleCount);
	Desc.sProjectileEffectName = m_StupefyDebug.bCore ? "KMS_Stupefy_Core" : "";
	Desc.sTrailParticleQueue = m_StupefyDebug.bRibbonTrail ? "KMS_Stupefy_Trail" : "";
	Desc.sImpactEffectName = m_StupefyDebug.bImpact ? "KMS_Stupefy_Impact" : "";
	Desc.fTrailSpacing = m_StupefyDebug.fTrailSpacing;
	Desc.bDebugSphere = m_StupefyDebug.bDebugSphere;
	Desc.bDebugPath = m_StupefyDebug.bDebugPath;
	Desc.bEnableSounds = m_StupefyDebug.bSound;

	const auto hProjectile = CGameInstance::Get().AddGameObjectToLayer(
		m_LevelTag,
		PROTO_GAMEOBJECT::Prototype_GameObject_PlayerStupefyBullet,
		"PlayerStupefyProjectile",
		&Desc);
	if (!hProjectile)
		return false;
	m_hLastStupefyProjectile = *hProjectile;

	// [Stupefy Muzzle] 유성 핵이 응축되며 청백색과 보랏빛 별가루가 터지는 시작점.
	if (m_StupefyDebug.bMuzzle)
		CGameInstance::Get().Spawn("KMS_Stupefy_Muzzle_Queue.json", spawnWorld);
	return true;
}

void CPlayer::UpdateStupefyDebugGUI()
{
	if (!ImGui::CollapsingHeader("Stupefy Debug"))
		return;

	const _bool bProjectileAlive =
		CGameInstance::Get().GetGameObjectByHandle(m_hLastStupefyProjectile) != nullptr;
	ImGui::Text("Last Projectile: %s", bProjectileAlive ? "Alive" : "None / Finished");
	ImGui::Text("Target: %s", CGameInstance::Get().GetGameObjectByHandle(m_hAutoTarget)
		? "Locked Target" : "Forward Test Shot");

	if (ImGui::Button("Fire Stupefy Test"))
	{
		if (!FireStupefyProjectile())
			DEBUG_LOG("[Stupefy Debug] Test fire failed. Check weapon and projectile prototype.\n");
	}
	ImGui::SameLine();
	if (ImGui::Button("Reset Stupefy Values"))
		m_StupefyDebug = {};

	ImGui::Separator();
	ImGui::TextUnformatted("Layers");
	ImGui::Checkbox("Muzzle Flash", &m_StupefyDebug.bMuzzle);
	ImGui::Checkbox("Projectile Core", &m_StupefyDebug.bCore);
	ImGui::Checkbox("Ribbon Trail", &m_StupefyDebug.bRibbonTrail);
	ImGui::Checkbox("Impact Flash", &m_StupefyDebug.bImpact);
	ImGui::Checkbox("Debug Projectile Sphere", &m_StupefyDebug.bDebugSphere);
	ImGui::Checkbox("Debug Sweep Path", &m_StupefyDebug.bDebugPath);
	ImGui::TextDisabled("These options affect newly fired test projectiles.");
	ImGui::Checkbox("Projectile Sound", &m_StupefyDebug.bSound);

	ImGui::Separator();
	ImGui::TextUnformatted("Flight Tuning");
	ImGui::DragFloat("Speed", &m_StupefyDebug.fSpeed, 1.f, 1.f, 300.f, "%.1f");
	ImGui::DragFloat("Range", &m_StupefyDebug.fRange, 0.5f, 1.f, 100.f, "%.1f");
	ImGui::DragFloat("Life Time", &m_StupefyDebug.fLifeTime, 0.05f, 0.1f, 10.f, "%.2f");
	ImGui::DragFloat("Sweep Radius", &m_StupefyDebug.fRadius, 0.005f, 0.01f, 2.f, "%.3f");
	ImGui::DragFloat("Curve Amplitude", &m_StupefyDebug.fCurveAmplitude, 0.005f, 0.f, 2.f, "%.3f");
	ImGui::DragFloat("Curve Frequency", &m_StupefyDebug.fCurveFrequency, 0.05f, 0.f, 10.f, "%.2f");
	ImGui::DragFloat("Trail Spacing", &m_StupefyDebug.fTrailSpacing, 0.005f, 0.01f, 2.f, "%.3f");
	ImGui::DragInt("Path Samples", &m_StupefyDebug.iPathSampleCount, 1.f, 8, 256);

	ImGui::Separator();
	ImGui::TextUnformatted("Effect Data Names");
	ImGui::TextUnformatted("Muzzle : KMS_Stupefy_Muzzle_Queue.json");
	ImGui::TextUnformatted("Core   : KMS_Stupefy_Core");
	ImGui::TextUnformatted("Trail  : KMS_Stupefy_Trail (continuous mesh)");
	ImGui::TextUnformatted("Impact : KMS_Stupefy_Impact");
	ImGui::TextColored(ImVec4(0.52f, 0.86f, 1.f, 1.f), "Meteor Core: cyan-white");
	ImGui::TextColored(ImVec4(0.72f, 0.42f, 1.f, 1.f), "Galaxy Dust: blue-violet");
	ImGui::TextDisabled("Missing effect data is isolated per layer; disable that layer while testing.");
}

void CPlayer::UpdateAncientThrowTargetDebugGUI()
{
	if (!ImGui::Begin("Ancient Throw Target Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::End();
		return;
	}

	auto& gameInstance = CGameInstance::Get();
	auto* pEnemyTarget = gameInstance.GetGameObjectByHandle(m_hAutoTarget);
	const _bool bEnemyTargetValid = pEnemyTarget && !pEnemyTarget->GetPendingDestroy();
	ImGui::TextColored(
		bEnemyTargetValid ? ImVec4(0.25f, 1.f, 0.35f, 1.f) : ImVec4(1.f, 0.25f, 0.2f, 1.f),
		"Enemy Target: %s", bEnemyTargetValid ? "FOUND" : "NONE");
	if (bEnemyTargetValid)
	{
		const _float3 position = pEnemyTarget->GetTransform().GetPosition();
		ImGui::Text("Tag: %s", pEnemyTarget->GetObjectTag().data());
		ImGui::Text("Position: %.2f, %.2f, %.2f", position.x, position.y, position.z);
	}

	auto* pCamera = gameInstance.GetActiveCamera();
	uint32_t iTotalBarrels{};
	uint32_t iUsableBarrels{};
	uint32_t iVisibleBarrels{};
	CPropBarrel* pBestBarrel{};
	_float fBestAlignment = -FLT_MAX;

	_vector vCameraPosition{};
	_vector vCameraLook{};
	_matrix cameraView{};
	_matrix cameraProjection{};
	_bool bCameraValid{};
	if (pCamera)
	{
		vCameraPosition = pCamera->GetTransform().GetState(STATE::POSITION);
		vCameraLook = pCamera->GetTransform().GetState(STATE::LOOK);
		cameraView = pCamera->GetView();
		cameraProjection = pCamera->GetProj();
		if (XMVectorGetX(XMVector3LengthSq(vCameraLook)) > FLT_EPSILON)
		{
			vCameraLook = XMVector3Normalize(vCameraLook);
			bCameraValid = true;
		}
	}

	for (const auto& [_, handles] : gameInstance.GetGameObjectLayers())
	{
		for (const CHandle& handle : handles)
		{
			auto* pBarrel = gameInstance.GetGameObjectByHandleT<CPropBarrel>(handle);
			if (!pBarrel)
				continue;

			++iTotalBarrels;
			if (pBarrel->GetPendingDestroy() ||
				pBarrel->GetBarrelState() != CPropBarrel::BARREL_STATE::CREATED)
			{
				continue;
			}
			++iUsableBarrels;
			if (!bCameraValid)
				continue;

			_float3 position = pBarrel->GetTransform().GetPosition();
			position.y += 0.75f;
			if (!IsAncientThrowTargetInCameraView(
				position, cameraView, cameraProjection))
				continue;

			++iVisibleBarrels;
			_vector vToBarrel = XMLoadFloat3(&position) - vCameraPosition;
			if (XMVectorGetX(XMVector3LengthSq(vToBarrel)) <= FLT_EPSILON)
				continue;

			vToBarrel = XMVector3Normalize(vToBarrel);
			const _float fAlignment = XMVectorGetX(XMVector3Dot(vCameraLook, vToBarrel));
			if (fAlignment > fBestAlignment)
			{
				fBestAlignment = fAlignment;
				pBestBarrel = pBarrel;
			}
		}
	}

	ImGui::Separator();
	ImGui::Text("Camera: %s", bCameraValid ? "VALID" : "NONE / INVALID");
	ImGui::Text("CPropBarrel Total: %u", iTotalBarrels);
	ImGui::Text("CPropBarrel Usable: %u", iUsableBarrels);
	ImGui::Text("CPropBarrel In View: %u", iVisibleBarrels);
	ImGui::TextColored(
		pBestBarrel ? ImVec4(0.25f, 1.f, 0.35f, 1.f) : ImVec4(1.f, 0.25f, 0.2f, 1.f),
		"Throw Target: %s", pBestBarrel ? "FOUND" : "NONE");
	const auto hLiveTarget = FindAncientThrowTarget();
	ImGui::Text("Live Finder: %s", hLiveTarget ? "FOUND" : "NONE");
	ImGui::Text("Pending Handle: %s", m_hPendingAncientThrowTarget ? "SET" : "EMPTY");

	if (pBestBarrel)
	{
		const _float3 position = pBestBarrel->GetTransform().GetPosition();
		const _float3 playerPosition = GetTransform().GetPosition();
		const _float dx = position.x - playerPosition.x;
		const _float dy = position.y - playerPosition.y;
		const _float dz = position.z - playerPosition.z;
		const _float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
		ImGui::Text("Barrel Tag: %s", pBestBarrel->GetObjectTag().data());
		ImGui::Text("Barrel Position: %.2f, %.2f, %.2f", position.x, position.y, position.z);
		ImGui::Text("Distance (display only): %.2f", distance);
		ImGui::Text("Camera Alignment: %.3f", fBestAlignment);
	}

	ImGui::TextDisabled("Read-only diagnostics: this panel does not change targeting.");
	ImGui::End();
}

std::optional<CHandle> CPlayer::FindAncientThrowTarget() const
{
	auto& gameInstance = CGameInstance::Get();
	auto* pCamera = gameInstance.GetActiveCamera();
	if (!pCamera)
		return std::nullopt;

	const _vector vCameraPosition = pCamera->GetTransform().GetState(STATE::POSITION);
	_vector vCameraLook = pCamera->GetTransform().GetState(STATE::LOOK);
	const _matrix cameraView = pCamera->GetView();
	const _matrix cameraProjection = pCamera->GetProj();
	if (XMVectorGetX(XMVector3LengthSq(vCameraLook)) <= FLT_EPSILON)
		return std::nullopt;
	vCameraLook = XMVector3Normalize(vCameraLook);

	std::optional<CHandle> hBestTarget{};
	_float fBestAlignment = -FLT_MAX;
	for (const auto& [_, handles] : gameInstance.GetGameObjectLayers())
	{
		for (const CHandle& handle : handles)
		{
			auto* pBarrel = gameInstance.GetGameObjectByHandleT<CPropBarrel>(handle);
			if (!pBarrel || pBarrel->GetPendingDestroy() ||
				pBarrel->GetBarrelState() != CPropBarrel::BARREL_STATE::CREATED)
				continue;

			_float3 position = pBarrel->GetTransform().GetPosition();
			position.y += 0.75f;
			if (!IsAncientThrowTargetInCameraView(
				position, cameraView, cameraProjection))
				continue;

			_vector vToBarrel = XMLoadFloat3(&position) - vCameraPosition;
			if (XMVectorGetX(XMVector3LengthSq(vToBarrel)) <= FLT_EPSILON)
				continue;
			vToBarrel = XMVector3Normalize(vToBarrel);
			const _float alignment = XMVectorGetX(XMVector3Dot(vCameraLook, vToBarrel));
			if (alignment > fBestAlignment)
			{
				fBestAlignment = alignment;
				hBestTarget = handle;
			}
		}
	}
	return hBestTarget;
}

std::optional<CHandle> CPlayer::ConsumeAncientThrowTarget()
{
	auto target = m_hPendingAncientThrowTarget;
	m_hPendingAncientThrowTarget.reset();
	return target;
}

void CPlayer::OnWake()
{
}

void CPlayer::OnSleep()
{
	int x = 0;
}

void CPlayer::OnCollisionEnter(CGameObject* pObj, const PX_ON_COLLISION_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][Character] Collision Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");

	Hit_Player_HurtBox(pObj, info);
}

void CPlayer::OnCollisionExit(CGameObject* pObj, const PX_ON_COLLISION_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][Character] Collision Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

void CPlayer::OnTriggerEnter(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][Character] Trigger Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");

	if (!info.bSelfIsTrigger)
		return;

	const auto ePlayerCollision =
		static_cast<PLAYER_COLLISIONS>(info.iSelfShapeSubIndex);
	if (ePlayerCollision == PLAYER_COLLISIONS::PLAYER_LEFT_FOOT ||
		ePlayerCollision == PLAYER_COLLISIONS::PLAYER_RIGHT_FOOT)
	{
		PlayFootstepSound(ePlayerCollision);
	}
}

void CPlayer::OnTriggerExit(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][Character] Trigger Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

void CPlayer::PlayFootstepSound(PLAYER_COLLISIONS eFoot)
{
	if (!m_pComSound || !m_pComCharacterMotor || !m_pComMoveIntent ||
		m_fFootstepSoundCooldown > 0.f ||
		!m_pComCharacterMotor->IsGrounded() ||
		(eFoot != PLAYER_COLLISIONS::PLAYER_LEFT_FOOT &&
		 eFoot != PLAYER_COLLISIONS::PLAYER_RIGHT_FOOT))
		return;

	const auto& tMoveOutput = m_pComMoveIntent->GetOutput();
	if (!tMoveOutput.bMoveRequested ||
		tMoveOutput.fMoveSpeed <= std::numeric_limits<_float>::epsilon())
		return;

	const _float fPitch = 1.f;

	const CComPxCollider* pFootCollider =
		eFoot == PLAYER_COLLISIONS::PLAYER_LEFT_FOOT
		? static_cast<CComPxCollider*>(m_pComPxLeftFootCollider)
		: static_cast<CComPxCollider*>(m_pComPxRightFootCollider);
	_float3 vSoundPosition = GetTransform().GetPosition();
	if (pFootCollider)
	{
		const _float3 vLocalPosition = pFootCollider->GetLocalPosition();
		_vector vWorldPosition = XMVector3Rotate(
			XMLoadFloat3(&vLocalPosition),
			GetTransform().GetLoadedQuaternion());
		vWorldPosition += XMLoadFloat3(&vSoundPosition);
		XMStoreFloat3(&vSoundPosition, vWorldPosition);
	}

	const SOUND_ID iSoundID = m_pComSound->Play3D(
		"./Resources/SampleClient/Sound/Player/StepSound/Step_01.wav",
		SOUND_3D_DESC{
			.vPosition = vSoundPosition,
			.fMinDistance = 2.f,
			.fMaxDistance = 15.f,
			.eRolloff = SOUND_3D_ROLLOFF::LINEAR
		},
		SOUND_PLAY_DESC{
			.sBusID = SOUND_BUS::SFX,
			.fVolume = 0.25f,
			.fPitch = fPitch,
			.iPriority = 96,
			.bLoop = false
		});

	if (iSoundID != INVALID_SOUND_ID)
		m_fFootstepSoundCooldown = 0.12f;
}

/*----------- 광윤 추가 -----------*/
bool CPlayer::GetShadowBounds(BoundingBox& OutBounds) const
{
	if (nullptr == m_pComCharacterController)	return false;

	const auto PlayerPosition = m_pComCharacterController->GetPosition();
	OutBounds.Center = PlayerPosition;
	OutBounds.Extents = { 1.25f, 1.6f, 1.0f };

	return true;
}
/*---------------------------------*/

void CPlayer::DelayFinish(_float fTimeDelta)
{
	

	if(m_iHp <= 0 && m_fDelayTime != -1.f){
		m_fDelayTime += fTimeDelta;
		if (m_fDelayTime >= 3.18f )
		{
			//[LSY] 3.18초 후에 게임 종료 처리
			m_fDelayTime = -1.f;
			auto* pUIController = CGameInstance::Get().GetGameObjectByHandleT<CUIController>(m_UIHandle);

			pUIController->CreateDeathScene();
		}
	}
}

void CPlayer::SetFlyRequested(_bool bRequested)
{
	m_bFlyRequested = bRequested;
	if (bRequested && m_pStateMachine)
		m_pStateMachine->RequestState(PLAYER_STATE::FLY);
}

void CPlayer::SetBroomVisible(_bool bVisible)
{
	if (auto* pBroom = CGameInstance::Get().GetGameObjectByHandleT<CPlayer_Broom>(
		m_Partes[ETOUI(PARTES::BROOM)]))
	{
		pBroom->SetVisible(bVisible);
	}
}

void CPlayer::SetBroomMovementRatio(_float fRatio)
{
	if (auto* pBroom = CGameInstance::Get().GetGameObjectByHandleT<CPlayer_Broom>(
		m_Partes[ETOUI(PARTES::BROOM)]))
	{
		pBroom->SetMovementRatio(fRatio);
	}
}

void CPlayer::SetBroomBoostEffectRatio(_float fRatio)
{
	if (auto* pBroom = CGameInstance::Get().GetGameObjectByHandleT<CPlayer_Broom>(
		m_Partes[ETOUI(PARTES::BROOM)]))
	{
		pBroom->SetBoostEffectRatio(fRatio);
	}
}

_bool CPlayer::IsBroomVisible() const
{
	if (auto* pBroom = CGameInstance::Get().GetGameObjectByHandleT<CPlayer_Broom>(
		m_Partes[ETOUI(PARTES::BROOM)]))
	{
		return pBroom->IsVisible();
	}
	return false;
}

E::UPtr<CPlayer> CPlayer::Create()
{
	auto pInstance = E::ToUPtr(new CPlayer{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CPlayer");
		return nullptr;
	}
	return  pInstance;
}


E::UPtr<E::CPrototype> CPlayer::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CPlayer{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CPlayer");
		return nullptr;
	}

	return pInstance;
}

void CPlayer::Free()
{
	SetLumosActive(false);
	// [LSY] 컨트롤러가 플레이어 참조를 사용하므로 기반 오브젝트 해제 전에 정리한다.
	m_pBombardaController.reset();
	m_pConfringoController.reset();
	m_pAvadaKedavraController.reset();
	CAnimationObject::Free();
}
