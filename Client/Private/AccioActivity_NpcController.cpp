#include "pch.h"
#include "AccioActivity_NpcController.h"

#include "AccioActivity_Base.h"
#include "AccioActivity_Platform.h"
#include "AccioActivity_NpcCharacter.h"
#include "AccioBall.h"
#include "ComPxRigidBody.h"
#include "CinematicAsset.h"
#include "CinematicTypes.h"
#include "DbgLineRender.h"
#include "GameInstance.h"
#include "Player.h"
#include "UIManager.h"

NS_USING(Client)

CAccioActivity_NpcController::CAccioActivity_NpcController() = default;

CAccioActivity_NpcController::CAccioActivity_NpcController(const CAccioActivity_NpcController& prototype)
	: CGameObject{ prototype }
{
}

HRESULT CAccioActivity_NpcController::InitializePrototype(void*)
{
	return S_OK;
}

HRESULT CAccioActivity_NpcController::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc || FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	m_hActivity = pDesc->hActivity;
	m_hPlatform = pDesc->hPlatform;
	m_hNpcCharacter = pDesc->hNpcCharacter;
	m_hInteractionPlayer = pDesc->hInteractionPlayer;
	m_sSpeakerName = pDesc->SpeakerName;
	m_Dialogue = pDesc->Dialogue;
	m_PlayerWinDialogue = pDesc->PlayerWinDialogue;
	m_NpcWinDialogue = pDesc->NpcWinDialogue;
	m_DrawDialogue = pDesc->DrawDialogue;
	m_fInteractionDistance = std::max(pDesc->fInteractionDistance, 0.1f);
	m_bRepeatDialogue = pDesc->bRepeatDialogue;
	m_fDialogueFadeDuration = std::max(
		pDesc->fDialogueFadeDuration, 0.05f);
	m_fDialogueFadeHoldDuration = std::max(
		pDesc->fDialogueFadeHoldDuration, 0.f);
	m_vPlayerDialogueOffset = pDesc->vPlayerDialogueOffset;
	m_vDialogueCameraOffset = pDesc->vDialogueCameraOffset;
	m_fDialogueCameraTargetHeight = pDesc->fDialogueCameraTargetHeight;
	m_fDialogueCameraFovY = std::max(pDesc->fDialogueCameraFovY, 1.f);
	m_fTurnStartDelay = pDesc->fTurnStartDelay;
	m_fMoveSpeed = pDesc->fMoveSpeed;
	m_fMoveAcceleration = pDesc->fMoveAcceleration;
	m_fMoveDeceleration = pDesc->fMoveDeceleration;
	m_fMoveArrivalDistance = pDesc->fMoveArrivalDistance;
	m_fMoveAreaMargin = pDesc->fMoveAreaMargin;
	m_fSideStandbyInset = pDesc->fSideStandbyInset;
	m_fMatchRestBackwardOffset = pDesc->fMatchRestBackwardOffset;
	m_fAimDelay = pDesc->fAimDelay;
	m_fScorePullDuration = pDesc->fScorePullDuration;
	m_fMaximumAttackEdgeDistance = pDesc->fMaximumAttackEdgeDistance;
	m_fEstimatedAttackPullSpeed = pDesc->fEstimatedAttackPullSpeed;
	m_fAttackEdgeHoldSecondsPerUnit = pDesc->fAttackEdgeHoldSecondsPerUnit;
	m_fAttackAimOffsetRatio = pDesc->fAttackAimOffsetRatio;
	m_fMinimumAttackPullDuration = pDesc->fMinimumAttackPullDuration;
	m_fMaximumAttackPullDuration = pDesc->fMaximumAttackPullDuration;
	m_fPullTimeout = pDesc->fPullTimeout;
	m_fReleaseLeadTime = pDesc->fReleaseLeadTime;
	m_iMinimumAttackTargetScore = pDesc->iMinimumAttackTargetScore;
	SanitizeTuning();
	m_fPlannedPullDuration = m_fScorePullDuration;
	m_bDebugDraw = pDesc->bDebugDraw;

	GetTransform().SetPosition(pDesc->vInitialPosition);
	GetTransform().SetRotationEuler(pDesc->vInitialRotation);
	GetTransform().Update();
	return S_OK;
}

void CAccioActivity_NpcController::OnRegisteredToManager()
{
	if (!GetNpcCharacter())
		return;

	if (auto* pActivity = CGameInstance::Get().
		GetGameObjectByHandleT<CAccioActivity_Base>(m_hActivity))
	{
		// [LSY] 경기 참가자와 공 제어자는 판단 전용 객체가 아니라 실제 학생 Pawn이다.
		pActivity->SetParticipantHandle(
			CAccioActivity_Base::PARTICIPANT::NPC,
			m_hNpcCharacter);
	}
}

void CAccioActivity_NpcController::FixedUpdate(_float fTimeDelta)
{
	if (!GetNpcCharacter())
	{
		ResetTurnState();
		return;
	}

	auto* pActivity = CGameInstance::Get().
		GetGameObjectByHandleT<CAccioActivity_Base>(m_hActivity);
	if (!pActivity || pActivity->GetPendingDestroy())
	{
		ResetTurnState();
		return;
	}

	if (m_bTalking)
	{
		ResolveInteractionPlayer();
		if (const auto* pPlayer = CGameInstance::Get().
			GetGameObjectByHandleT<CPlayer>(m_hInteractionPlayer))
		{
			FaceTowards(pPlayer->GetTransform().GetPosition());
		}
		return;
	}

	const auto eMatchState = pActivity->GetMatchState();
	const _float fSafeDelta = std::max(fTimeDelta, 0.f);
	const _bool bMatchInactive =
		eMatchState == CAccioActivity_Base::MATCH_STATE::READY ||
		eMatchState == CAccioActivity_Base::MATCH_STATE::MATCH_END;
	if (bMatchInactive)
	{
		if (m_eState == STATE::LEAVING_MATCH)
		{
			UpdatePlatformPath(fSafeDelta);
			return;
		}

		if (!m_bAtSideStandby)
		{
			// [LSY] 리셋 또는 경기 종료 시 어떤 턴 상태였든 오른쪽 대기 위치로 복귀한다.
			BeginLeaveMatch();
			return;
		}

		GetNpcCharacter()->SetAction(CAccioActivity_NpcCharacter::ACTION::IDLE);
		if (m_bHasRestFacingTarget)
			FaceTowards(m_vRestFacingTarget);
		return;
	}

	if (m_eState == STATE::ENTERING_MATCH)
	{
		UpdatePlatformPath(fSafeDelta);
		return;
	}

	if (!m_bMatchEntered)
	{
		// [LSY] 선공 여부와 관계없이 매치에 들어오면 중앙 뒤쪽 대기점을 먼저 거친다.
		BeginEnterMatch();
		return;
	}

	if (m_eState == STATE::PULL_RECOVERY)
	{
		UpdatePullRecovery();
		return;
	}

	if (eMatchState == CAccioActivity_Base::MATCH_STATE::NPC_TURN)
	{
		UpdateNpcTurn(*pActivity, fSafeDelta);
		return;
	}

	if (m_eState == STATE::RETURNING)
	{
		UpdatePlatformPath(fSafeDelta);
		return;
	}

	if (eMatchState == CAccioActivity_Base::MATCH_STATE::WAIT_NPC_BALL_SETTLED)
	{
		// [LSY] NPC 공의 Sleep과 점수 확정이 끝날 때까지 시전 위치에서 결과를 지켜본다.
		m_eState = STATE::WAIT_BALL_SETTLED;
		GetNpcCharacter()->SetAction(CAccioActivity_NpcCharacter::ACTION::IDLE);
		return;
	}

	if (m_eState == STATE::WAIT_BALL_SETTLED)
	{
		// [LSY] Base가 대기 상태를 벗어난 시점이 점수 결과가 확정된 시점이다.
		if (!BeginReturnToRest())
			ResetTurnState();
		return;
	}

	ResetTurnState();
}

