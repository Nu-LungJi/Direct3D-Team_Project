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
#include "ComPxRigidBody.h"
#include "ComPxBoxCollider.h"
#include "ComPxSphereCollider.h"
#include "ComPxCapsuleCollider.h"
#include "ComPxCharacterController.h"
#include "ComCharacterMoveIntent.h"
#include "ComCharacterMotor.h"
#include "PlayerThirdPersonCamera.h"
#include "DbgLineRender.h"
#include "Player_StateMachine.h"
#include "Player_Locomotion_State.h"
#include "Player_Jump_State.h"
#include "Player_Roll_State.h"
#include "Player_Attack_State.h"
#include "Player_Hit_State.h"
#include "PlayerAnimationRatioGuard.h"
#include "Player_DashSkill_State.h"
#include "Player_AcientAttack_State.h"
#include "Player_AccioSkill_State.h"
#include "Player_DepulsoSkill_State.h"
#include "Player_DescendoSkill_State.h"
#include "Player_RevelioSkill_State.h"
#include "Player_Magic_Bullet.h"
#include "Player_Weapon.h"
#include "Trail_CPU.h"
#include "UIController.h"
#include "UIManager.h"
NS_USING(Client)

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

}

CPlayer::CPlayer()
	: CAnimationObject{}
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
	}

	{
		CComPxRigidBody::DESC Desc{};
		Desc.eType = CComPxRigidBody::TYPE::KINEMATIC;
		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxRigidBody, "ComHitboxRigidbody", &Desc, &m_pComPxRigidBody)))
		{
			return E_FAIL;
		};
	}

	{
		CComPxBoxCollider::DESC Desc{};
		Desc.pComPxRigidBody = m_pComPxRigidBody;
		Desc.pResBoxGeo = CResPhysXBoxGeometry::CreateAndLoad({ .vHalfExtents = {0.5f, 0.5f, 0.5f} });
		Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});
		Desc.iShapeSubIndex = ETOUI(PLAYER_SHAPE::TEST_BOX);
		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxBoxCollider, "ComPxBoxCollider", &Desc, &m_pComPxBoxCollider)))
		{
			return E_FAIL;
		};
	}

	{
		CComPxCharacterController::DESC Desc{};
		Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad(CResPhysXMaterial::DESC{});
		Desc.tFilter = pDesc->tFilter;
		Desc.vPosition = pDesc->vInitialPosition;
		Desc.iShapeSubIndex = ETOUI(PLAYER_SHAPE::CCT_CAPSULE);
		//Desc.fStepOffset = 0.f;
		//Desc.fSlopeLimit = 1.f;	
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
		Desc.fGravity = -9.81f;
		Desc.fJumpVelocity = 7.f;
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
			PLAYER_STATE::REVELIO_SKILL,
			CPlayer_RevelioSkill_State::Create()))
		{
			return E_FAIL;
		}

		if (!m_pStateMachine->SetInitialState(PLAYER_STATE::LOCOMOTION))
		{
			return E_FAIL;
		}
	}

	m_pComMoveIntent->RequestWarp(pDesc->vInitialPosition);

	CPlayer_Weapon::WEAPON_DESC WeaponDesc{};
	WeaponDesc.sObjectTag = "Weapon";
	WeaponDesc.LevelTag = pDesc->LevelTag.GetDbgStr();
	WeaponDesc.WeaponName = "PLAYER_WEAPON_RESROUCE";
	WeaponDesc.iBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("SKT_RightHandSocket");
	WeaponDesc.ParentHandle = GetHandle();

	
	auto Weapon = E::CGameInstance::Get().AddGameObjectToLayer(pDesc->LevelTag, PROTO_GAMEOBJECT::Prototype_GameObject_PlayerWeapon, "Weapon", &WeaponDesc);
	if (!Weapon.has_value())
	{
		MSG_BOX("Create Failed Weapon");
		return E_FAIL;
	}

	m_Partes[ETOUI(PARTES::WEAPON)] = Weapon.value();

	{
		{
			auto a = CGameInstance::Get().GetParticle("PlayerAttackTrail_CPU", "PlayerAttackTrail_CPU");
			static_cast<CTrail_CPU*>(a)->SetColor(_float4(1.f, 113/255.f, 113 / 255.f, 1.f));
			static_cast<CTrail_CPU*>(a)->SetEmissive(_float4(1.f, 44 / 255.f, 44 / 255.f, 5.f));
		}

		{
			auto a = CGameInstance::Get().GetParticle("PlayerDashTrail1_CPU", "PlayerDashTrail1_CPU");
			static_cast<CTrail_CPU*>(a)->SetColor(_float4(182 / 255.f, 1.f, 241 / 255.f, 140 / 255.f));
			static_cast<CTrail_CPU*>(a)->SetEmissive(_float4(182 / 255.f, 1.f, 241 / 255.f, 2.f));
		}
	}
	return S_OK;

}


