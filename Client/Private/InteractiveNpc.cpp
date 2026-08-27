#include "pch.h"
#include "InteractiveNpc.h"
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
#include "AccioActivity_Base.h"
#include "UIManager.h"
#include "Player.h"
#include "CinematicTypes.h"

NS_USING(Client)

namespace
{
	const StringID MINIGAME_NPC_TIME_SCALE_TAG{ "MiniGameNpc_WorldPause" };
}


CInteractiveNpc::~CInteractiveNpc()
{
	if (m_bTalking)
		GET_SINGLE(UIManager)->ClearChoiceUI(false);
	SyncInteractionPrompt(false);
	EndMiniGameWorldPause();
}

HRESULT CInteractiveNpc::InitializePrototype(void* pArg)
{
	return __super::InitializePrototype(pArg);
}

HRESULT CInteractiveNpc::Initialize(void* pArg)
{
	if (!pArg)
		return E_INVALIDARG;

	auto* pDesc = static_cast<DESC*>(pArg);
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	m_iHp = m_iMaxHp = 3555;

	if (!m_pComRigidBody || !m_pComSphereCol || !m_pCharacterController ||
		!m_pMoveIntent || !m_pCharacterMotor || !m_pComModelInstance ||
		!m_pModelAnimator)
		return E_FAIL;

	const _matrix rotation =
		XMMatrixRotationX(XMConvertToRadians(pDesc->vRot.x)) *
		XMMatrixRotationY(XMConvertToRadians(pDesc->vRot.y)) *
		XMMatrixRotationZ(XMConvertToRadians(pDesc->vRot.z));
	GetTransform().SetQuaternion(XMQuaternionRotationMatrix(rotation));

	GetTransform().SetPosition(pDesc->vPos);
	GetTransform().Update();
	m_pModelAnimator->SetEvaluationMode(CComAnimator::EVALUATION_MODE::CPU_GPU);
	m_pModelAnimator->Build_BoneMatrices_CPU(0.f);
	m_pComSphereCol->SetQueryEnabled(true);
	m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::ADD);
	m_eState = STATE::IDLE;

	m_SpeakerName = pDesc->SpeakerName;
	m_Dialogue = pDesc->Dialogue;
	m_ResolveStartDialogueIndex = pDesc->ResolveStartDialogueIndex;
	m_IdleExpressionAnim = pDesc->IdleExpressionAnim;
	m_fInteractionDistance = std::max(0.1f, pDesc->InteractionDistance);
	m_bSecondSpellMiniGame = pDesc->SecondSpellMiniGame;
	m_bRepeatable = pDesc->Repeatable;
	m_fFadeDuration = std::max(0.05f, pDesc->FadeDuration);
	m_fFadeHoldDuration = std::max(0.f, pDesc->FadeHoldDuration);
	m_vPlayerDialogueOffset = pDesc->PlayerDialogueOffset;
	m_DialogueCinematicName = pDesc->DialogueCinematicName;
	m_MoveDestinations = pDesc->MoveDestination;
	m_fMoveSpeed = std::max(0.1f, pDesc->MoveSpeed);
	m_fMoveStopDistance = std::max(0.05f, pDesc->MoveStopDistance);
	m_hAccioActivity = pDesc->AccioActivityHandle;
	m_hInteractionPlayer = pDesc->TargetHandle;
	ResolvePlayerHandle();
	SetExpression(m_IdleExpressionAnim, true);
	return S_OK;
}

void CInteractiveNpc::FixedUpdate(E::_float fTimeDelta)
{
	if (m_pCharacterMotor)
		m_pCharacterMotor->FixedUpdate(fTimeDelta);
}


