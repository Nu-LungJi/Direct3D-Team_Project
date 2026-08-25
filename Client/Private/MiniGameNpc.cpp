#include "pch.h"
#include "MiniGameNpc.h"
#include "ComAnimator.h"
#include "ComConstantBuffer.h"
#include "ComModelInstance.h"
#include "ComCollider.h"
#include "ComPxRigidBody.h"
#include "ComPxSphereCollider.h"
#include "ComPxCharacterController.h"
#include "ComCharacterMoveIntent.h"
#include "ComCharacterMotor.h"
#include "Resources.h"
#include "Client_Resources.h"
#include "GameInstance.h"
#include "UIController.h"
#include "SpellMiniGame.h"
#include "UIManager.h"
#include "Player.h"
#include "CinematicAsset.h"
#include "CinematicTypes.h"

NS_USING(Client)

namespace
{
	const StringID MINIGAME_NPC_TIME_SCALE_TAG{ "MiniGameNpc_WorldPause" };
	const StringID MINIGAME_NPC_DIALOGUE_CINEMATIC{ "MiniGameNpcDialogue" };
}

CMiniGameNpc::~CMiniGameNpc()
{
	SyncInteractionPrompt(false);
	if (m_hDialogueFade)
		GET_SINGLE(UIManager)->DeleteUIRecursive(*m_hDialogueFade);
	EndMiniGameWorldPause();
}

HRESULT CMiniGameNpc::InitializePrototype(void* pArg)
{
	return __super::InitializePrototype(pArg);
}