void CAccioActivity_NpcController::UpdateNpcTurn(
	CAccioActivity_Base& activity,
	_float fTimeDelta)
{
	switch (m_eState)
	{
	case STATE::IDLE:
	{
		GetNpcCharacter()->SetAction(CAccioActivity_NpcCharacter::ACTION::IDLE);
		m_fStateElapsed += fTimeDelta;
		if (m_fStateElapsed < m_fTurnStartDelay)
			return;

		const auto hBall = activity.FindControllableBall(GetParticipantHandle());
		if (!hBall)
		{
			// [LSY] 리소스 누락이나 공 파괴로 선택지가 없어도 NPC 턴을 고착시키지 않는다.
			AbortNpcTurn(activity);
			return;
		}

		m_hActiveBall = *hBall;
		m_hDisruptionTargetBall = CHandle{};
		const _float fOffsetMagnitude = Randf(0.35f, 1.f) *
			m_fAttackAimOffsetRatio;
		m_fCurrentAttackAimOffsetRatio = RandInt(0, 1) == 0 ?
			-fOffsetMagnitude : fOffsetMagnitude;

		m_fStateElapsed = 0.f;
		const CAccioBall* pBall = GetSelectedBall();
		if (pBall && PrepareMoveTarget(activity, *pBall))
		{
			m_eState = STATE::MOVING;
		}
		else
		{
			// [LSY] 이동 영역이나 공 정보를 읽지 못한 상태로 제어를 시작하지 않는다.
			AbortNpcTurn(activity);
		}
		break;
	}

	case STATE::MOVING:
		m_fStateElapsed += fTimeDelta;
		if (!GetSelectedBall())
		{
			AbortNpcTurn(activity);
			break;
		}
		if (MoveToPreparedTarget(fTimeDelta))
		{
			m_fStateElapsed = 0.f;
			m_eState = STATE::AIMING;
		}
		break;

	case STATE::AIMING:
		GetNpcCharacter()->SetAction(CAccioActivity_NpcCharacter::ACTION::AIM);
		if (const CAccioBall* pBall = GetSelectedBall())
			FaceTowards(pBall->GetTransform().GetPosition());
		else
		{
			AbortNpcTurn(activity);
			break;
		}

		m_fStateElapsed += fTimeDelta;
		if (m_fStateElapsed >= m_fAimDelay)
		{
			const CAccioBall* pBall = GetSelectedBall();
			if (!pBall || !PrepareMoveTarget(activity, *pBall))
			{
				AbortNpcTurn(activity);
				break;
			}

			// [LSY] 이동·조준 중 공 배치가 바뀌었으면 최신 계획 위치로 다시 이동한다.
			if (!IsAtPreparedMoveTarget())
			{
				m_fStateElapsed = 0.f;
				m_eState = STATE::MOVING;
				break;
			}

			if (AcquireSelectedBall())
			{
				m_fStateElapsed = 0.f;
				m_eState = STATE::PULLING;
			}
			else
			{
				AbortNpcTurn(activity);
			}
		}
		break;

	case STATE::PULLING:
	{
		GetNpcCharacter()->SetAction(CAccioActivity_NpcCharacter::ACTION::PULL);
		m_fStateElapsed += fTimeDelta;
		CAccioBall* pBall = GetSelectedBall();
		if (!pBall || !pBall->IsControlledBy(GetParticipantHandle()))
		{
			AbortNpcTurn(activity);
			break;
		}

		// [LSY] 턴 시작 시 전략에 맞춰 계산한 유지시간만큼만 제어한다.
		// 득점은 고정시간, 공격은 충돌점과 보드 끝 거리로 계산된 시간이다.
		// [LSY] 득점과 공격 모두 계산된 시점보다 약간 먼저 놓아 보드 끝에 걸치는 현상을 줄인다.
		const _float fReleaseDuration = std::max(
			m_fPlannedPullDuration - m_fReleaseLeadTime,
			0.1f);
		const _bool bReachedPlannedDuration =
			m_fStateElapsed >= fReleaseDuration;
		const _bool bTimedOut = m_fStateElapsed >= m_fPullTimeout;
		if (bReachedPlannedDuration || bTimedOut)
		{
			if (ReleaseSelectedBall())
			{
				// [LSY] 공을 놓은 직후 걷지 않고 AccioPull의 남은 동작부터 끝낸다.
				GetNpcCharacter()->SetAction(CAccioActivity_NpcCharacter::ACTION::IDLE);
				m_fStateElapsed = 0.f;
				m_eState = STATE::PULL_RECOVERY;
			}
			else
			{
				AbortNpcTurn(activity);
			}
		}
		break;
	}

	case STATE::PULL_RECOVERY:
		UpdatePullRecovery();
		break;

	case STATE::WAIT_BALL_SETTLED:
		GetNpcCharacter()->SetAction(CAccioActivity_NpcCharacter::ACTION::IDLE);
		break;

	case STATE::ENTERING_MATCH:
	case STATE::RETURNING:
	case STATE::LEAVING_MATCH:
		UpdatePlatformPath(fTimeDelta);
		break;
	}
}

void CAccioActivity_NpcController::Update(_float fTimeDelta)
{
	CGameObject::Update(fTimeDelta);
	UpdateDialogue(std::max(fTimeDelta, 0.f));
	UpdateAccioEffects(std::max(fTimeDelta, 0.f));
}

void CAccioActivity_NpcController::SetInteractionPlayerHandle(
	const CHandle& hPlayer)
{
	m_hInteractionPlayer = hPlayer;
}

void CAccioActivity_NpcController::UpdateDialogue(_float fTimeDelta)
{
	auto* pActivity = CGameInstance::Get().
		GetGameObjectByHandleT<CAccioActivity_Base>(m_hActivity);
	if (!pActivity || pActivity->GetPendingDestroy())
	{
		if (m_bTalking)
			CancelDialogue();
		else
			SyncInteractionPrompt(false);
		return;
	}

	const auto eMatchState = pActivity->GetMatchState();
	const _bool bDialogueStateValid =
		(m_eDialoguePurpose == DIALOGUE_PURPOSE::START_MATCH &&
			eMatchState == CAccioActivity_Base::MATCH_STATE::READY) ||
		(m_eDialoguePurpose == DIALOGUE_PURPOSE::MATCH_RESULT &&
			eMatchState == CAccioActivity_Base::MATCH_STATE::MATCH_END);
	if (m_bTalking && !bDialogueStateValid)
	{
		CancelDialogue();
		return;
	}

	if (m_bTalking)
	{
		UpdateDialogueIntro(fTimeDelta);
		const _bool bCanAdvanceDialogue =
			m_eConversationPhase == CONVERSATION_PHASE::TALKING;
		// 대화 진행용 F 입력은 유지하지만 액티브 버튼 UI는 숨긴다.
		SyncInteractionPrompt(false);
		if (!bCanAdvanceDialogue || !CGameInstance::Get().KeyDown(DIK_F))
			return;

		GET_SINGLE(UIManager)->RemoveActiveButton(GetHandle(), false);
		m_bInteractionPromptVisible = false;
		AdvanceDialogue();
		return;
	}

	if (eMatchState == CAccioActivity_Base::MATCH_STATE::MATCH_END)
	{
		SyncInteractionPrompt(false);
		// 최종 점수와 승패 UI가 모두 끝난 뒤 결과 대화를 시작한다.
		if (GET_SINGLE(UIManager)->IsAssioMiniGameActive())
			return;
		if (!m_bMatchEndDialogueCompleted &&
			m_bAtSideStandby && m_eState == STATE::IDLE)
		{
			// [LSY] 경기 결과가 확정되고 NPC가 측면 대기점에 복귀한 뒤 결과 대화를 자동 시작한다.
			BeginMatchEndDialogue(*pActivity);
		}
		return;
	}

	if (eMatchState != CAccioActivity_Base::MATCH_STATE::READY)
	{
		// [LSY] 새 경기가 실제로 진행되면 다음 MATCH_END에서 결과 대화를 다시 허용한다.
		m_bMatchEndDialogueCompleted = false;
		SyncInteractionPrompt(false);
		return;
	}

	const _bool bCanStartDialogue =
		!m_Dialogue.empty() &&
		m_bAtSideStandby &&
		m_eState == STATE::IDLE &&
		(!m_bDialogueCompleted || m_bRepeatDialogue) &&
		IsInteractionPlayerInRange();
	SyncInteractionPrompt(bCanStartDialogue);

	if (!bCanStartDialogue ||
		!CGameInstance::Get().KeyDown(DIK_F))
	{
		return;
	}

	// [LSY] UIManager도 같은 프레임에 F 입력을 소비하므로 로컬 표시 상태를 함께 맞춘다.
	GET_SINGLE(UIManager)->RemoveActiveButton(GetHandle(), false);
	m_bInteractionPromptVisible = false;
	BeginDialogue();
}

void CAccioActivity_NpcController::BeginDialogue()
{
	if (m_bTalking || m_Dialogue.empty() ||
		(m_bDialogueCompleted && !m_bRepeatDialogue) ||
		!m_bAtSideStandby)
	{
		return;
	}

	BeginDialogueSequence(m_Dialogue, DIALOGUE_PURPOSE::START_MATCH);
}

void CAccioActivity_NpcController::BeginDialogueSequence(
	const std::vector<DIALOGUE_LINE>& dialogue,
	DIALOGUE_PURPOSE ePurpose)
{
	if (m_bTalking || dialogue.empty() ||
		ePurpose == DIALOGUE_PURPOSE::NONE)
	{
		return;
	}

	m_ActiveDialogue = dialogue;
	m_eDialoguePurpose = ePurpose;
	m_bTalking = true;
	if (ePurpose == DIALOGUE_PURPOSE::START_MATCH)
	{
		GET_SINGLE(UIManager)->SetMiniMapObjectiveActive(
			"Hogwart_AccioStudentQuest", false);
	}
	m_iDialogueIndex = 0u;
	m_fDialogueIntroElapsed = 0.f;
	m_eConversationPhase = CONVERSATION_PHASE::FADING_OUT;
	StopAccioEffects();
	SyncInteractionPrompt(false);
	SetPlayerMovementLocked(true);

	if (auto* pNpcCharacter = GetNpcCharacter())
		pNpcCharacter->SetAction(CAccioActivity_NpcCharacter::ACTION::IDLE);

	// [LSY] 현재 HUD를 숨긴 뒤 동일한 BlackBG를 이용해 대화 카메라 전환을 가린다.
	GET_SINGLE(UIManager)->PlayFadeOutAll2DUI(
		0.f, m_fDialogueFadeDuration);
	GET_SINGLE(UIManager)->CreateFadeIn(
		0.f, m_fDialogueFadeDuration);
}

void CAccioActivity_NpcController::BeginMatchEndDialogue(
	const CAccioActivity_Base& activity)
{
	const std::vector<DIALOGUE_LINE>* pDialogue = &m_DrawDialogue;
	if (activity.GetBlueScore() > activity.GetRedScore())
		pDialogue = &m_PlayerWinDialogue;
	else if (activity.GetRedScore() > activity.GetBlueScore())
		pDialogue = &m_NpcWinDialogue;

	if (pDialogue->empty())
	{
		m_bMatchEndDialogueCompleted = true;
		return;
	}

	BeginDialogueSequence(*pDialogue, DIALOGUE_PURPOSE::MATCH_RESULT);
}

void CAccioActivity_NpcController::UpdateDialogueIntro(_float fTimeDelta)
{
	if (m_eConversationPhase != CONVERSATION_PHASE::FADING_OUT &&
		m_eConversationPhase != CONVERSATION_PHASE::HOLDING_BLACK &&
		m_eConversationPhase != CONVERSATION_PHASE::FADING_IN)
	{
		return;
	}

	m_fDialogueIntroElapsed += fTimeDelta;
	if (m_eConversationPhase == CONVERSATION_PHASE::FADING_OUT)
	{
		if (m_fDialogueIntroElapsed < m_fDialogueFadeDuration)
			return;

		m_fDialogueIntroElapsed = 0.f;
		m_eConversationPhase = CONVERSATION_PHASE::HOLDING_BLACK;
		return;
	}

	if (m_eConversationPhase == CONVERSATION_PHASE::HOLDING_BLACK)
	{
		if (m_fDialogueIntroElapsed < m_fDialogueFadeHoldDuration)
			return;

		m_fDialogueIntroElapsed = 0.f;
		BeginDialogueCamera();
		GET_SINGLE(UIManager)->CreateFadeOut(
			0.f, m_fDialogueFadeDuration);
		m_eConversationPhase = CONVERSATION_PHASE::FADING_IN;
		return;
	}

	if (m_fDialogueIntroElapsed < m_fDialogueFadeDuration)
		return;

	ShowFirstDialogueLine();
}