void CInteractiveNpc::Update(E::_float fTimeDelta)
{
	__super::Update(fTimeDelta);
	UpdateDialogueIntro(fTimeDelta);
	UpdateMoveOutcome();
	UpdateMiniGameState();

	const _bool playerInRange = IsPlayerInRange();
	const _bool canStartDialogue = playerInRange &&
		(!m_bCompleted || m_bRepeatable) &&
		m_eState == STATE::IDLE;
	// 대화가 시작된 뒤에는 암전 중 위치 보정이나 물리 높이 차이 때문에
	// 거리 판정이 달라져도 다음 대사가 막히지 않아야 한다.
	const _bool canAdvanceDialogue = m_bTalking &&
		m_eState == STATE::TALKING &&
		m_eConversationPhase == CONVERSATION_PHASE::TALKING;

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


void CInteractiveNpc::BeginDialogue()
{
	if (m_bTalking || m_Dialogue.empty() || (m_bCompleted && !m_bRepeatable))
		return;

	m_bTalking = true;
	m_iDialogueIndex = m_ResolveStartDialogueIndex
		? m_ResolveStartDialogueIndex()
		: 0u;
	if (m_iDialogueIndex >= m_Dialogue.size())
		m_iDialogueIndex = 0u;
	m_ePendingDialogueAction = DIALOGUE_ACTION::NONE;
	SetPlayerMovementLocked(true);
	SyncInteractionPrompt(false);
	m_eConversationPhase = CONVERSATION_PHASE::FADING_OUT;
	m_eState = STATE::DIALOGUE_INTRO;
	m_fIntroElapsed = 0.f;
	// 현재 표시 중인 메인 HUD 상태를 보관한 뒤 대화 연출 동안 숨긴다.
	GET_SINGLE(UIManager)->PlayFadeOutAll2DUI(0.f, m_fFadeDuration);
	// BlackBG의 생성과 재사용은 UIManager가 관리한다.
	GET_SINGLE(UIManager)->CreateFadeIn(0.f, m_fFadeDuration);
}

void CInteractiveNpc::UpdateDialogueIntro(_float fTimeDelta)
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

		BeginDialogueCamera();
		GET_SINGLE(UIManager)->CreateFadeOut(
			m_fFadeHoldDuration, m_fFadeDuration);

		m_fIntroElapsed = -m_fFadeHoldDuration;
		m_eConversationPhase = CONVERSATION_PHASE::FADING_IN;
		return;
	}

	if (m_fIntroElapsed < m_fFadeDuration)
		return;
	ShowFirstDialogueLine();
}


void CInteractiveNpc::ShowFirstDialogueLine()
{
	m_fIntroElapsed = 0.f;
	m_eConversationPhase = CONVERSATION_PHASE::TALKING;
	m_eState = STATE::TALKING;
	const auto& line = m_Dialogue[m_iDialogueIndex];
	SetExpression(line.ExpressionAnim, line.LoopExpression);
	GET_SINGLE(UIManager)->AddDialoguePopup(m_SpeakerName, line.Text);
}