HRESULT CMiniGameNpc::Initialize(void* pArg)
{
	if (!pArg)
		return E_INVALIDARG;

	auto* pDesc = static_cast<DESC*>(pArg);
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	m_iHp = m_iMaxHp = 3555;
	{
		CComPxRigidBody::DESC Desc{};
		Desc.eType = CComPxRigidBody::TYPE::KINEMATIC;
		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::PHYSX,
			ES_EngineProtoPhysXComponent::Prototype_Component_ComPxRigidBody,
			"ComPxRigidBody", &Desc, &m_pComRigidBody)))
			return E_FAIL;
	}
	{
		CComPxSphereCollider::DESC Desc{};
		Desc.pComPxRigidBody = m_pComRigidBody;
		Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});
		Desc.bIsTrigger = false;
		Desc.tFilter = PX_FILTER_DESC{
			.iLayer = ETOUI(COLLISION_LAYER::ENEMY_HURTBOX),
			.iSimulationMask = ETOUI(COLLISION_LAYER::PLAYER_PROJECTILE) };
		Desc.pResSphereGeo = CResPhysXSphereGeometry::CreateAndLoad({ .fRadius = 1.2f });
		if (!Desc.pResMaterial || FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PHYSX,
			ES_EngineProtoPhysXComponent::Prototype_Component_ComPxSphereCollider,
			"ComPxSphereCollider", &Desc, &m_pComSphereCol)))
			return E_FAIL;
		if (!m_pComSphereCol->SetQueryEnabled(false))
			return E_FAIL;
	}
	{
		CComPxCharacterController::DESC Desc{};
		Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});
		const _float horizontalScale =
			std::max(std::abs(pDesc->vScale.x), std::abs(pDesc->vScale.z));
		const _float verticalScale = std::abs(pDesc->vScale.y);
		const _float3 centerOffset{
			pDesc->vCCTCenterOffset.x * pDesc->vScale.x,
			pDesc->vCCTCenterOffset.y * verticalScale,
			pDesc->vCCTCenterOffset.z * pDesc->vScale.z };
		Desc.fHeight = pDesc->fCCTHeight * verticalScale;
		Desc.fRadius = pDesc->fCCTRadius * horizontalScale;
		Desc.fStepOffset = pDesc->fCCTStepOffset;
		Desc.vPosition = {
			pDesc->vPos.x + centerOffset.x,
			pDesc->vPos.y + centerOffset.y,
			pDesc->vPos.z + centerOffset.z };
		Desc.tFilter = pDesc->tFilter;
		if (!Desc.pResMaterial || FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PHYSX,
			ES_EngineProtoPhysXComponent::Prototype_Component_ComPxCharacterController,
			"ComPxCharacterController", &Desc, &m_pCharacterController)))
			return E_FAIL;
	}
	{
		CComCharacterMoveIntent::DESC Desc{};
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoComponent::Prototype_Component_ComCharacterMoveIntent,
			"ComCharacterMoveIntent", &Desc, &m_pMoveIntent)))
			return E_FAIL;
	}
	{
		CComCharacterMotor::DESC Desc{};
		Desc.pMoveIntent = m_pMoveIntent;
		Desc.pCharacterController = m_pCharacterController;
		Desc.fGravity = -9.81f;
		Desc.vControllerCenterOffset = {
			pDesc->vCCTCenterOffset.x * pDesc->vScale.x,
			pDesc->vCCTCenterOffset.y * std::abs(pDesc->vScale.y),
			pDesc->vCCTCenterOffset.z * pDesc->vScale.z };
		Desc.bUseGravity = true;
		Desc.bSyncTransform = true;
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoComponent::Prototype_Component_ComCharacterMotor,
			"ComCharacterMotor", &Desc, &m_pCharacterMotor)))
			return E_FAIL;
	}
	{
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto(
			"PERMANENT", "Prototype_Component_ConstantBuffer",
			"ComCBufferPerObject", &Desc, &m_pComCBufferPerObject)))
			return E_FAIL;
	}
	{
		CComModelInstance::DESC Desc{};
		Desc.sGroupTag = pDesc->LevelTag;
		Desc.sResTag = pDesc->ReSourceTag;
		if (FAILED(AddComponentFromProto(
			"PERMANENT", "Prototype_Component_ModelInstance",
			"ComCModelIntance", &Desc, &m_pComModelInstance)))
			return E_FAIL;
	}
	{
		CComAnimator::DESC Desc{};
		Desc.sComTag = "ComCModelIntance";
		if (FAILED(AddComponentFromProto(
			"PERMANENT", "Prototype_Component_Animator",
			"ComCModelAnimator", &Desc, &m_pModelAnimator)))
			return E_FAIL;
	}
	{
		CComCollider::DESC Desc{};
		Desc.eCollType = CollType::Box;
		Desc.vExtents = { 1.f, 1.f, 1.f };
		if (FAILED(AddComponentFromProto(
			"COLLIDER", "Prototype_Component_Collider", "ComColl", &Desc, &m_pComCollider)))
			return E_FAIL;
	}

	const _matrix rotation =
		XMMatrixRotationX(XMConvertToRadians(pDesc->vRot.x)) *
		XMMatrixRotationY(XMConvertToRadians(pDesc->vRot.y)) *
		XMMatrixRotationZ(XMConvertToRadians(pDesc->vRot.z));
	GetTransform().SetQuaternion(XMQuaternionRotationMatrix(rotation));
	GetTransform().SetPosition(m_pCharacterController->GetFootPosition());
	GetTransform().Update();
	m_pModelAnimator->SetEvaluationMode(CComAnimator::EVALUATION_MODE::CPU_GPU);
	m_pModelAnimator->Build_BoneMatrices_CPU(0.f);
	m_pComSphereCol->SetQueryEnabled(true);
	m_eState = STATE::IDLE;

	m_SpeakerName = pDesc->SpeakerName;
	m_Dialogue = pDesc->Dialogue;
	m_IdleExpressionAnim = pDesc->IdleExpressionAnim;
	m_fInteractionDistance = std::max(0.1f, pDesc->InteractionDistance);
	m_bSecondSpellMiniGame = pDesc->SecondSpellMiniGame;
	m_bRepeatable = pDesc->Repeatable;
	m_eOutcome = pDesc->Outcome;
	m_fFadeDuration = std::max(0.05f, pDesc->FadeDuration);
	m_fFadeHoldDuration = std::max(0.f, pDesc->FadeHoldDuration);
	m_vPlayerDialogueOffset = pDesc->PlayerDialogueOffset;
	m_vDialogueCameraOffset = pDesc->DialogueCameraOffset;
	m_fDialogueCameraFovY = pDesc->DialogueCameraFovY;
	m_vMoveDestination = pDesc->MoveDestination;
	m_fMoveSpeed = std::max(0.1f, pDesc->MoveSpeed);
	m_fMoveStopDistance = std::max(0.05f, pDesc->MoveStopDistance);
	m_hInteractionPlayer = pDesc->TargetHandle;
	ResolvePlayerHandle();
	SetExpression(m_IdleExpressionAnim, true);
	return S_OK;
}

void CMiniGameNpc::FixedUpdate(E::_float fTimeDelta)
{
	if (m_pCharacterMotor)
		m_pCharacterMotor->FixedUpdate(fTimeDelta);
}