void CAccioActivity_NpcController::ShowFirstDialogueLine()
{
	if (m_ActiveDialogue.empty())
	{
		CancelDialogue();
		return;
	}

	m_fDialogueIntroElapsed = 0.f;
	m_eConversationPhase = CONVERSATION_PHASE::TALKING;
	const auto& line = m_ActiveDialogue[m_iDialogueIndex];
	SetDialogueExpression(line);
	GET_SINGLE(UIManager)->AddDialoguePopup(m_sSpeakerName, line.Text);
}

void CAccioActivity_NpcController::AdvanceDialogue()
{
	if (!m_bTalking ||
		m_eConversationPhase != CONVERSATION_PHASE::TALKING)
	{
		return;
	}

	++m_iDialogueIndex;
	if (m_iDialogueIndex >= m_ActiveDialogue.size())
	{
		FinishDialogue();
		return;
	}

	const auto& line = m_ActiveDialogue[m_iDialogueIndex];
	SetDialogueExpression(line);
	GET_SINGLE(UIManager)->AddDialoguePopup(m_sSpeakerName, line.Text);
}

void CAccioActivity_NpcController::CancelDialogue()
{
	m_bTalking = false;
	m_ActiveDialogue.clear();
	m_eDialoguePurpose = DIALOGUE_PURPOSE::NONE;
	m_iDialogueIndex = 0u;
	m_fDialogueIntroElapsed = 0.f;
	m_eConversationPhase = CONVERSATION_PHASE::IDLE;
	SyncInteractionPrompt(false);

	if (auto* pNpcCharacter = GetNpcCharacter())
		pNpcCharacter->SetAction(CAccioActivity_NpcCharacter::ACTION::IDLE);
	EndDialogueCamera();
	SetPlayerMovementLocked(false);
	GET_SINGLE(UIManager)->PlayFadeInAll2DUI(
		0.f, m_fDialogueFadeDuration);
}

void CAccioActivity_NpcController::FinishDialogue()
{
	const DIALOGUE_PURPOSE eFinishedPurpose = m_eDialoguePurpose;
	m_bTalking = false;
	if (eFinishedPurpose == DIALOGUE_PURPOSE::START_MATCH)
		m_bDialogueCompleted = true;
	else if (eFinishedPurpose == DIALOGUE_PURPOSE::MATCH_RESULT)
		m_bMatchEndDialogueCompleted = true;
	m_ActiveDialogue.clear();
	m_eDialoguePurpose = DIALOGUE_PURPOSE::NONE;
	m_iDialogueIndex = 0u;
	m_fDialogueIntroElapsed = 0.f;
	m_eConversationPhase = CONVERSATION_PHASE::IDLE;
	SyncInteractionPrompt(false);

	if (auto* pNpcCharacter = GetNpcCharacter())
		pNpcCharacter->SetAction(CAccioActivity_NpcCharacter::ACTION::IDLE);
	EndDialogueCamera();
	SetPlayerMovementLocked(false);

	auto* pActivity = CGameInstance::Get().
		GetGameObjectByHandleT<CAccioActivity_Base>(m_hActivity);
	if (eFinishedPurpose == DIALOGUE_PURPOSE::MATCH_RESULT)
	{
		GET_SINGLE(UIManager)->PlayFadeInAll2DUI(
			0.f, m_fDialogueFadeDuration);
		// [LSY] 결과 확인 후 공과 점수를 준비 상태로 되돌려 NPC에게 다시 말을 걸 수 있게 한다.
		if (pActivity && pActivity->ResetMatch(true))
		{
			m_bDialogueCompleted = false;
			return;
		}

		DEBUG_LOG("[AccioActivity] Match result dialogue finished, but ResetMatch failed.\n");
		return;
	}
	if (eFinishedPurpose != DIALOGUE_PURPOSE::START_MATCH)
	{
		GET_SINGLE(UIManager)->PlayFadeInAll2DUI(
			0.f, m_fDialogueFadeDuration);
		return;
	}

	if (pActivity && pActivity->StartMatch())
		return;

	GET_SINGLE(UIManager)->PlayFadeInAll2DUI(
		0.f, m_fDialogueFadeDuration);
	// [LSY] 경기 시작 실패 시 F 상호작용을 다시 열어 재시도할 수 있게 한다.
	m_bDialogueCompleted = false;
	DEBUG_LOG("[AccioActivity] Dialogue finished, but StartMatch failed.\n");
}

void CAccioActivity_NpcController::BeginDialogueCamera()
{
	static const StringID sCinematicID{ "AccioActivityNpcDialogue" };
	static const StringID sTrackID{ "AccioActivityNpcDialogueTrack" };
	static const StringID sShotID{ "NpcFaceCloseUp" };

	auto& gameInstance = CGameInstance::Get();
	auto* pPlayer = gameInstance.
		GetGameObjectByHandleT<CPlayer>(m_hInteractionPlayer);
	auto* pNpcCharacter = GetNpcCharacter();
	if (!pPlayer || !pNpcCharacter)
		return;

	const _vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	const _vector vNpcPosition =
		pNpcCharacter->GetTransform().GetState(E::STATE::POSITION);
	_vector vNpcRight = XMVectorSetY(
		pNpcCharacter->GetTransform().GetState(E::STATE::RIGHT), 0.f);
	_vector vNpcLook = XMVectorSetY(
		pNpcCharacter->GetTransform().GetState(E::STATE::LOOK), 0.f);
	if (XMVectorGetX(XMVector3LengthSq(vNpcRight)) <= FLT_EPSILON)
		vNpcRight = XMVectorSet(1.f, 0.f, 0.f, 0.f);
	else
		vNpcRight = XMVector3Normalize(vNpcRight);
	if (XMVectorGetX(XMVector3LengthSq(vNpcLook)) <= FLT_EPSILON)
		vNpcLook = XMVectorSet(0.f, 0.f, 1.f, 0.f);
	else
		vNpcLook = XMVector3Normalize(vNpcLook);

	// [LSY] 암전 중 NPC 로컬 축 기준의 대화 위치로 플레이어를 옮긴다.
	const _vector vPlayerPosition = vNpcPosition +
		vNpcRight * m_vPlayerDialogueOffset.x +
		vUp * m_vPlayerDialogueOffset.y +
		vNpcLook * m_vPlayerDialogueOffset.z;
	_float3 vDialoguePlayerPosition{};
	_float3 vNpcLookAt{};
	XMStoreFloat3(&vDialoguePlayerPosition, vPlayerPosition);
	XMStoreFloat3(&vNpcLookAt, vNpcPosition + vUp * 1.35f);
	pPlayer->SetDialoguePose(vDialoguePlayerPosition, vNpcLookAt);

	// [LSY] NPC 로컬 앞쪽에 카메라를 두고 얼굴 높이만 바라보는 클로즈업 구도다.
	const _vector vTarget = XMVectorSet(
		0.f, m_fDialogueCameraTargetHeight, 0.f, 1.f);
	const _vector vCameraPosition = XMVectorSet(
		m_vDialogueCameraOffset.x,
		m_vDialogueCameraOffset.y,
		m_vDialogueCameraOffset.z,
		1.f);
	const _vector vLook = XMVector3Normalize(vTarget - vCameraPosition);
	const _vector vRight = XMVector3Normalize(XMVector3Cross(vUp, vLook));
	const _vector vCameraUp = XMVector3Normalize(XMVector3Cross(vLook, vRight));
	_matrix cameraWorld = XMMatrixIdentity();
	cameraWorld.r[0] = vRight;
	cameraWorld.r[1] = vCameraUp;
	cameraWorld.r[2] = vLook;
	_float3 vCameraPositionFloat{};
	_float4 vCameraRotation{};
	XMStoreFloat3(&vCameraPositionFloat, vCameraPosition);
	XMStoreFloat4(
		&vCameraRotation,
		XMQuaternionRotationMatrix(cameraWorld));

	FCinematicAssetData data{};
	data.CinematicID = sCinematicID;
	data.CameraTrack.TrackID = sTrackID;
	FCinematicCameraShot shot{};
	shot.ShotID = sShotID;
	shot.eCoordinateSpace = ECinematicCoordinateSpace::TargetLocal;
	shot.eBindingMode = ECinematicBindingMode::Live;
	FCinematicCameraKeyframe first{};
	first.vPosition = vCameraPositionFloat;
	first.vRotation = vCameraRotation;
	first.fFovY = m_fDialogueCameraFovY;
	FCinematicCameraKeyframe last = first;
	last.fTime = 600.f;
	shot.Keyframes = { first, last };
	data.CameraTrack.Shots.push_back(std::move(shot));

	auto pCinematic = CCinematicAsset::Create(data);
	if (!pCinematic || FAILED(gameInstance.RegistCinematicAsset(pCinematic)))
		return;

	FCinematicPlayOptions options{};
	options.eStartMode = ECinematicStartMode::Immediate;
	options.eReturnMode = ECinematicReturnMode::Immediate;
	m_bDialogueCinematicPlaying =
		gameInstance.PlayCinematic(
			sCinematicID, m_hNpcCharacter, options) == S_OK;
}

void CAccioActivity_NpcController::EndDialogueCamera()
{
	if (!m_bDialogueCinematicPlaying)
		return;

	CGameInstance::Get().StopCinematic();
	m_bDialogueCinematicPlaying = false;
}

void CAccioActivity_NpcController::SetPlayerMovementLocked(_bool bLocked)
{
	if (auto* pPlayer = CGameInstance::Get().
		GetGameObjectByHandleT<CPlayer>(m_hInteractionPlayer))
	{
		pPlayer->SetMovementLocked(bLocked);
	}
}