void CInteractiveNpc::AdvanceDialogue()
{
	if (!m_bTalking ||
		m_eConversationPhase != CONVERSATION_PHASE::TALKING ||
		m_iDialogueIndex >= m_Dialogue.size())
		return;

	if (m_ePendingDialogueAction != DIALOGUE_ACTION::NONE)
	{
		const DIALOGUE_ACTION action = m_ePendingDialogueAction;
		m_ePendingDialogueAction = DIALOGUE_ACTION::NONE;
		ExecuteDialogueAction(action);
		return;
	}

	const auto& currentLine = m_Dialogue[m_iDialogueIndex];
	if (!currentLine.Choices.empty())
	{
		m_eConversationPhase = CONVERSATION_PHASE::WAITING_CHOICE;
		std::vector<std::string> choiceTexts{};
		choiceTexts.reserve(currentLine.Choices.size());
		for (const auto& choice : currentLine.Choices)
			choiceTexts.push_back(choice.Text);

		const CHandle npcHandle = GetHandle();
		GET_SINGLE(UIManager)->CreateChoiceUI(
			choiceTexts,
			[npcHandle](size_t choiceIndex)
			{
				if (auto* npc = E::CGameInstance::Get().
					GetGameObjectByHandleT<CInteractiveNpc>(npcHandle))
				{
					npc->SelectDialogueChoice(choiceIndex);
				}
			});

		SyncInteractionPrompt(false);
		return;
	}

	if (currentLine.ActionOnAdvance != DIALOGUE_ACTION::NONE)
	{
		ExecuteDialogueAction(currentLine.ActionOnAdvance);
		return;
	}

	if (currentLine.ResolveNextDialogueIndex)
		m_iDialogueIndex = currentLine.ResolveNextDialogueIndex();
	else
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

void CInteractiveNpc::SelectDialogueChoice(size_t choiceIndex)
{
	if (!m_bTalking ||
		m_eConversationPhase != CONVERSATION_PHASE::WAITING_CHOICE ||
		m_iDialogueIndex >= m_Dialogue.size())
		return;

	const auto& choices = m_Dialogue[m_iDialogueIndex].Choices;
	if (choiceIndex >= choices.size())
		return;

	const DIALOGUE_CHOICE choice = choices[choiceIndex];
	size_t nextDialogueIndex = choice.NextDialogueIndex;
	if (choice.ResolveNextDialogueIndex)
		nextDialogueIndex = choice.ResolveNextDialogueIndex();

	//GET_SINGLE(UIManager)->ClearDialogueChoices();

	if (nextDialogueIndex >= m_Dialogue.size())
	{
		ExecuteDialogueAction(choice.Action);
		return;
	}

	m_ePendingDialogueAction =
		choice.Action == DIALOGUE_ACTION::CONTINUE_DIALOGUE ||
		choice.Action == DIALOGUE_ACTION::NONE
		? DIALOGUE_ACTION::NONE
		: choice.Action;
	m_iDialogueIndex = nextDialogueIndex;
	m_eConversationPhase = CONVERSATION_PHASE::TALKING;

	const auto& line = m_Dialogue[m_iDialogueIndex];
	SetExpression(line.ExpressionAnim, line.LoopExpression);
	GET_SINGLE(UIManager)->AddDialoguePopup(m_SpeakerName, line.Text);
	SyncInteractionPrompt(true);
}

void CInteractiveNpc::ExecuteDialogueAction(DIALOGUE_ACTION action)
{
	switch (action)
	{
	case DIALOGUE_ACTION::CONTINUE_DIALOGUE:
		m_eConversationPhase = CONVERSATION_PHASE::TALKING;
		SyncInteractionPrompt(true);
		break;

	case DIALOGUE_ACTION::MOVE_TO_DESTINATION:
		if (StartMoveToDestination(0u))
			FinishDialogue();
		break;

	case DIALOGUE_ACTION::START_SPELL_MINIGAME:
		if (StartSpellMiniGame())
			FinishDialogue();
		break;

	case DIALOGUE_ACTION::START_COIN_MINIGAME:
		if (StartCoinMiniGame())
			FinishDialogue();
		break;

	case DIALOGUE_ACTION::START_ACCIO_MINIGAME:
		if (StartAccioMiniGame())
			FinishDialogue();
		break;
	case DIALOGUE_ACTION::OPEN_SHOP:
		// 상점 ui open

		FinishDialogue();
		break;

	case DIALOGUE_ACTION::CANCEL_DIALOGUE:
		CancelDialogue();
		break;

	case DIALOGUE_ACTION::NONE:
	default:
		FinishDialogue();
		break;
	}
}


void CInteractiveNpc::CancelDialogue()
{
	GET_SINGLE(UIManager)->ClearChoiceUI(false);
	m_bTalking = false;
	m_iDialogueIndex = 0u;
	m_ePendingDialogueAction = DIALOGUE_ACTION::NONE;
	SetExpression(m_IdleExpressionAnim, true);
	EndDialogueCamera();
	SetPlayerMovementLocked(false);
	GET_SINGLE(UIManager)->PlayFadeInAll2DUI(0.f, m_fFadeDuration);
	m_eConversationPhase = CONVERSATION_PHASE::IDLE;
	m_eState = STATE::IDLE;
}


void CInteractiveNpc::FinishDialogue()
{
	GET_SINGLE(UIManager)->ClearChoiceUI(false);
	m_bTalking = false;
	m_bCompleted = true;
	m_ePendingDialogueAction = DIALOGUE_ACTION::NONE;
	SetExpression(m_IdleExpressionAnim, true);
	m_eConversationPhase = CONVERSATION_PHASE::IDLE;

	// 이동 화면 전환과 미니게임이 끝날 때까지 카메라와 입력 잠금을 유지한다.
	if (m_eState == STATE::MOVING || m_eState == STATE::MINIGAME)
		return;

	EndDialogueCamera();
	SetPlayerMovementLocked(false);
	GET_SINGLE(UIManager)->PlayFadeInAll2DUI(0.f, m_fFadeDuration);
	m_eState = STATE::IDLE;
}


void CInteractiveNpc::BeginDialogueCamera()
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

	if (m_DialogueCinematicName.empty())
		return;

	E::FCinematicPlayOptions options{};
	options.eStartMode = E::ECinematicStartMode::Immediate;
	options.eReturnMode = E::ECinematicReturnMode::Immediate;
	m_bDialogueCinematicPlaying =
		gameInstance.PlayCinematic(
			StringID{ m_DialogueCinematicName }, GetHandle(), options) == S_OK;
}