void CPlayer::PriorityUpdate(E::_float fTimeDelta)
{
	if (m_pStateMachine)
		m_pStateMachine->PriorityUpdate(fTimeDelta);

	if (m_pStateMachine &&
		m_pStateMachine->GetCurrentState() == PLAYER_STATE::ACIENTATTACK_SKILL)
	{
		m_bRawMoveInput = false;
		m_bSprintRequested = false;
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
				? (m_bSprintRequested ? m_fSprintSpeed : m_fJogSpeed)
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

	if (CGameInstance::Get().KeyDown(DIK_R))
	{
		//m_pComMoveIntent->RequestWarp({ -6.f, -215.f, 156.f });
		m_pComMoveIntent->RequestWarp(m_vInitialPosition);
	}

	if (m_pStateMachine &&m_pComCharacterMotor &&m_pStateMachine->GetCurrentState() == PLAYER_STATE::LOCOMOTION &&m_pComCharacterMotor->IsGrounded() &&CGameInstance::Get().KeyDown(DIK_SPACE))
	{
		m_pStateMachine->RequestState(PLAYER_STATE::JUMP);
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
		{
			//CGameInstance::Get().GetPhysXManager()->RayCast()

		//if(false)
		//if (auto pPlayerCamera = CGameInstance::Get().GetCamera("FLY"))
		//{
		//	std::vector< PX_RAYCAST_RESULT> results{};
		//	const auto& [ori, dir] = pPlayerCamera->GetRay();

		//	//CGameInstance::Get().GetDbgLineRender()->AddRay()

		//	auto cachedCol = CGameInstance::Get().GetDbgLineRender()->GetColor();
		//	auto cachedDepth = CGameInstance::Get().GetDbgLineRender()->GetDepthMode();
		//	CGameInstance::Get().GetDbgLineRender()->SetColor({ 0.f, 1.f, 0.f, 1.f });
		//	CGameInstance::Get().GetDbgLineRender()->SetDepthTest(false);
		//	CGameInstance::Get().GetDbgLineRender()->AddRay(
		//		ori,
		//		dir,
		//		100.f);
		//	CGameInstance::Get().GetDbgLineRender()->SetColor(cachedCol);
		//	CGameInstance::Get().GetDbgLineRender()->SetDepthMode(cachedDepth);

		//	if (CGameInstance::Get().GetPhysXManager()->RayCastMultiple({ .vOrigin = ori, .vDirection = dir, .fMaxDistance = 100.f,
		//	.tFilter = {.hIgnoreGameObject = GetHandle() } }, results))
		//	{
		//		for (const auto& result : results)
		//		{
		//			const auto hit = result.vHitpos;
		//			const auto normalEnd = _float3{
		//				hit.x + result.vHitNormal.x,
		//				hit.y + result.vHitNormal.y,
		//				hit.z + result.vHitNormal.z
		//			};
		//			//CGameInstance::Get().GetDbgLineRender()->AddLine(
		//			//	hit,
		//			//	normalEnd,
		//			//	{ 0.f, 1.f, 0.f, 1.f });


		//		}
		//	}
		//}


		//if (auto pPlayerCamera = CGameInstance::Get().GetCamera("FLY"))
		//{
		//	const auto& [ori, dir] = pPlayerCamera->GetRay();
		//	auto cachedCol = CGameInstance::Get().GetDbgLineRender()->GetColor();
		//	auto cachedDepth = CGameInstance::Get().GetDbgLineRender()->GetDepthMode();
		//	CGameInstance::Get().GetDbgLineRender()->SetColor({ 0.f, 1.f, 0.f, 1.f });
		//	CGameInstance::Get().GetDbgLineRender()->SetDepthTest(false);
		//	CGameInstance::Get().GetDbgLineRender()->AddSphere(10.f, XMMatrixTranslation(ori.x, ori.y, ori.z));
		//	CGameInstance::Get().GetDbgLineRender()->SetColor(cachedCol);
		//	CGameInstance::Get().GetDbgLineRender()->SetDepthMode(cachedDepth);

		//	std::vector<PX_OVERLAP_RESULT> results{};
		//	if (CGameInstance::Get().GetPhysXManager()->OverlapMultiple(PX_OVERLAP_DESC{ .tGeometry = {.eType = PX_QUERY_GEOMETRY_TYPE::SPHERE, .fRadius = 10.f}, .tPose = {.vPosition = ori} }, results))
		//	{
		//		for (const auto& result : results)
		//		{
		//		}
		//	}
		//}
		}
		
		//  가까이 있는거 한번 더 감지 
			auto ori = m_pComTransform->GetPosition();
		if (false) {
			auto matrix = XMLoadFloat4x4(m_pComTransform->GetWorldMatrix());
			auto cachedCol = CGameInstance::Get().GetDbgLineRender()->GetColor();
			auto cachedDepth = CGameInstance::Get().GetDbgLineRender()->GetDepthMode();
			CGameInstance::Get().GetDbgLineRender()->SetColor({ 1.f, 1.f, 0.f, 1.f });
			CGameInstance::Get().GetDbgLineRender()->SetDepthTest(false);
			CGameInstance::Get().GetDbgLineRender()->AddSphere(25.f, matrix);
			CGameInstance::Get().GetDbgLineRender()->SetColor(cachedCol);
			CGameInstance::Get().GetDbgLineRender()->SetDepthMode(cachedDepth);
		}


		std::vector<PX_OVERLAP_RESULT> results{};
		if (CGameInstance::Get().GetPhysXManager()->OverlapMultiple(PX_OVERLAP_DESC{ .tGeometry = {.eType = PX_QUERY_GEOMETRY_TYPE::SPHERE, .fRadius = 25.f}, .tPose = {.vPosition = ori},.tFilter = {.iQueryMask = ETOUI(COLLISION_LAYER::ENEMY_BODY)} }, results))
		{
			const auto& result = results.front();
			m_hAutoTarget = result.pGameObject->GetHandle();
		}
	}
	else {
		CGameObject* pTarget = CGameInstance::Get().GetGameObjectByHandle(m_hAutoTarget);
		//  그냥 일상시 타깃 감지
		if (!pTarget) {
				auto ori = m_pComTransform->GetPosition();
			if (false) {
				auto matrix = XMLoadFloat4x4(m_pComTransform->GetWorldMatrix());
				auto cachedCol = CGameInstance::Get().GetDbgLineRender()->GetColor();
				auto cachedDepth = CGameInstance::Get().GetDbgLineRender()->GetDepthMode();
				CGameInstance::Get().GetDbgLineRender()->SetColor({ 0.f, 1.f, 0.f, 1.f });
				CGameInstance::Get().GetDbgLineRender()->SetDepthTest(false);
				CGameInstance::Get().GetDbgLineRender()->AddSphere(15.f, matrix);
				CGameInstance::Get().GetDbgLineRender()->SetColor(cachedCol);
				CGameInstance::Get().GetDbgLineRender()->SetDepthMode(cachedDepth);

			}

			std::vector<PX_OVERLAP_RESULT> results{};
			if (CGameInstance::Get().GetPhysXManager()->OverlapMultiple(PX_OVERLAP_DESC{ .tGeometry = {.eType = PX_QUERY_GEOMETRY_TYPE::SPHERE, .fRadius = 40.f}, .tPose = {.vPosition = ori},.tFilter = {.iQueryMask = ETOUI(COLLISION_LAYER::ENEMY_BODY)} }, results))
			{
				const auto& result = results.front();
				m_hAutoTarget = result.pGameObject->GetHandle();
			}
		}

		if (pTarget) {
			auto ori = m_pComTransform->GetPosition();
			if (false) {
				auto matrix = XMLoadFloat4x4(m_pComTransform->GetWorldMatrix());

				auto cachedCol = CGameInstance::Get().GetDbgLineRender()->GetColor();
				auto cachedDepth = CGameInstance::Get().GetDbgLineRender()->GetDepthMode();
				CGameInstance::Get().GetDbgLineRender()->SetColor({ 1.f, 0.f, 0.f, 1.f });
				CGameInstance::Get().GetDbgLineRender()->SetDepthTest(false);
				CGameInstance::Get().GetDbgLineRender()->AddSphere(40.f, matrix);
				CGameInstance::Get().GetDbgLineRender()->SetColor(cachedCol);
				CGameInstance::Get().GetDbgLineRender()->SetDepthMode(cachedDepth);
			}


			std::vector<PX_OVERLAP_RESULT> results{};

			const bool bOverlapped =CGameInstance::Get().GetPhysXManager()->OverlapMultiple(PX_OVERLAP_DESC{.tGeometry = {.eType = PX_QUERY_GEOMETRY_TYPE::SPHERE,.fRadius = 40.f},.tPose = {.vPosition = ori},.tFilter = {.iQueryMask =ETOUI(COLLISION_LAYER::ENEMY_BODY)}},results);

			const bool bTargetStillInRange =bOverlapped &&std::ranges::any_of(results,[this](const PX_OVERLAP_RESULT& result){return result.pGameObject &&result.pGameObject->GetHandle() == m_hAutoTarget;});

			if (!bTargetStillInRange)
			{
				m_hAutoTarget = CHandle{};
			}
		}
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
		m_pStateMachine->RequestState(PLAYER_STATE::ACIENTATTACK_SKILL);
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



	if (CGameInstance::Get().KeyDown(DIK_1) && !m_bCoolTime_Num1) {
		//if (TryUseSkillSlot(1))
		if(m_pStateMachine->RequestState(PLAYER_STATE::ACCIO_SKILL))
			m_bCoolTime_Num1 = true;
	}

	if (CGameInstance::Get().KeyDown(DIK_2) && !m_bCoolTime_Num2)
	{
		//if (TryUseSkillSlot(2))
		if(m_pStateMachine->RequestState(PLAYER_STATE::DEPULSO_SKILL))
			m_bCoolTime_Num2 = true;
	}
	if (CGameInstance::Get().KeyDown(DIK_3) && !m_bCoolTime_Num3)
	{
		//if (TryUseSkillSlot(3))
		if(m_pStateMachine->RequestState(PLAYER_STATE::DESCENDO_SKILL))
			m_bCoolTime_Num3 = true;
	}	

	if (CGameInstance::Get().KeyDown(DIK_4) && !m_bCoolTime_Num4) {
		//if (TryUseSkillSlot(4))
		//m_pStateMachine->RequestState(PLAYER_STATE::);
		m_bCoolTime_Num4 = true;
	}

#ifdef _DEBUG
	if (m_pStateMachine && CGameInstance::Get().KeyDown(DIK_H))
		m_pStateMachine->RequestState(PLAYER_STATE::HIT);
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
	pUIController->SetSpellType(1, ETOUI(SPELL_TYPE::ASSIO));
	pUIController->SetSpellType(2, ETOUI(SPELL_TYPE::DEPULSO));
	pUIController->SetSpellType(3, ETOUI(SPELL_TYPE::DESENDO));

	// SPELL_TYPE에 REVELIO가 추가되기 전까지 4번 슬롯은 빈 슬롯으로 둔다.
	pUIController->SetSpellType(4, ETOUI(SPELL_TYPE::NONE));

	m_bSkillSlotUIInitialized = true;
}

_bool CPlayer::TryUseSkillSlot(uint32_t iSlotNumber)
{
	auto* pUIController =
		CGameInstance::Get().GetGameObjectByHandleT<CUIController>(m_UIHandle);
	if (!pUIController || !m_pStateMachine)
		return false;

	PLAYER_STATE eSkillState = PLAYER_STATE::NONE;
	switch (static_cast<SPELL_TYPE>(pUIController->GetSpellType(iSlotNumber)))
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

	default:
		// 플레이어에 구현되지 않았거나 비어 있는 스킬 슬롯이다.
		return false;
	}

	if (!m_pStateMachine->RequestState(eSkillState))
		return false;

	pUIController->UseSpell(iSlotNumber);
	return true;
}

void CPlayer::FixedUpdate(_float fTimeDelta)
{
	if (m_bMovementLocked)
	{
		m_pComMoveIntent->ClearMoveIntent();

		_float3 vVelocity = m_pComCharacterMotor->GetVelocity();
		vVelocity.x = 0.f;
		vVelocity.z = 0.f;
		m_pComCharacterMotor->SetVelocity(vVelocity);
	}

	ApplyGroundFollow(fTimeDelta);
	m_pComCharacterMotor->FixedUpdate(fTimeDelta);

	m_pComPxRigidBody->SetKinematicTarget(m_pComCharacterController->GetPosition(), GetTransform().GetQuaternion());

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
	const _float3 vPredictedFootPosition{
		vFootPosition.x +
			tMoveOutput.vMoveDirection.x *
			tMoveOutput.fMoveSpeed *
			fFixedTimeDelta * 5,
		vFootPosition.y,
		vFootPosition.z +
			tMoveOutput.vMoveDirection.z *
			tMoveOutput.fMoveSpeed *
			fFixedTimeDelta * 5
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

	PX_SWEEP_RESULT tGroundHit{};
	if (!pPhysXManager->Sweep(tSweepDesc, tGroundHit) ||
		!tGroundHit.bHit)
	{
		return;
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

	const _float fSlopeLimit =
		m_pComCharacterController->GetSlopeLimit();
	if (tGroundHit.vHitNormal.y < fSlopeLimit)
		return;

	const _float fStepDown =
		tGroundHit.vHitpos.y - vFootPosition.y;
	if (fStepDown < 0.f &&
		fStepDown >= -m_fGroundFollowMaxStepDown)
	{
		m_pComMoveIntent->AddExternalDisplacement(
			{ 0.f, fStepDown, 0.f });
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
				DEBUG_LOG_STR(sLog);
			}
			else
			{
				DEBUG_LOG("[CPlayer][CCT] Standing handle is no longer valid.\n");
			}
		}
		else
		{
			DEBUG_LOG("[CPlayer][CCT] Standing On: None\n");
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
		? (m_bSprintRequested ? m_fSprintSpeed : m_fJogSpeed)
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


	if (nullptr == CGameInstance::Get().GetGameObjectByHandleT<CUIController>(m_UIHandle))
	{
		m_bSkillSlotUIInitialized = false;

		const auto hUIController = GET_SINGLE(UIManager)->GetUIController();
		if (hUIController.has_value())
			m_UIHandle = *hUIController;
	}
	InitializeSkillSlotUI();

	_bool bApplyRootMotionTranslation{};
	_float3 vRootMotionDelta{};

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

	if (m_pComModelInstance->GetModel()->GetAnimations().size() != 0) {

		m_pModelAnimator->Update(fTimeDelta);
		bApplyRootMotionTranslation = m_bRootMotionTranslationActive;
		vRootMotionDelta = m_pModelAnimator->GetRootMotionDelta();

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

	// Animator를 먼저 진행해야 Locomotion State가 현재 프레임의
	// 재생 비율과 종료 상태로 Turn 회전을 맞출 수 있다.
	if (m_pStateMachine)
		m_pStateMachine->Update(fTimeDelta);

	// Turn 시작 당시 활성 상태를 보관했기 때문에 종료 프레임의
	// 마지막 RootMotionDelta도 빠뜨리지 않고 적용한다.
	if (bApplyRootMotionTranslation && m_pComMoveIntent)
	{
		const _vector vLocalDelta = XMLoadFloat3(&vRootMotionDelta);
		const _vector vWorldDelta = XMVector3Rotate(
			vLocalDelta,
			GetTransform().GetLoadedQuaternion());

		_float3 vWorldDisplacement{};
		XMStoreFloat3(&vWorldDisplacement, vWorldDelta);

		m_pComMoveIntent->AddExternalDisplacement(vWorldDisplacement);
	}

}

void CPlayer::LateUpdate(E::_float fTimeDelta)
{
	if (m_pStateMachine)
		m_pStateMachine->LateUpdate(fTimeDelta);


	//m_pComPhysX->UpdateSyncedDataToTransform(m_pComTransform);
	GetTransform().Update();

	// 플레이어 Transform을 먼저 확정한 뒤 같은 프레임의 카메라 View를 갱신한다.


	if (auto* pCamera = Cast<CPlayerThirdPersonCamera>(CGameInstance::Get().GetActiveCamera("PlayerCamera")))
	{
		pCamera->UpdateFollow();
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
	UpdateAttachedEffects();

	const auto& pModel = m_pComModelInstance->GetModel();

	if (!pModel)
		return;

	if (!pModel->GetAnimations().empty())
	{
		CGameInstance::Get().Add_Instance(m_pComModelInstance, m_pModelAnimator, *GetTransform().GetCombinedWorldMatrix());
		return;
	}

	/// 이펙트 위치 갱신

	CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
}

void CPlayer::UpdateAttachedEffects()
{
	if (m_iDashBodyEffectID == INVALID_EFFECT_INSTANCE_ID)
		return;

	auto a = GetTransform().GetWorldMatrix();
	CGameInstance::Get().SetEffectWorldMatrix(
		m_iDashBodyEffectID,
		*GetTransform().GetWorldMatrix());
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
	pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

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
		D3D11_MAPPED_SUBRESOURCE mapped{};
		if (FAILED(pContext->Map(m_pResSkinMeshCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
			return E_FAIL;
		memcpy(mapped.pData, &skinningConstants, sizeof(skinningConstants));
		pContext->Unmap(m_pResSkinMeshCBuffer->GetCBuffer().Get(), 0);
		ID3D11Buffer* skinningCB = m_pResSkinMeshCBuffer->GetCBuffer().Get();
		pContext->VSSetConstantBuffers(5, 1, &skinningCB);
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

	ID3D11ShaderResourceView* nullVSSRVs[3]{};
	pContext->VSSetShaderResources(6, 3, nullVSSRVs);

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


void CPlayer::Attack_Magic_Bullet()
{



	auto* pWeapon = CGameInstance::Get().GetGameObjectByHandleT<CPlayer_Weapon>(m_Partes[ETOUI(PARTES::WEAPON)]);

	if (!pWeapon)
		return;

	// 무기 발사 위치
	const _float4x4 spawnWorld = pWeapon->GetSpawnWorldMatrix();

	CPlayer_Magic_Bullet::MAGIC_BULLET_DESC desc{};
	desc.vStartPosition = { spawnWorld._41, spawnWorld._42, spawnWorld._43 };
	desc.hOwner = GetHandle();

	auto* pTarget = CGameInstance::Get().GetGameObjectByHandle(m_hAutoTarget);

	if (pTarget)
	{
		// 타깃이 있으면 타깃을 향해 발사
		XMStoreFloat3(&desc.vEndPosition, pTarget->GetTransform().GetState(STATE::POSITION));
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
}

void CPlayer::OnTriggerExit(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][Character] Trigger Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
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