void CMiniGameNpc::Update(E::_float fTimeDelta)
{
	__super::Update(fTimeDelta);
	UpdateDialogueIntro(fTimeDelta);
	UpdateMoveOutcome();
	UpdateMiniGameWorldPause();

	const _bool playerInRange = IsPlayerInRange();
	const _bool canStartDialogue = playerInRange &&
		(!m_bCompleted || m_bRepeatable) &&
		m_eState == STATE::IDLE;
	// 대화가 시작된 뒤에는 암전 중 위치 보정이나 물리 높이 차이 때문에
	// 거리 판정이 달라져도 다음 대사가 막히지 않아야 한다.
	const _bool canAdvanceDialogue = m_bTalking &&
		m_eState == STATE::TALKING;
	SyncInteractionPrompt(canStartDialogue || canAdvanceDialogue);

	if ((canStartDialogue || canAdvanceDialogue) &&
		E::CGameInstance::Get().KeyDown(DIK_F))
	{
		// UIManager도 같은 프레임에 F 프롬프트를 소비하므로 로컬 상태를 맞춘다.
		GET_SINGLE(UIManager)->RemoveActiveButton(GetHandle(), false);
		m_bPromptVisible = false;
		if (canAdvanceDialogue)
			AdvanceDialogue();
		else
			BeginDialogue();
	}
}

void CMiniGameNpc::BeginDialogue()
{
	if (m_bTalking || m_Dialogue.empty() || (m_bCompleted && !m_bRepeatable))
		return;

	m_bTalking = true;
	m_iDialogueIndex = 0u;
	SetPlayerMovementLocked(true);
	SyncInteractionPrompt(false);
	m_eConversationPhase = CONVERSATION_PHASE::FADING_OUT;
	m_eState = STATE::DIALOGUE_INTRO;
	m_fIntroElapsed = 0.f;
	// 현재 표시 중인 메인 HUD 상태를 보관한 뒤 대화 연출 동안 숨긴다.
	GET_SINGLE(UIManager)->PlayFadeOutAll2DUI(0.f, m_fFadeDuration);
	// 같은 BlackBG를 FadeIn/FadeOut 양쪽에서 사용해야 암전이 정상 해제된다.
	const auto fadeRoots = GET_SINGLE(UIManager)->LoadPrefab("BlackBG");
	if (!fadeRoots.empty())
	{
		m_hDialogueFade = fadeRoots.front();
		GET_SINGLE(UIManager)->PlayFadeIn(
			*m_hDialogueFade, 0.f, m_fFadeDuration);
	}
}

void CMiniGameNpc::UpdateDialogueIntro(_float fTimeDelta)
{
	if (m_eConversationPhase != CONVERSATION_PHASE::FADING_OUT &&
		m_eConversationPhase != CONVERSATION_PHASE::HOLDING_BLACK &&
		m_eConversationPhase != CONVERSATION_PHASE::FADING_IN)
		return;

	m_fIntroElapsed += fTimeDelta;
	if (m_eConversationPhase == CONVERSATION_PHASE::FADING_OUT)
	{
		if (m_fIntroElapsed < m_fFadeDuration)
			return;
		m_fIntroElapsed = 0.f;
		m_eConversationPhase = CONVERSATION_PHASE::HOLDING_BLACK;
		return;
	}

	if (m_eConversationPhase == CONVERSATION_PHASE::HOLDING_BLACK)
	{
		if (m_fIntroElapsed < m_fFadeHoldDuration)
			return;
		m_fIntroElapsed = 0.f;
		BeginDialogueCamera();
		if (m_hDialogueFade)
		{
			GET_SINGLE(UIManager)->PlayFadeOutDelete(
				*m_hDialogueFade, 0.f, m_fFadeDuration);
			m_hDialogueFade.reset();
		}
		m_eConversationPhase = CONVERSATION_PHASE::FADING_IN;
		return;
	}

	if (m_fIntroElapsed < m_fFadeDuration)
		return;
	ShowFirstDialogueLine();
}

void CMiniGameNpc::ShowFirstDialogueLine()
{
	m_fIntroElapsed = 0.f;
	m_eConversationPhase = CONVERSATION_PHASE::TALKING;
	m_eState = STATE::TALKING;
	const auto& line = m_Dialogue[m_iDialogueIndex];
	SetExpression(line.ExpressionAnim, line.LoopExpression);
	GET_SINGLE(UIManager)->AddDialoguePopup(m_SpeakerName, line.Text);
}