void CInteractiveNpc::EndDialogueCamera()
{
	if (!m_bDialogueCinematicPlaying)
		return;

	E::CGameInstance::Get().StopCinematic();
	m_bDialogueCinematicPlaying = false;
}


void CInteractiveNpc::SetPlayerMovementLocked(_bool locked)
{
	if (auto* pPlayer = E::CGameInstance::Get().GetGameObjectByHandleT<CPlayer>(m_hInteractionPlayer))
		pPlayer->SetMovementLocked(locked);
}


_bool CInteractiveNpc::StartMoveToDestination(size_t destinationIndex)
{
	if (destinationIndex >= m_MoveDestinations.size())
		return false;

	m_vMoveDestination = m_MoveDestinations[destinationIndex];

	GET_SINGLE(UIManager)->CreateFadeIn(0.f, m_fMoveFadeInDuration);

	m_bMovingToDestination = true;
	m_fMoveOutcomeElapsed = 0.f;
	m_eState = STATE::MOVING;
	return true;
}

_bool CInteractiveNpc::StartSpellMiniGame()
{
	const auto hController = GET_SINGLE(UIManager)->GetUIController();
	if (!hController)
		return false;

	auto* pController = E::CGameInstance::Get().GetGameObjectByHandleT<CUIController>(*hController);
	if (!pController)
		return false;

	const _bool bSecondSpellMiniGame =
		m_bSecondSpellMiniGame ||
		GET_SINGLE(UIManager)->IsSpellUnlocked(SPELL_TYPE::TRANSFORMATION);
	if (!pController->StartSpellMiniGame(bSecondSpellMiniGame))
		return false;

	m_eActiveMiniGame = ACTIVE_MINIGAME::SPELL;
	m_eState = STATE::MINIGAME;
	BeginMiniGameWorldPause();
	return true;
}

_bool CInteractiveNpc::StartCoinMiniGame()
{
	// 코인 코스는 별도의 런타임 컨트롤러가 없고 코인 충돌체가 이미 활성화되어 있다.
	// 따라서 설정된 코스 시작 위치로 이동시키는 것이 시작 동작이다.
	m_eActiveMiniGame = ACTIVE_MINIGAME::COIN;
	if (StartMoveToDestination(1u))
		return true;

	m_eActiveMiniGame = ACTIVE_MINIGAME::NONE;
	return false;
}

_bool CInteractiveNpc::StartAccioMiniGame()
{
	auto* pActivity = E::CGameInstance::Get().
		GetGameObjectByHandleT<CAccioActivity_Base>(m_hAccioActivity);
	if (!pActivity)
		return false;

	GET_SINGLE(UIManager)->FadeOutQuest(0.3f);
	m_eActiveMiniGame = ACTIVE_MINIGAME::ACCIO;
	if (StartMoveToDestination(0u))
		return true;

	m_eActiveMiniGame = ACTIVE_MINIGAME::NONE;
	return false;
}


void CInteractiveNpc::BeginMiniGameWorldPause()
{
	E::TIME_SCALE_REQUEST_DESC Desc{};
	Desc.fTargetScale = 0.f;
	Desc.fBlendIn = 0.f;
	Desc.fMaxUnscaledDuration = 600.f;
	Desc.fSafetyBlendOut = 0.f;
	Desc.sTag = MINIGAME_NPC_TIME_SCALE_TAG;
	m_bOwnsWorldPause = E::CGameInstance::Get().BeginTimeScale(Desc);
}

void CInteractiveNpc::UpdateMiniGameState()
{
	if (m_eState != STATE::MINIGAME)
		return;

	if (m_eActiveMiniGame == ACTIVE_MINIGAME::SPELL)
	{
		if (IsSpellMiniGameRunning())
			return;

		EndMiniGameWorldPause();
		EndDialogueCamera();
		SetPlayerMovementLocked(false);
		GET_SINGLE(UIManager)->PlayFadeInAll2DUI(0.f, m_fFadeDuration);
	}
	else if (m_eActiveMiniGame == ACTIVE_MINIGAME::ACCIO)
	{
		auto* pActivity = E::CGameInstance::Get().
			GetGameObjectByHandleT<CAccioActivity_Base>(m_hAccioActivity);
		if (pActivity && pActivity->GetMatchState() !=
			CAccioActivity_Base::MATCH_STATE::MATCH_END)
		{
			return;
		}
		GET_SINGLE(UIManager)->FadeInQuest(0.5f);
	}

	m_eActiveMiniGame = ACTIVE_MINIGAME::NONE;
	m_eState = STATE::IDLE;
}