void CAccioActivity_NpcController::SetDialogueExpression(
	const DIALOGUE_LINE& line)
{
	auto* pNpcCharacter = GetNpcCharacter();
	if (!pNpcCharacter)
		return;

	if (line.ExpressionAnim.empty() ||
		!pNpcCharacter->PlayDialogueAnimation(
			line.ExpressionAnim, line.LoopExpression))
	{
		pNpcCharacter->SetAction(CAccioActivity_NpcCharacter::ACTION::IDLE);
	}
}

void CAccioActivity_NpcController::ResolveInteractionPlayer()
{
	auto& gameInstance = CGameInstance::Get();
	if (const auto* pPlayer = gameInstance.
		GetGameObjectByHandleT<CPlayer>(m_hInteractionPlayer);
		pPlayer && !pPlayer->GetPendingDestroy())
	{
		return;
	}

	m_hInteractionPlayer = CHandle{};
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

_bool CAccioActivity_NpcController::IsInteractionPlayerInRange()
{
	ResolveInteractionPlayer();
	const auto* pPlayer = CGameInstance::Get().
		GetGameObjectByHandleT<CPlayer>(m_hInteractionPlayer);
	const auto* pNpcCharacter = GetNpcCharacter();
	if (!pPlayer || pPlayer->GetPendingDestroy() || !pNpcCharacter)
		return false;

	const _float3 vNpcPosition = pNpcCharacter->GetTransform().GetPosition();
	const _float3 vPlayerPosition = pPlayer->GetTransform().GetPosition();
	const _float fDeltaX = vNpcPosition.x - vPlayerPosition.x;
	const _float fDeltaY = vNpcPosition.y - vPlayerPosition.y;
	const _float fDeltaZ = vNpcPosition.z - vPlayerPosition.z;
	return fDeltaX * fDeltaX + fDeltaY * fDeltaY + fDeltaZ * fDeltaZ <=
		m_fInteractionDistance * m_fInteractionDistance;
}

void CAccioActivity_NpcController::SyncInteractionPrompt(_bool bShow)
{
	if (m_bInteractionPromptVisible == bShow)
		return;

	m_bInteractionPromptVisible = bShow;
	if (bShow)
		GET_SINGLE(UIManager)->CreateActiveButton(GetHandle(), DIK_F);
	else
		GET_SINGLE(UIManager)->RemoveActiveButton(GetHandle());
}

void CAccioActivity_NpcController::UpdatePullRecovery()
{
	auto* pNpcCharacter = GetNpcCharacter();
	if (!pNpcCharacter)
	{
		m_eState = STATE::WAIT_BALL_SETTLED;
		return;
	}

	if (!pNpcCharacter->IsPullAnimationFinished())
		return;

	// [LSY] 후속 동작이 끝나도 공의 결과가 확정될 때까지 현재 위치를 유지한다.
	pNpcCharacter->SetAction(CAccioActivity_NpcCharacter::ACTION::IDLE);
	m_fStateElapsed = 0.f;
	m_eState = STATE::WAIT_BALL_SETTLED;
}

void CAccioActivity_NpcController::AbortNpcTurn(CAccioActivity_Base& activity)
{
	// [LSY] Release가 성공했다면 Base가 이미 WAIT 상태로 전환됐으므로 Skip하지 않는다.
	ResetTurnState();
	if (activity.GetMatchState() ==
		CAccioActivity_Base::MATCH_STATE::NPC_TURN)
	{
		activity.SkipNpcTurn(GetParticipantHandle());
	}
	BeginReturnToRest();
}

_bool CAccioActivity_NpcController::BeginEnterMatch()
{
	m_bAtSideStandby = false;
	return BeginPlatformPath(
		{ 0.f, 0.f, -m_fMatchRestBackwardOffset },
		{ 0.f, 0.f, 0.f },
		STATE::ENTERING_MATCH);
}

_bool CAccioActivity_NpcController::BeginReturnToRest()
{
	// [LSY] 턴 사이에는 중앙보다 뒤쪽으로 물러나 다음 행동 공간을 비워둔다.
	return BeginPlatformPath(
		{ 0.f, 0.f, -m_fMatchRestBackwardOffset },
		{ 0.f, 0.f, 0.f },
		STATE::RETURNING);
}

_bool CAccioActivity_NpcController::BeginLeaveMatch()
{
	const auto* pPlatform = CGameInstance::Get().
		GetGameObjectByHandleT<CAccioActivity_Platform>(m_hPlatform);
	if (!pPlatform || pPlatform->GetPendingDestroy())
		return false;

	BoundingOrientedBox moveArea{};
	if (!pPlatform->GetNpcMoveAreaWorldOBB(moveArea))
		return false;

	const _float fSideLocalX = std::max(
		moveArea.Extents.x - m_fMoveAreaMargin - m_fSideStandbyInset,
		0.f);
	m_bMatchEntered = false;
	m_bAtSideStandby = false;
	return BeginPlatformPath(
		{ fSideLocalX, 0.f, 0.f },
		{ fSideLocalX - 1.f, 0.f, 0.f },
		STATE::LEAVING_MATCH);
}

_bool CAccioActivity_NpcController::BeginPlatformPath(
	const _float3& vLocalTarget,
	const _float3& vLocalFacingTarget,
	STATE ePathState)
{
	const auto* pPlatform = CGameInstance::Get().
		GetGameObjectByHandleT<CAccioActivity_Platform>(m_hPlatform);
	if (!pPlatform || pPlatform->GetPendingDestroy())
		return false;

	BoundingOrientedBox moveArea{};
	if (!pPlatform->GetNpcMoveAreaWorldOBB(moveArea))
		return false;

	const _matrix moveAreaWorld =
		XMMatrixRotationQuaternion(XMLoadFloat4(&moveArea.Orientation)) *
		XMMatrixTranslation(
			moveArea.Center.x,
			moveArea.Center.y,
			moveArea.Center.z);
	const _matrix inverseMoveArea = XMMatrixInverse(nullptr, moveAreaWorld);
	const _float fUsableHalfX = std::max(
		moveArea.Extents.x - m_fMoveAreaMargin,
		0.f);
	const _float fUsableHalfZ = std::max(
		moveArea.Extents.z - m_fMoveAreaMargin,
		0.f);
	_float3 vSafeLocalTarget{
		std::clamp(vLocalTarget.x, -fUsableHalfX, fUsableHalfX),
		0.f,
		std::clamp(vLocalTarget.z, -fUsableHalfZ, fUsableHalfZ)
	};

	XMStoreFloat3(
		&m_vMoveTarget,
		XMVector3TransformCoord(
			XMLoadFloat3(&vSafeLocalTarget),
			moveAreaWorld));
	m_vMoveTarget.y = GetNpcPosition().y;
	XMStoreFloat3(
		&m_vRestFacingTarget,
		XMVector3TransformCoord(
			XMLoadFloat3(&vLocalFacingTarget),
			moveAreaWorld));
	m_vRestFacingTarget.y = m_vMoveTarget.y;
	m_bHasRestFacingTarget = true;

	// [LSY] 목표 자세의 반대편을 마지막 제어점으로 사용한다.
	// 도착 직전에 이미 원하는 방향으로 걷게 되어 제자리 회전이 남지 않는다.
	m_vReturnPathStart = GetNpcPosition();
	m_vReturnPathEnd = m_vMoveTarget;
	_float3 vStartLocal{};
	XMStoreFloat3(
		&vStartLocal,
		XMVector3TransformCoord(
			XMLoadFloat3(&m_vReturnPathStart),
			inverseMoveArea));

	const _float fTravelX = vSafeLocalTarget.x - vStartLocal.x;
	const _float fTravelZ = vSafeLocalTarget.z - vStartLocal.z;
	const _float fTravelLength = sqrtf(
		fTravelX * fTravelX + fTravelZ * fTravelZ);
	const _float fSafeTravelLength = std::max(fTravelLength, FLT_EPSILON);
	const _float fPerpendicularX = -fTravelZ / fSafeTravelLength;
	const _float fPerpendicularZ = fTravelX / fSafeTravelLength;
	const _float fArcWidth = std::min(
		std::max(moveArea.Extents.z * 0.45f, 0.75f),
		std::max(fUsableHalfZ, 0.75f));
	_float fFacingX = vLocalFacingTarget.x - vSafeLocalTarget.x;
	_float fFacingZ = vLocalFacingTarget.z - vSafeLocalTarget.z;
	const _float fFacingLength = sqrtf(
		fFacingX * fFacingX + fFacingZ * fFacingZ);
	if (fFacingLength > FLT_EPSILON)
	{
		fFacingX /= fFacingLength;
		fFacingZ /= fFacingLength;
	}
	else
	{
		fFacingX = fTravelX / fSafeTravelLength;
		fFacingZ = fTravelZ / fSafeTravelLength;
	}
	const _float fBehindDistance = std::min(
		std::max(m_fMoveAreaMargin * 0.5f, 0.2f),
		0.75f);
	const _float3 vControlALocal{
		std::clamp(
			std::lerp(vStartLocal.x, vSafeLocalTarget.x, 0.35f) +
				fPerpendicularX * fArcWidth,
			-fUsableHalfX,
			fUsableHalfX),
		vStartLocal.y,
		std::clamp(
			std::lerp(vStartLocal.z, vSafeLocalTarget.z, 0.35f) +
				fPerpendicularZ * fArcWidth,
			-fUsableHalfZ,
			fUsableHalfZ)
	};
	const _float fControlHalfX = std::max(
		moveArea.Extents.x - m_fMoveAreaMargin * 0.25f,
		fUsableHalfX);
	const _float fControlHalfZ = std::max(
		moveArea.Extents.z - m_fMoveAreaMargin * 0.25f,
		fUsableHalfZ);
	const _float3 vControlBLocal{
		std::clamp(
			vSafeLocalTarget.x - fFacingX * fBehindDistance,
			-fControlHalfX,
			fControlHalfX),
		vStartLocal.y,
		std::clamp(
			vSafeLocalTarget.z - fFacingZ * fBehindDistance,
			-fControlHalfZ,
			fControlHalfZ)
	};
	XMStoreFloat3(
		&m_vReturnPathControlA,
		XMVector3TransformCoord(XMLoadFloat3(&vControlALocal), moveAreaWorld));
	XMStoreFloat3(
		&m_vReturnPathControlB,
		XMVector3TransformCoord(XMLoadFloat3(&vControlBLocal), moveAreaWorld));
	m_vReturnPathControlA.y = m_vReturnPathStart.y;
	m_vReturnPathControlB.y = m_vReturnPathStart.y;

	_float fPathLength = 0.f;
	_float3 vPrevious = m_vReturnPathStart;
	for (uint32_t i = 1; i <= 12; ++i)
	{
		const _float3 vCurrent = EvaluateReturnPath(static_cast<_float>(i) / 12.f);
		const _float fDeltaX = vCurrent.x - vPrevious.x;
		const _float fDeltaZ = vCurrent.z - vPrevious.z;
		fPathLength += sqrtf(fDeltaX * fDeltaX + fDeltaZ * fDeltaZ);
		vPrevious = vCurrent;
	}
	m_fReturnPathElapsed = 0.f;
	m_fReturnPathDuration = std::max(fPathLength / m_fMoveSpeed, 0.1f);
	m_bReturnPathReady = true;

	m_fStateElapsed = 0.f;
	m_fCurrentMoveSpeed = 0.f;
	m_eState = ePathState;
	return true;
}

void CAccioActivity_NpcController::UpdatePlatformPath(_float fTimeDelta)
{
	if (!MoveAlongReturnPath(fTimeDelta))
		return;
	const STATE eCompletedPath = m_eState;

	if (auto* pNpcCharacter = GetNpcCharacter())
	{
		// [LSY] 경로 마지막 접선에서 시작한 목표 방향 회전을 제한 속도로 마무리한다.
		pNpcCharacter->FaceTowards(m_vRestFacingTarget);
		pNpcCharacter->SetAction(CAccioActivity_NpcCharacter::ACTION::IDLE);
	}
	m_fStateElapsed = 0.f;
	if (eCompletedPath == STATE::ENTERING_MATCH)
	{
		m_bMatchEntered = true;
		m_bAtSideStandby = false;
	}
	else if (eCompletedPath == STATE::LEAVING_MATCH)
	{
		m_bMatchEntered = false;
		m_bAtSideStandby = true;
	}
	m_eState = STATE::IDLE;
}

_bool CAccioActivity_NpcController::MoveAlongReturnPath(_float fTimeDelta)
{
	auto* pNpcCharacter = GetNpcCharacter();
	if (!pNpcCharacter || !m_bReturnPathReady)
		return false;

	pNpcCharacter->SetAction(CAccioActivity_NpcCharacter::ACTION::MOVE);
	const _float fSafeDelta = std::max(fTimeDelta, 0.f);
	m_fReturnPathElapsed += fSafeDelta;
	const _float fPathRatio = std::clamp(
		m_fReturnPathElapsed / m_fReturnPathDuration,
		0.f,
		1.f);
	const _float3 vPathTarget = EvaluateReturnPath(fPathRatio);
	const _float3 vPosition = pNpcCharacter->GetTransform().GetPosition();
	const _vector toTarget = XMVectorSet(
		vPathTarget.x - vPosition.x,
		0.f,
		vPathTarget.z - vPosition.z,
		0.f);
	const _float fDistance = XMVectorGetX(XMVector3Length(toTarget));
	const _float fEndDeltaX = m_vReturnPathEnd.x - vPosition.x;
	const _float fEndDeltaZ = m_vReturnPathEnd.z - vPosition.z;
	const _float fEndDistance = sqrtf(
		fEndDeltaX * fEndDeltaX + fEndDeltaZ * fEndDeltaZ);

	if (fPathRatio >= 1.f && fEndDistance <= m_fMoveArrivalDistance)
	{
		pNpcCharacter->SetWorldPosition({
			m_vReturnPathEnd.x,
			vPosition.y,
			m_vReturnPathEnd.z
		});
		m_fCurrentMoveSpeed = 0.f;
		m_bReturnPathReady = false;
		return true;
	}

	const _vector direction = fDistance > FLT_EPSILON ?
		toTarget / fDistance :
		XMVectorZero();
	const _float fBrakingDistance = std::max(
		fEndDistance - m_fMoveArrivalDistance,
		0.f);
	const _float fBrakingSpeed = sqrtf(
		2.f * m_fMoveDeceleration * fBrakingDistance);
	const _float fDesiredSpeed = std::min(m_fMoveSpeed, fBrakingSpeed);
	if (m_fCurrentMoveSpeed < fDesiredSpeed)
	{
		m_fCurrentMoveSpeed = std::min(
			m_fCurrentMoveSpeed + m_fMoveAcceleration * fSafeDelta,
			fDesiredSpeed);
	}
	else
	{
		m_fCurrentMoveSpeed = std::max(
			m_fCurrentMoveSpeed - m_fMoveDeceleration * fSafeDelta,
			fDesiredSpeed);
	}

	_float3 vMoveDirection{};
	XMStoreFloat3(&vMoveDirection, direction);
	const _float fAppliedSpeed = fSafeDelta > FLT_EPSILON && fDistance > FLT_EPSILON ?
		std::min(m_fCurrentMoveSpeed, fDistance / fSafeDelta) :
		0.f;
	pNpcCharacter->SetMoveIntent(vMoveDirection, fAppliedSpeed);

	if (fPathRatio < 0.8f)
	{
		const _float3 vFacingTarget = EvaluateReturnPath(
			std::min(fPathRatio + 0.08f, 1.f));
		pNpcCharacter->FaceTowards(vFacingTarget);
	}
	else
	{
		// [LSY] 마지막 구간부터 목적 자세를 향해 회전해 도착 후 제자리 회전을 없앤다.
		pNpcCharacter->FaceTowards(m_vRestFacingTarget);
	}
	return false;
}

_float3 CAccioActivity_NpcController::EvaluateReturnPath(_float fRatio) const
{
	const _float t = std::clamp(fRatio, 0.f, 1.f);
	const _float inverseT = 1.f - t;
	const _float startWeight = inverseT * inverseT * inverseT;
	const _float controlAWeight = 3.f * inverseT * inverseT * t;
	const _float controlBWeight = 3.f * inverseT * t * t;
	const _float endWeight = t * t * t;
	return {
		m_vReturnPathStart.x * startWeight +
			m_vReturnPathControlA.x * controlAWeight +
			m_vReturnPathControlB.x * controlBWeight +
			m_vReturnPathEnd.x * endWeight,
		m_vReturnPathStart.y,
		m_vReturnPathStart.z * startWeight +
			m_vReturnPathControlA.z * controlAWeight +
			m_vReturnPathControlB.z * controlBWeight +
			m_vReturnPathEnd.z * endWeight
	};
}

void CAccioActivity_NpcController::ResetTurnState()
{
	if (CAccioBall* pBall = GetSelectedBall();
		pBall && pBall->IsControlledBy(GetParticipantHandle()))
	{
		pBall->ReleaseControl(GetParticipantHandle());
	}

	m_hActiveBall = CHandle{};
	m_hDisruptionTargetBall = CHandle{};
	m_vMoveTarget = GetNpcPosition();
	m_fStateElapsed = 0.f;
	m_fCurrentMoveSpeed = 0.f;
	m_fReturnPathElapsed = 0.f;
	m_bReturnPathReady = false;
	m_fPlannedPullDuration = m_fScorePullDuration;
	m_fPlannedCollisionDistance = 0.f;
	m_fPlannedTargetEdgeDistance = 0.f;
	m_fCurrentAttackAimOffsetRatio = 0.f;
	m_eTactic = TACTIC::SCORE;
	m_eState = STATE::IDLE;
	if (auto* pNpcCharacter = GetNpcCharacter())
		pNpcCharacter->SetAction(CAccioActivity_NpcCharacter::ACTION::IDLE);
	if (m_bHasRestFacingTarget)
		FaceTowards(m_vRestFacingTarget);
}

void CAccioActivity_NpcController::SanitizeTuning()
{
	m_fTurnStartDelay = std::max(m_fTurnStartDelay, 0.f);
	m_fMoveSpeed = std::max(m_fMoveSpeed, 0.1f);
	m_fMoveAcceleration = std::max(m_fMoveAcceleration, 0.1f);
	m_fMoveDeceleration = std::max(m_fMoveDeceleration, 0.1f);
	m_fMoveArrivalDistance = std::max(m_fMoveArrivalDistance, 0.01f);
	m_fMoveAreaMargin = std::max(m_fMoveAreaMargin, 0.f);
	m_fSideStandbyInset = std::max(m_fSideStandbyInset, 0.f);
	m_fMatchRestBackwardOffset = std::max(
		m_fMatchRestBackwardOffset, 0.f);
	m_fAimDelay = std::max(m_fAimDelay, 0.f);
	m_fScorePullDuration = std::max(m_fScorePullDuration, 0.1f);
	m_fMaximumAttackEdgeDistance = std::max(
		m_fMaximumAttackEdgeDistance, 0.f);
	m_fEstimatedAttackPullSpeed = std::max(
		m_fEstimatedAttackPullSpeed, 0.1f);
	m_fAttackEdgeHoldSecondsPerUnit = std::max(
		m_fAttackEdgeHoldSecondsPerUnit, 0.f);
	m_fAttackAimOffsetRatio = std::clamp(
		m_fAttackAimOffsetRatio, 0.f, 0.9f);
	m_fMinimumAttackPullDuration = std::max(
		m_fMinimumAttackPullDuration, 0.1f);
	m_fMaximumAttackPullDuration = std::max(
		m_fMaximumAttackPullDuration,
		m_fMinimumAttackPullDuration);
	m_fPullTimeout = std::max(m_fPullTimeout, 0.1f);
	m_fReleaseLeadTime = std::clamp(m_fReleaseLeadTime, 0.f, 1.f);
	m_iMinimumAttackTargetScore = std::clamp(
		m_iMinimumAttackTargetScore, 30, 50);
}

_bool CAccioActivity_NpcController::PrepareMoveTarget(
	const CAccioActivity_Base& activity,
	const CAccioBall& ball)
{
	const auto* pPlatform = CGameInstance::Get().
		GetGameObjectByHandleT<CAccioActivity_Platform>(m_hPlatform);
	if (!pPlatform)
		return false;

	BoundingOrientedBox moveArea{};
	if (!pPlatform->GetNpcMoveAreaWorldOBB(moveArea))
		return false;

	const _matrix moveAreaWorld =
		XMMatrixRotationQuaternion(XMLoadFloat4(&moveArea.Orientation)) *
		XMMatrixTranslation(
			moveArea.Center.x,
			moveArea.Center.y,
			moveArea.Center.z);
	_vector determinant{};
	const _matrix inverseMoveArea = XMMatrixInverse(
		&determinant,
		moveAreaWorld);
	if (fabsf(XMVectorGetX(determinant)) <= FLT_EPSILON)
		return false;

	_float3 vBallPosition = ball.GetTransform().GetPosition();
	if (ball.GetRigidBody())
		vBallPosition = ball.GetRigidBody()->GetPosition();

	_float3 localBall{};
	XMStoreFloat3(
		&localBall,
		XMVector3TransformCoord(
			XMLoadFloat3(&vBallPosition),
			inverseMoveArea));

	_float3 localNpc{};
	const _float3 vNpcPosition = GetNpcPosition();
	XMStoreFloat3(
		&localNpc,
		XMVector3TransformCoord(
			XMLoadFloat3(&vNpcPosition),
			inverseMoveArea));

	const _float fUsableHalfX = std::max(
		moveArea.Extents.x - m_fMoveAreaMargin, 0.f);

	// [LSY] 득점 전략은 공과 NPC의 X를 맞춘 직선 최단 경로를 사용한다.
	localNpc.x = localBall.x;
	m_hDisruptionTargetBall = CHandle{};
	m_eTactic = TACTIC::SCORE;
	m_fPlannedPullDuration = m_fScorePullDuration;
	m_fPlannedCollisionDistance = 0.f;
	m_fPlannedTargetEdgeDistance = 0.f;
	const auto attackPlan = BuildAttackPlan(
		activity,
		ball,
		inverseMoveArea,
		fUsableHalfX);
	if (attackPlan)
	{
		// [LSY] 후보 검사 중에는 상태를 건드리지 않고, 완성된 계획만 한 번에 반영한다.
		localNpc.x = attackPlan->fLocalNpcX;
		m_hDisruptionTargetBall = attackPlan->hTargetBall;
		m_eTactic = TACTIC::ATTACK;
		m_fPlannedCollisionDistance = attackPlan->fCollisionDistance;
		m_fPlannedTargetEdgeDistance = attackPlan->fTargetDistanceToEdge;
		m_fPlannedPullDuration = attackPlan->fPullDuration;
	}

	localNpc.x = std::clamp(localNpc.x, -fUsableHalfX, fUsableHalfX);
	localNpc.z = 0.f;
	m_fCurrentMoveSpeed = 0.f;

	XMStoreFloat3(
		&m_vMoveTarget,
		XMVector3TransformCoord(
			XMLoadFloat3(&localNpc),
			moveAreaWorld));
	return true;
}

std::optional<CAccioActivity_NpcController::ATTACK_PLAN>
CAccioActivity_NpcController::BuildAttackPlan(
	const CAccioActivity_Base& activity,
	const CAccioBall& ball,
	const _matrix& inverseMoveArea,
	_float fUsableHalfX) const
{
	_float3 vBallPosition = ball.GetTransform().GetPosition();
	if (ball.GetRigidBody())
		vBallPosition = ball.GetRigidBody()->GetPosition();

	_float3 localBall{};
	XMStoreFloat3(
		&localBall,
		XMVector3TransformCoord(
			XMLoadFloat3(&vBallPosition),
			inverseMoveArea));
	const _matrix moveAreaWorld = XMMatrixInverse(nullptr, inverseMoveArea);

	// [LSY] 50점, 30점 순으로만 검사한다. 10점과 20점 공은 공격 가치가
	// 낮으므로 후보에서 제외하고 NPC 자신의 득점을 우선한다.
	const auto targetHandles = activity.FindScoringBalls(
		CAccioActivity_Base::PARTICIPANT::PLAYER,
		m_iMinimumAttackTargetScore);
	for (const CHandle& hTarget : targetHandles)
	{
		const auto* pTargetBall = CGameInstance::Get().
			GetGameObjectByHandleT<CAccioBall>(hTarget);
		if (!pTargetBall || pTargetBall->GetPendingDestroy())
			continue;

		_float3 vTargetPosition = pTargetBall->GetTransform().GetPosition();
		if (pTargetBall->GetRigidBody())
			vTargetPosition = pTargetBall->GetRigidBody()->GetPosition();

		_float3 localTarget{};
		XMStoreFloat3(
			&localTarget,
			XMVector3TransformCoord(
				XMLoadFloat3(&vTargetPosition),
				inverseMoveArea));
		_float3 localAimTarget = localTarget;
		localAimTarget.x += pTargetBall->GetSphereRadius() *
			m_fCurrentAttackAimOffsetRatio;

		const _float fDeltaZ = localAimTarget.z - localBall.z;
		if (fabsf(fDeltaZ) <= FLT_EPSILON)
			continue;

		// 선택 공 -> 플레이어 공 직선을 NPC 이동선까지 연장했을 때의 위치다.
		const _float fIntersectionRatio = -localBall.z / fDeltaZ;
		if (fIntersectionRatio <= 1.f)
			continue;

		const _float fAttackPositionX = localBall.x +
			(localAimTarget.x - localBall.x) * fIntersectionRatio;
		if (fabsf(fAttackPositionX) > fUsableHalfX)
			continue;

		_float3 vAimTarget{};
		XMStoreFloat3(
			&vAimTarget,
			XMVector3TransformCoord(
				XMLoadFloat3(&localAimTarget),
				moveAreaWorld));
		// [LSY] 목표 공의 중심 대신 좌우로 치우친 지점을 향해 당겨
		// 같은 공격 전술에서도 충돌 방향과 결과가 매번 조금씩 달라진다.
		const _float3 vPushDirection{
			vAimTarget.x - vBallPosition.x,
			0.f,
			vAimTarget.z - vBallPosition.z
		};
		const auto activeBallDistanceToEdge =
			activity.GetDistanceToPlayAreaEdge(ball, vPushDirection);
		if (!activeBallDistanceToEdge)
			continue;

		const auto firstPlayerHit = activity.FindFirstBallOnPath(
			CAccioActivity_Base::PARTICIPANT::PLAYER,
			ball.GetHandle(),
			vBallPosition,
			vPushDirection,
			ball.GetSphereRadius(),
			*activeBallDistanceToEdge);
		if (!firstPlayerHit || firstPlayerHit->hBall != hTarget)
		{
			// 낮은 점수 공이라도 먼저 부딪히면 목표 공을 직접 공격할 수 없다.
			continue;
		}

		const auto npcPathBlocker = activity.FindFirstBallOnPath(
			CAccioActivity_Base::PARTICIPANT::NPC,
			ball.GetHandle(),
			vBallPosition,
			vPushDirection,
			ball.GetSphereRadius(),
			*activeBallDistanceToEdge);
		if (npcPathBlocker)
		{
			// 이미 득점한 NPC 공이 제거 경로에 있으면 자기 공을 보호한다.
			continue;
		}

		const auto distanceToEdge = activity.GetDistanceToPlayAreaEdge(
			*pTargetBall,
			vPushDirection);
		if (!distanceToEdge ||
			*distanceToEdge > m_fMaximumAttackEdgeDistance)
		{
			continue;
		}

		const _float fCollisionDistance =
			firstPlayerHit->fDistanceUntilCollision;
		return ATTACK_PLAN{
			.hTargetBall = hTarget,
			.fLocalNpcX = fAttackPositionX,
			.fCollisionDistance = fCollisionDistance,
			.fTargetDistanceToEdge = *distanceToEdge,
			.fPullDuration = CalculateAttackPullDuration(
				fCollisionDistance,
				*distanceToEdge)
		};
	}

	return std::nullopt;
}

_float CAccioActivity_NpcController::CalculateAttackPullDuration(
	_float fCollisionDistance,
	_float fTargetDistanceToEdge) const
{
	// [LSY] 충돌점까지 도달할 예상시간에, 목표 공이 보드 끝까지 가야 하는
	// 거리만큼 유지시간을 더한다. 먼 공일수록 더 강한 충돌을 준비한다.
	const _float fCollisionTravelTime = std::max(
		fCollisionDistance,
		0.f) / m_fEstimatedAttackPullSpeed;
	const _float fEdgeHoldTime = std::max(
		fTargetDistanceToEdge,
		0.f) * m_fAttackEdgeHoldSecondsPerUnit;
	return std::clamp(
		fCollisionTravelTime + fEdgeHoldTime,
		m_fMinimumAttackPullDuration,
		m_fMaximumAttackPullDuration);
}

_bool CAccioActivity_NpcController::IsAtPreparedMoveTarget() const
{
	const _float3 vPosition = GetNpcPosition();
	const _float fDeltaX = m_vMoveTarget.x - vPosition.x;
	const _float fDeltaZ = m_vMoveTarget.z - vPosition.z;
	return fDeltaX * fDeltaX + fDeltaZ * fDeltaZ <=
		m_fMoveArrivalDistance * m_fMoveArrivalDistance;
}

_bool CAccioActivity_NpcController::MoveToPreparedTarget(_float fTimeDelta)
{
	auto* pNpcCharacter = GetNpcCharacter();
	if (!pNpcCharacter)
		return false;

	pNpcCharacter->SetAction(CAccioActivity_NpcCharacter::ACTION::MOVE);
	const _float3 vPosition = pNpcCharacter->GetTransform().GetPosition();
	const _vector toTarget = XMVectorSet(
		m_vMoveTarget.x - vPosition.x,
		0.f,
		m_vMoveTarget.z - vPosition.z,
		0.f);
	const _float fDistance = XMVectorGetX(XMVector3Length(toTarget));
	if (fDistance <= m_fMoveArrivalDistance)
	{
		pNpcCharacter->SetWorldPosition(_float3{
			m_vMoveTarget.x,
			vPosition.y,
			m_vMoveTarget.z
		});
		m_fCurrentMoveSpeed = 0.f;
		return true;
	}

	const _vector direction = toTarget / std::max(fDistance, FLT_EPSILON);
	const _float fBrakingDistance = std::max(
		fDistance - m_fMoveArrivalDistance, 0.f);
	const _float fBrakingSpeed = sqrtf(
		2.f * m_fMoveDeceleration * fBrakingDistance);
	const _float fDesiredSpeed = std::min(m_fMoveSpeed, fBrakingSpeed);
	const _float fSafeDelta = std::max(fTimeDelta, 0.f);
	if (m_fCurrentMoveSpeed < fDesiredSpeed)
	{
		m_fCurrentMoveSpeed = std::min(
			m_fCurrentMoveSpeed + m_fMoveAcceleration * fSafeDelta,
			fDesiredSpeed);
	}
	else
	{
		m_fCurrentMoveSpeed = std::max(
			m_fCurrentMoveSpeed - m_fMoveDeceleration * fSafeDelta,
			fDesiredSpeed);
	}

	_float3 vMoveDirection{};
	XMStoreFloat3(&vMoveDirection, direction);
	const _float fAppliedSpeed = fSafeDelta > FLT_EPSILON
		? std::min(m_fCurrentMoveSpeed, fDistance / fSafeDelta)
		: 0.f;
	pNpcCharacter->SetMoveIntent(vMoveDirection, fAppliedSpeed);
	FaceTowards(m_vMoveTarget);
	return false;
}

void CAccioActivity_NpcController::UpdateAccioEffects(_float fTimeDelta)
{
	CAccioBall* pBall = GetSelectedBall();
	CAccioActivity_NpcCharacter* pNpcCharacter = GetNpcCharacter();
	const _bool bPullRequested =
		m_eState == STATE::PULLING &&
		pBall && pNpcCharacter &&
		pBall->IsControlledBy(GetParticipantHandle());

	_float4x4 wandWorld{};
	const _bool bHasWand = bPullRequested &&
		pNpcCharacter->TryGetWandSpawnWorldMatrix(wandWorld);
	if (bHasWand)
	{
		const _float3 vStart{ wandWorld._41, wandWorld._42, wandWorld._43 };
		const _float3 vEnd = pBall->GetTransform().GetPosition();
		if (m_iPullEffectID == INVALID_EFFECT_INSTANCE_ID)
		{
			m_fPullEffectBlend = 0.f;
			const CHandle hOwner = GetHandle();
			m_iPullEffectID = CGameInstance::Get().PlayEffect(
				"AccioBallPullNpc",
				wandWorld,
				XMVectorSetW(XMLoadFloat3(&vEnd), 1.f),
				[hOwner](EFFECT_INSTANCE_ID iEffectID, EFFECT_FINISH_REASON)
				{
					auto* pOwner = CGameInstance::Get().
						GetGameObjectByHandleT<CAccioActivity_NpcController>(hOwner);
					if (!pOwner || pOwner->m_iPullEffectID != iEffectID)
						return;
					pOwner->m_iPullEffectID = INVALID_EFFECT_INSTANCE_ID;
					pOwner->m_fPullEffectBlend = 0.f;
				});
		}

		if (m_iPullEffectID != INVALID_EFFECT_INSTANCE_ID)
		{
			CGameInstance::Get().SetBeamPositionsByOwner(
				m_iPullEffectID, vStart, vEnd);
			m_fPullEffectBlend = std::min(
				1.f,
				m_fPullEffectBlend +
					fTimeDelta / PULL_EFFECT_FADE_IN_TIME);
		}
	}
	else if (m_iPullEffectID != INVALID_EFFECT_INSTANCE_ID)
	{
		m_fPullEffectBlend = std::max(
			0.f,
			m_fPullEffectBlend -
				fTimeDelta / PULL_EFFECT_FADE_OUT_TIME);
	}

	if (m_iPullEffectID != INVALID_EFFECT_INSTANCE_ID)
	{
		const _float fBlend = m_fPullEffectBlend * m_fPullEffectBlend *
			(3.f - 2.f * m_fPullEffectBlend);
		CGameInstance::Get().ChangeEffectColorByOwner(
			m_iPullEffectID, { 1.f, 1.f, 0.6f, fBlend });

		if (!bHasWand && m_fPullEffectBlend <= 0.f)
		{
			const EFFECT_INSTANCE_ID iEffectID = m_iPullEffectID;
			m_iPullEffectID = INVALID_EFFECT_INSTANCE_ID;
			CGameInstance::Get().StopEffect(iEffectID);
		}
	}

	_float4x4 ballWorld{};
	if (bPullRequested)
	{
		const _float3 vBallPosition = pBall->GetTransform().GetPosition();
		XMStoreFloat4x4(
			&ballWorld,
			XMMatrixTranslation(
				vBallPosition.x,
				vBallPosition.y,
				vBallPosition.z));

		if (m_iGrabEffectID == INVALID_EFFECT_INSTANCE_ID)
		{
			m_fGrabEffectBlend = 0.f;
			const CHandle hOwner = GetHandle();
			m_iGrabEffectID = CGameInstance::Get().PlayEffect(
				"AccioBallGrab",
				ballWorld,
				_vector{},
				[hOwner](EFFECT_INSTANCE_ID iEffectID, EFFECT_FINISH_REASON)
				{
					auto* pOwner = CGameInstance::Get().
						GetGameObjectByHandleT<CAccioActivity_NpcController>(hOwner);
					if (!pOwner || pOwner->m_iGrabEffectID != iEffectID)
						return;
					pOwner->m_iGrabEffectID = INVALID_EFFECT_INSTANCE_ID;
					pOwner->m_fGrabEffectBlend = 0.f;
				});
		}

		if (m_iGrabEffectID != INVALID_EFFECT_INSTANCE_ID)
		{
			CGameInstance::Get().SetEffectWorldMatrix(
				m_iGrabEffectID, ballWorld);
			m_fGrabEffectBlend = std::min(
				1.f,
				m_fGrabEffectBlend +
					fTimeDelta / GRAB_EFFECT_FADE_IN_TIME);
		}
	}
	else if (m_iGrabEffectID != INVALID_EFFECT_INSTANCE_ID)
	{
		m_fGrabEffectBlend = std::max(
			0.f,
			m_fGrabEffectBlend -
				fTimeDelta / GRAB_EFFECT_FADE_OUT_TIME);
	}

	if (m_iGrabEffectID != INVALID_EFFECT_INSTANCE_ID)
	{
		const _float fBlend = m_fGrabEffectBlend * m_fGrabEffectBlend *
			(3.f - 2.f * m_fGrabEffectBlend);
		CGameInstance::Get().ChangeEffectColorByOwner(
			m_iGrabEffectID,
			{ 1.f, 1.f, 0.f, GRAB_EFFECT_MAX_ALPHA * fBlend });

		if (!bPullRequested && m_fGrabEffectBlend <= 0.f)
		{
			const EFFECT_INSTANCE_ID iEffectID = m_iGrabEffectID;
			m_iGrabEffectID = INVALID_EFFECT_INSTANCE_ID;
			CGameInstance::Get().StopEffect(iEffectID);
		}
	}
}

void CAccioActivity_NpcController::StopAccioEffects()
{
	if (m_iPullEffectID != INVALID_EFFECT_INSTANCE_ID)
	{
		const EFFECT_INSTANCE_ID iEffectID = m_iPullEffectID;
		m_iPullEffectID = INVALID_EFFECT_INSTANCE_ID;
		CGameInstance::Get().StopEffect(iEffectID);
	}
	if (m_iGrabEffectID != INVALID_EFFECT_INSTANCE_ID)
	{
		const EFFECT_INSTANCE_ID iEffectID = m_iGrabEffectID;
		m_iGrabEffectID = INVALID_EFFECT_INSTANCE_ID;
		CGameInstance::Get().StopEffect(iEffectID);
	}
	m_fPullEffectBlend = 0.f;
	m_fGrabEffectBlend = 0.f;
}

void CAccioActivity_NpcController::FaceTowards(const _float3& vWorldPosition)
{
	if (auto* pNpcCharacter = GetNpcCharacter())
		pNpcCharacter->FaceTowards(vWorldPosition);
}

_bool CAccioActivity_NpcController::AcquireSelectedBall()
{
	CAccioBall* pBall = GetSelectedBall();
	return pBall && pBall->TryAcquireControl(GetParticipantHandle());
}

_bool CAccioActivity_NpcController::ReleaseSelectedBall()
{
	CAccioBall* pBall = GetSelectedBall();
	return pBall && pBall->ReleaseControl(GetParticipantHandle());
}

CAccioBall* CAccioActivity_NpcController::GetSelectedBall() const
{
	return CGameInstance::Get().GetGameObjectByHandleT<CAccioBall>(m_hActiveBall);
}

CAccioBall* CAccioActivity_NpcController::GetDisruptionTargetBall() const
{
	return CGameInstance::Get().
		GetGameObjectByHandleT<CAccioBall>(m_hDisruptionTargetBall);
}

CAccioActivity_NpcCharacter* CAccioActivity_NpcController::GetNpcCharacter() const
{
	return CGameInstance::Get().
		GetGameObjectByHandleT<CAccioActivity_NpcCharacter>(m_hNpcCharacter);
}

CHandle CAccioActivity_NpcController::GetParticipantHandle() const
{
	return GetNpcCharacter() ? m_hNpcCharacter : CHandle{};
}

_float3 CAccioActivity_NpcController::GetNpcPosition() const
{
	if (const auto* pNpcCharacter = GetNpcCharacter())
		return pNpcCharacter->GetTransform().GetPosition();
	return GetTransform().GetPosition();
}

void CAccioActivity_NpcController::LateUpdate(_float)
{
	GetTransform().Update();
	if (m_bDebugDraw)
		DrawDebugState();
}

void CAccioActivity_NpcController::UpdateGUI()
{
	CGameObject::UpdateGUI();

	ImGui::Text("State: %s", GetStateName(m_eState));
	ImGui::Text("State Time: %.2f", m_fStateElapsed);
	if (ImGui::CollapsingHeader(
		"Dialogue Camera",
		ImGuiTreeNodeFlags_DefaultOpen))
	{
		_bool bCameraChanged = false;
		bCameraChanged |= ImGui::DragFloat3(
			"Player Local Offset",
			&m_vPlayerDialogueOffset.x,
			0.05f);
		bCameraChanged |= ImGui::DragFloat3(
			"Camera Right Height Front",
			&m_vDialogueCameraOffset.x,
			0.05f);
		bCameraChanged |= ImGui::DragFloat(
			"Camera Target Height",
			&m_fDialogueCameraTargetHeight,
			0.05f,
			0.f,
			10.f,
			"%.2f");
		bCameraChanged |= ImGui::DragFloat(
			"Dialogue Camera FOV",
			&m_fDialogueCameraFovY,
			0.25f,
			20.f,
			100.f,
			"%.1f");
		m_fDialogueCameraFovY = std::clamp(
			m_fDialogueCameraFovY, 20.f, 100.f);

		if (bCameraChanged && m_bDialogueCinematicPlaying)
		{
			// [LSY] 대화 중 저작 값을 바꾸면 시네마틱을 즉시 재생성해 현재 화면에서 구도를 맞춘다.
			EndDialogueCamera();
			BeginDialogueCamera();
		}
	}
	if (m_eTactic == TACTIC::ATTACK)
		ImGui::Text("Tactic: Attack");
	else
		ImGui::Text("Tactic: Score");
	ImGui::Text("Planned Pull Time: %.2f", m_fPlannedPullDuration);
	const _float fEffectivePullTime = std::min(
		std::max(m_fPlannedPullDuration - m_fReleaseLeadTime, 0.1f),
		m_fPullTimeout);
	ImGui::Text("Effective Pull Time: %.2f", fEffectivePullTime);
	if (m_fPullTimeout < m_fPlannedPullDuration)
	{
		ImGui::TextColored(
			{ 1.f, 0.75f, 0.2f, 1.f },
			"Safety timeout shortens this plan");
	}
	if (m_eTactic == TACTIC::ATTACK)
	{
		ImGui::Text(
			"Collision Distance: %.2f",
			m_fPlannedCollisionDistance);
		ImGui::Text(
			"Target To Edge: %.2f",
			m_fPlannedTargetEdgeDistance);
		ImGui::Text(
			"Attack Aim Offset: %+.2f radius",
			m_fCurrentAttackAimOffsetRatio);
	}
	ImGui::Checkbox("Debug Draw", &m_bDebugDraw);
	if (GetDisruptionTargetBall())
		ImGui::Text("Disruption Target: Player Ball");
	else
		ImGui::Text("Disruption Target: None");
	ImGui::DragFloat(
		"Turn Start Delay", &m_fTurnStartDelay,
		0.05f, 0.f, 5.f, "%.2f");
	ImGui::DragFloat("Move Speed", &m_fMoveSpeed, 0.1f, 0.1f, 20.f, "%.1f");
	ImGui::DragFloat(
		"Move Acceleration", &m_fMoveAcceleration,
		0.1f, 0.1f, 30.f, "%.1f");
	ImGui::DragFloat(
		"Move Deceleration", &m_fMoveDeceleration,
		0.1f, 0.1f, 40.f, "%.1f");
	ImGui::DragFloat(
		"Move Arrival Distance", &m_fMoveArrivalDistance,
		0.01f, 0.01f, 2.f, "%.2f");
	ImGui::DragFloat(
		"Move Area Margin", &m_fMoveAreaMargin, 0.05f, 0.f, 5.f, "%.2f");
	ImGui::DragFloat("Aim Delay", &m_fAimDelay, 0.05f, 0.f, 5.f, "%.2f");
	ImGui::DragFloat(
		"Score Pull Duration", &m_fScorePullDuration,
		0.05f, 0.1f, 3.f, "%.2f");
	ImGui::DragFloat(
		"Maximum Attack Edge Distance", &m_fMaximumAttackEdgeDistance,
		0.1f, 0.f, 30.f, "%.1f");
	ImGui::DragFloat(
		"Estimated Attack Pull Speed", &m_fEstimatedAttackPullSpeed,
		0.1f, 0.1f, 40.f, "%.1f");
	ImGui::DragFloat(
		"Attack Edge Hold Seconds Per Unit",
		&m_fAttackEdgeHoldSecondsPerUnit,
		0.005f, 0.f, 0.2f, "%.3f");
	ImGui::DragFloat(
		"Attack Aim Offset Ratio",
		&m_fAttackAimOffsetRatio,
		0.01f, 0.f, 0.9f, "%.2f");
	ImGui::DragFloat(
		"Minimum Attack Pull Duration",
		&m_fMinimumAttackPullDuration,
		0.05f, 0.1f, m_fMaximumAttackPullDuration, "%.2f");
	m_fMaximumAttackPullDuration = std::max(
		m_fMaximumAttackPullDuration,
		m_fMinimumAttackPullDuration);
	ImGui::DragFloat(
		"Maximum Attack Pull Duration",
		&m_fMaximumAttackPullDuration,
		0.05f, m_fMinimumAttackPullDuration, 5.f, "%.2f");
	ImGui::DragFloat(
		"Pull Safety Timeout", &m_fPullTimeout, 0.1f, 0.1f, 20.f, "%.1f");
	ImGui::DragFloat(
		"Release Lead Time", &m_fReleaseLeadTime,
		0.01f, 0.f, 1.f, "%.2f");
	ImGui::SliderInt(
		"Minimum Attack Target Score",
		&m_iMinimumAttackTargetScore,
		30,
		50);
	SanitizeTuning();
}

void CAccioActivity_NpcController::DrawDebugState() const
{
	auto* pDebug = CGameInstance::Get().GetDbgLineRender();
	if (!pDebug)
		return;

	const _float4 vPreviousColor = pDebug->GetColor();
	const auto ePreviousDepthMode = pDebug->GetDepthMode();
	_float4 vStateColor{ 1.f, 0.2f, 0.2f, 1.f };
	if (m_eState == STATE::AIMING)
		vStateColor = { 1.f, 0.85f, 0.15f, 1.f };
	else if (m_eState == STATE::MOVING)
		vStateColor = { 0.25f, 1.f, 0.25f, 1.f };
	else if (m_eState == STATE::PULLING)
		vStateColor = { 1.f, 0.1f, 0.8f, 1.f };
	else if (m_eState == STATE::PULL_RECOVERY)
		vStateColor = { 0.8f, 0.35f, 1.f, 1.f };
	else if (m_eState == STATE::ENTERING_MATCH)
		vStateColor = { 0.2f, 0.9f, 0.45f, 1.f };
	else if (m_eState == STATE::RETURNING)
		vStateColor = { 0.2f, 1.f, 0.75f, 1.f };
	else if (m_eState == STATE::LEAVING_MATCH)
		vStateColor = { 0.25f, 0.65f, 1.f, 1.f };
	else if (m_eState == STATE::WAIT_BALL_SETTLED)
		vStateColor = { 0.3f, 0.8f, 1.f, 1.f };

	pDebug->SetDepthTest(false);
	pDebug->SetColor(vStateColor);
	const _float3 vPosition = GetNpcPosition();
	pDebug->AddSphere(0.75f, XMMatrixTranslation(
		vPosition.x, vPosition.y + 1.f, vPosition.z));
	pDebug->AddCross(vPosition, 0.5f);
	if (m_eState == STATE::MOVING ||
		m_eState == STATE::ENTERING_MATCH ||
		m_eState == STATE::RETURNING ||
		m_eState == STATE::LEAVING_MATCH)
	{
		pDebug->AddCross(m_vMoveTarget, 0.65f);
		pDebug->AddLine(vPosition, m_vMoveTarget);
	}

	if (const CAccioBall* pBall = GetSelectedBall())
	{
		const _float3 vBallPosition = pBall->GetTransform().GetPosition();
		pDebug->AddLine(vPosition, vBallPosition);
	}

	if (const CAccioBall* pTargetBall = GetDisruptionTargetBall())
	{
		_float3 vTargetPosition = pTargetBall->GetTransform().GetPosition();
		if (pTargetBall->GetRigidBody())
			vTargetPosition = pTargetBall->GetRigidBody()->GetPosition();
		pDebug->SetColor({ 0.2f, 0.85f, 1.f, 1.f });
		pDebug->AddCross(vTargetPosition, 0.9f);
		pDebug->AddLine(m_vMoveTarget, vTargetPosition);
	}

	pDebug->SetColor(vPreviousColor);
	pDebug->SetDepthMode(ePreviousDepthMode);
}

const _char* CAccioActivity_NpcController::GetStateName(STATE eState)
{
	switch (eState)
	{
	case STATE::IDLE:
		return "Idle";
	case STATE::MOVING:
		return "Moving";
	case STATE::AIMING:
		return "Aiming";
	case STATE::PULLING:
		return "Pulling";
	case STATE::PULL_RECOVERY:
		return "Pull Recovery";
	case STATE::ENTERING_MATCH:
		return "Entering Match";
	case STATE::RETURNING:
		return "Returning";
	case STATE::LEAVING_MATCH:
		return "Leaving Match";
	case STATE::WAIT_BALL_SETTLED:
		return "Wait Ball Settled";
	default:
		return "Unknown";
	}
}

UPtr<CAccioActivity_NpcController> CAccioActivity_NpcController::Create()
{
	auto pInstance = ToUPtr(new CAccioActivity_NpcController{});
	if (FAILED(pInstance->InitializePrototype()))
		return nullptr;
	return pInstance;
}

UPtr<CPrototype> CAccioActivity_NpcController::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CAccioActivity_NpcController{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
		return nullptr;
	return pInstance;
}

void CAccioActivity_NpcController::Free()
{
	SyncInteractionPrompt(false);
	EndDialogueCamera();
	SetPlayerMovementLocked(false);
	StopAccioEffects();
	CGameObject::Free();
}