void CMiniGameNpc::AdvanceDialogue()
{
	if (!m_bTalking || m_eConversationPhase != CONVERSATION_PHASE::TALKING)
		return;

	++m_iDialogueIndex;
	if (m_iDialogueIndex >= m_Dialogue.size())
	{
		FinishDialogue();
		return;
	}

	const auto& line = m_Dialogue[m_iDialogueIndex];
	SetExpression(line.ExpressionAnim, line.LoopExpression);
	GET_SINGLE(UIManager)->AddDialoguePopup(m_SpeakerName, line.Text);
}

void CMiniGameNpc::CancelDialogue()
{
	m_bTalking = false;
	m_iDialogueIndex = 0u;
	SetExpression(m_IdleExpressionAnim, true);
	if (m_hDialogueFade)
	{
		GET_SINGLE(UIManager)->DeleteUIRecursive(*m_hDialogueFade);
		m_hDialogueFade.reset();
	}
	EndDialogueCamera();
	SetPlayerMovementLocked(false);
	GET_SINGLE(UIManager)->PlayFadeInAll2DUI(0.f, m_fFadeDuration);
	m_eConversationPhase = CONVERSATION_PHASE::IDLE;
	m_eState = STATE::IDLE;
}

void CMiniGameNpc::FinishDialogue()
{
	m_bTalking = false;
	m_bCompleted = true;
	SetExpression(m_IdleExpressionAnim, true);
	m_eConversationPhase = CONVERSATION_PHASE::IDLE;

	// 스펠 미니게임 동안에는 대화 구도와 플레이어 잠금을 그대로 유지한다.
	// 미니게임 생성에 실패한 경우에만 즉시 원래 화면으로 복구한다.
	if (m_eOutcome == OUTCOME::SPELL_MINIGAME)
	{
		ExecuteOutcome();
		if (m_eState == STATE::MINIGAME)
			return;
	}

	EndDialogueCamera();
	SetPlayerMovementLocked(false);
	GET_SINGLE(UIManager)->PlayFadeInAll2DUI(0.f, m_fFadeDuration);
	m_eState = STATE::IDLE;
	if (m_eOutcome != OUTCOME::SPELL_MINIGAME)
		ExecuteOutcome();
}