void CInteractiveNpc::EndMiniGameWorldPause()
{
	if (!m_bOwnsWorldPause)
		return;
	auto& gameInstance = E::CGameInstance::Get();
	if (gameInstance.IsTimeScaleActive(MINIGAME_NPC_TIME_SCALE_TAG))
		gameInstance.EndTimeScale(MINIGAME_NPC_TIME_SCALE_TAG, 0.f);
	m_bOwnsWorldPause = false;
}


_bool CInteractiveNpc::IsSpellMiniGameRunning() const
{
	auto& gameInstance = E::CGameInstance::Get();
	for (const auto& [layerTag, handles] : gameInstance.GetGameObjectLayers())
		for (const CHandle& handle : handles)
			if (gameInstance.GetGameObjectByHandleT<CSpellMiniGame>(handle))
				return true;
	return false;
}


void CInteractiveNpc::UpdateMoveOutcome()
{
	if (!m_bMovingToDestination)
		return;

	m_fMoveOutcomeElapsed += E::CGameInstance::Get().GetUnscaledDelta();
	if (m_fMoveOutcomeElapsed < m_fMoveFadeInDuration)
		return;

	auto& gameInstance = E::CGameInstance::Get();
	if (auto* pPlayer = gameInstance.GetGameObjectByHandleT<CPlayer>(m_hInteractionPlayer))
	{
		_float3 lookAt = GetTransform().GetPosition();
		lookAt.y = m_vMoveDestination.y;
		pPlayer->SetDialoguePose(m_vMoveDestination, lookAt);
	}

	EndDialogueCamera();
	if (m_eActiveMiniGame == ACTIVE_MINIGAME::COIN)
	{
		GET_SINGLE(UIManager)->CreateFadeOut(
			0.f, m_fMoveFadeOutDuration,
			[]()
			{
				GET_SINGLE(UIManager)->StartRaceMiniGame();
			});
	}
	else
	{
		GET_SINGLE(UIManager)->CreateFadeOut(
			0.f, m_fMoveFadeOutDuration);
	}
	SetPlayerMovementLocked(false);
	GET_SINGLE(UIManager)->PlayFadeInAll2DUI(0.f, m_fMoveFadeOutDuration);
	m_bMovingToDestination = false;

	if (m_eActiveMiniGame == ACTIVE_MINIGAME::ACCIO)
	{
		auto* pActivity = gameInstance.
			GetGameObjectByHandleT<CAccioActivity_Base>(m_hAccioActivity);
		if (pActivity && pActivity->StartMatch())
		{
			m_eState = STATE::MINIGAME;
			return;
		}
	}

	m_eActiveMiniGame = ACTIVE_MINIGAME::NONE;
	m_eState = STATE::IDLE;
}

void CInteractiveNpc::SetExpression(const _string& animName, _bool loop)
{
	if (animName.empty() || !m_pModelAnimator)
		return;

	const int32_t animIndex = Find_AnimIndex(animName);
	if (animIndex >= 0)
		m_pModelAnimator->Play_Anim(animIndex, loop);
}


_bool CInteractiveNpc::IsPlayerInRange()
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


void CInteractiveNpc::ResolvePlayerHandle()
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

// 대화 시작용 F 상호작용 UI를 생성하거나 제거한다.
void CInteractiveNpc::SyncInteractionPrompt(_bool show)
{
	if (m_bPromptVisible == show)
		return;

	m_bPromptVisible = show;
	if (show)
		GET_SINGLE(UIManager)->CreateActiveButton(GetHandle(), DIK_F);
	else
		GET_SINGLE(UIManager)->RemoveActiveButton(GetHandle());
}

// 상호작용 NPC의 프로토타입 객체를 생성한다.
E::UPtr<CInteractiveNpc> CInteractiveNpc::Create()
{
	auto pInstance = E::ToUPtr(new CInteractiveNpc{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CInteractiveNpc");
		return nullptr;
	}
	return pInstance;
}

// 설명자를 적용해 실제 상호작용 NPC 인스턴스를 복제한다.
E::UPtr<E::CPrototype> CInteractiveNpc::Clone(void* pArg)
{
	auto pInstance = E::ToUPtr(new CInteractiveNpc{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CInteractiveNpc");
		return nullptr;
	}
	return pInstance;
}