void CMiniGameNpc::BeginDialogueCamera()
{
	auto& gameInstance = E::CGameInstance::Get();
	auto* pPlayer = gameInstance.GetGameObjectByHandleT<CPlayer>(m_hInteractionPlayer);
	if (!pPlayer)
		return;

	const _vector up = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	const _vector npcPosition = GetTransform().GetState(E::STATE::POSITION);
	_vector npcRight = XMVectorSetY(GetTransform().GetState(E::STATE::RIGHT), 0.f);
	_vector npcLook = XMVectorSetY(GetTransform().GetState(E::STATE::LOOK), 0.f);
	if (XMVectorGetX(XMVector3LengthSq(npcRight)) <= FLT_EPSILON)
		npcRight = XMVectorSet(1.f, 0.f, 0.f, 0.f);
	else
		npcRight = XMVector3Normalize(npcRight);
	if (XMVectorGetX(XMVector3LengthSq(npcLook)) <= FLT_EPSILON)
		npcLook = XMVectorSet(0.f, 0.f, 1.f, 0.f);
	else
		npcLook = XMVector3Normalize(npcLook);

	// 암전된 동안 NPC 기준의 고정 대화 자리로 플레이어를 옮긴다.
	const _vector playerPosition = npcPosition +
		npcRight * m_vPlayerDialogueOffset.x +
		up * m_vPlayerDialogueOffset.y +
		npcLook * m_vPlayerDialogueOffset.z;
	_float3 dialoguePlayerPosition{};
	_float3 npcLookAt{};
	XMStoreFloat3(&dialoguePlayerPosition, playerPosition);
	XMStoreFloat3(&npcLookAt, npcPosition + up * 1.35f);
	pPlayer->SetDialoguePose(dialoguePlayerPosition, npcLookAt);

	const _vector target = up * 1.45f;
	const _vector localPlayerPosition =
		XMVectorSet(m_vPlayerDialogueOffset.x, m_vPlayerDialogueOffset.y,
			m_vPlayerDialogueOffset.z, 1.f);
	_vector localNpcToPlayer = XMVectorSetY(localPlayerPosition, 0.f);
	if (XMVectorGetX(XMVector3LengthSq(localNpcToPlayer)) <= FLT_EPSILON)
		localNpcToPlayer = XMVectorSet(0.f, 0.f, 1.f, 0.f);
	else
		localNpcToPlayer = XMVector3Normalize(localNpcToPlayer);
	const _vector localShoulderRight = XMVector3Normalize(
		XMVector3Cross(up, localNpcToPlayer));
	const _vector cameraPosition = localPlayerPosition +
		localShoulderRight * m_vDialogueCameraOffset.x +
		up * m_vDialogueCameraOffset.y +
		localNpcToPlayer * m_vDialogueCameraOffset.z;
	const _vector look = XMVector3Normalize(target - cameraPosition);
	const _vector right = XMVector3Normalize(XMVector3Cross(up, look));
	const _vector cameraUp = XMVector3Normalize(XMVector3Cross(look, right));
	_matrix cameraWorld = XMMatrixIdentity();
	cameraWorld.r[0] = right;
	cameraWorld.r[1] = cameraUp;
	cameraWorld.r[2] = look;
	_float3 position{};
	_float4 rotation{};
	XMStoreFloat3(&position, cameraPosition);
	XMStoreFloat4(&rotation, XMQuaternionRotationMatrix(cameraWorld));

	E::FCinematicAssetData data{};
	data.CinematicID = MINIGAME_NPC_DIALOGUE_CINEMATIC;
	data.CameraTrack.TrackID = StringID{ "MiniGameNpcDialogueTrack" };
	E::FCinematicCameraShot shot{};
	shot.ShotID = StringID{ "NpcOverShoulder" };
	shot.eCoordinateSpace = E::ECinematicCoordinateSpace::TargetLocal;
	shot.eBindingMode = E::ECinematicBindingMode::Live;
	E::FCinematicCameraKeyframe first{};
	first.vPosition = position;
	first.vRotation = rotation;
	first.fFovY = m_fDialogueCameraFovY;
	E::FCinematicCameraKeyframe last = first;
	last.fTime = 600.f;
	shot.Keyframes = { first, last };
	data.CameraTrack.Shots.push_back(std::move(shot));

	auto cinematic = E::CCinematicAsset::Create(data);
	if (!cinematic || FAILED(gameInstance.RegistCinematicAsset(cinematic)))
		return;

	E::FCinematicPlayOptions options{};
	options.eStartMode = E::ECinematicStartMode::Immediate;
	options.eReturnMode = E::ECinematicReturnMode::Immediate;
	m_bDialogueCinematicPlaying =
		gameInstance.PlayCinematic(
			MINIGAME_NPC_DIALOGUE_CINEMATIC, GetHandle(), options) == S_OK;
}

void CMiniGameNpc::EndDialogueCamera()
{
	if (!m_bDialogueCinematicPlaying)
		return;

	E::CGameInstance::Get().StopCinematic();
	m_bDialogueCinematicPlaying = false;
}

void CMiniGameNpc::SetPlayerMovementLocked(_bool locked)
{
	if (auto* pPlayer = E::CGameInstance::Get().GetGameObjectByHandleT<CPlayer>(m_hInteractionPlayer))
		pPlayer->SetMovementLocked(locked);
}

void CMiniGameNpc::ExecuteOutcome()
{
	if (m_eOutcome == OUTCOME::MOVE_TO_DESTINATION)
	{
		m_bMovingToDestination = true;
		m_eState = STATE::MOVING;
		return;
	}
	if (m_eOutcome != OUTCOME::SPELL_MINIGAME)
		return;

	const auto hController = GET_SINGLE(UIManager)->GetUIController();
	if (hController)
		if (auto* pController = E::CGameInstance::Get().GetGameObjectByHandleT<CUIController>(*hController))
			if (pController->StartSpellMiniGame(m_bSecondSpellMiniGame))
			{
				m_eState = STATE::MINIGAME;
				BeginMiniGameWorldPause();
			}
}

void CMiniGameNpc::BeginMiniGameWorldPause()
{
	E::TIME_SCALE_REQUEST_DESC Desc{};
	Desc.fTargetScale = 0.f;
	Desc.fBlendIn = 0.f;
	Desc.fMaxUnscaledDuration = 600.f;
	Desc.fSafetyBlendOut = 0.f;
	Desc.sTag = MINIGAME_NPC_TIME_SCALE_TAG;
	m_bOwnsWorldPause = E::CGameInstance::Get().BeginTimeScale(Desc);
}

void CMiniGameNpc::UpdateMiniGameWorldPause()
{
	if (m_eState != STATE::MINIGAME)
		return;
	if (IsSpellMiniGameRunning())
		return;
	EndMiniGameWorldPause();
	EndDialogueCamera();
	SetPlayerMovementLocked(false);
	GET_SINGLE(UIManager)->PlayFadeInAll2DUI(0.f, m_fFadeDuration);
	m_eState = STATE::IDLE;
}

void CMiniGameNpc::EndMiniGameWorldPause()
{
	if (!m_bOwnsWorldPause)
		return;
	auto& gameInstance = E::CGameInstance::Get();
	if (gameInstance.IsTimeScaleActive(MINIGAME_NPC_TIME_SCALE_TAG))
		gameInstance.EndTimeScale(MINIGAME_NPC_TIME_SCALE_TAG, 0.f);
	m_bOwnsWorldPause = false;
}

_bool CMiniGameNpc::IsSpellMiniGameRunning() const
{
	auto& gameInstance = E::CGameInstance::Get();
	for (const auto& [layerTag, handles] : gameInstance.GetGameObjectLayers())
		for (const CHandle& handle : handles)
			if (gameInstance.GetGameObjectByHandleT<CSpellMiniGame>(handle))
				return true;
	return false;
}

void CMiniGameNpc::UpdateMoveOutcome()
{
	if (!m_bMovingToDestination || !m_pMoveIntent)
		return;
	const _float3 position = GetTransform().GetPosition();
	const _vector delta = XMLoadFloat3(&m_vMoveDestination) - XMLoadFloat3(&position);
	const _float distance = XMVectorGetX(XMVector3Length(delta));
	if (distance <= m_fMoveStopDistance)
	{
		m_bMovingToDestination = false;
		m_eState = STATE::IDLE;
		return;
	}
	_float3 direction{};
	XMStoreFloat3(&direction, XMVector3Normalize(delta));
	m_pMoveIntent->SetMoveIntent(direction, m_fMoveSpeed);
	m_pMoveIntent->SetFacingIntent(direction, 180.f);
}

void CMiniGameNpc::SetExpression(const _string& animName, _bool loop)
{
	if (animName.empty() || !m_pModelAnimator)
		return;

	const int32_t animIndex = Find_AnimIndex(animName);
	if (animIndex >= 0)
		m_pModelAnimator->Play_Anim(animIndex, loop);
}

_bool CMiniGameNpc::IsPlayerInRange()
{
	ResolvePlayerHandle();
	auto* pPlayer = E::CGameInstance::Get().GetGameObjectByHandleT<CPlayer>(
		m_hInteractionPlayer);
	if (!pPlayer || pPlayer->GetPendingDestroy())
		return false;

	const _float3 npcPos = GetTransform().GetPosition();
	const _float3 playerPos = pPlayer->GetTransform().GetPosition();
	const _float dx = npcPos.x - playerPos.x;
	const _float dy = npcPos.y - playerPos.y;
	const _float dz = npcPos.z - playerPos.z;
	return dx * dx + dy * dy + dz * dz <=
		m_fInteractionDistance * m_fInteractionDistance;
}

void CMiniGameNpc::ResolvePlayerHandle()
{
	auto& gameInstance = E::CGameInstance::Get();
	if (gameInstance.GetGameObjectByHandleT<CPlayer>(m_hInteractionPlayer))
		return;

	m_hInteractionPlayer = {};
	for (const auto& [layerTag, handles] : gameInstance.GetGameObjectLayers())
	{
		for (const CHandle& handle : handles)
		{
			auto* pPlayer = gameInstance.GetGameObjectByHandleT<CPlayer>(handle);
			if (!pPlayer || pPlayer->GetPendingDestroy())
				continue;

			m_hInteractionPlayer = handle;
			return;
		}
	}
}

void CMiniGameNpc::SyncInteractionPrompt(_bool show)
{
	if (m_bPromptVisible == show)
		return;

	m_bPromptVisible = show;
	if (show)
		GET_SINGLE(UIManager)->CreateActiveButton(GetHandle(), DIK_F);
	else
		GET_SINGLE(UIManager)->RemoveActiveButton(GetHandle());
}

E::UPtr<CMiniGameNpc> CMiniGameNpc::Create()
{
	auto pInstance = E::ToUPtr(new CMiniGameNpc{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CMiniGameNpc");
		return nullptr;
	}
	return pInstance;
}

E::UPtr<E::CPrototype> CMiniGameNpc::Clone(void* pArg)
{
	auto pInstance = E::ToUPtr(new CMiniGameNpc{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CMiniGameNpc");
		return nullptr;
	}
	return pInstance;
}
