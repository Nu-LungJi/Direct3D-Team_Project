#pragma once

#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Client)

class CAccioActivity_Base;
class CAccioBall;
class CAccioActivity_Platform;
class CAccioActivity_NpcCharacter;

// [LSY] 아씨오 액티비티의 NPC 턴만 담당한다.
// 실제 공 이동과 점수 판정은 각각 CAccioBall과 CAccioActivity_Base가 수행한다.
class CAccioActivity_NpcController final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CAccioActivity_NpcController, CGameObject)

	enum class STATE : uint8_t
	{
		IDLE,
		MOVING,
		AIMING,
		PULLING,
		PULL_RECOVERY,
		ENTERING_MATCH,
		RETURNING,
		LEAVING_MATCH,
		WAIT_BALL_SETTLED
	};

	enum class TACTIC : uint8_t
	{
		SCORE,
		ATTACK
	};

	struct DIALOGUE_LINE
	{
		_string Text{};
		_string ExpressionAnim{};
		_bool LoopExpression{ true };
	};

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		CHandle hActivity{};
		CHandle hPlatform{};
		CHandle hNpcCharacter{};
		CHandle hInteractionPlayer{};
		_float3 vInitialPosition{};
		_float3 vInitialRotation{};
		_string SpeakerName{ "NPC" };
		// 매치 시작 전, 플레이어 승리, NPC 승리, 무승부 순서의 대화 데이터.
		std::vector<DIALOGUE_LINE> Dialogue{};
		std::vector<DIALOGUE_LINE> PlayerWinDialogue{};
		std::vector<DIALOGUE_LINE> NpcWinDialogue{};
		std::vector<DIALOGUE_LINE> DrawDialogue{};
		_float fInteractionDistance{ 3.f };
		_bool bRepeatDialogue{};
		_float fDialogueFadeDuration{ 0.35f };
		_float fDialogueFadeHoldDuration{ 0.2f };
		// NPC 캐릭터 로컬 축 기준: x=오른쪽, y=높이, z=앞쪽.
		_float3 vPlayerDialogueOffset{ 0.f, 0.f, 4.2f };
		// NPC 로컬 축 기준 얼굴 클로즈업: x=좌우, y=카메라 높이, z=NPC 앞 거리.
		_float3 vDialogueCameraOffset{ 0.f, 2.9f, 3.5f };
		_float fDialogueCameraTargetHeight{ 2.65f };
		_float fDialogueCameraFovY{ 32.f };
		_float fTurnStartDelay{ 1.f };
		_float fMoveSpeed{ 5.f };
		_float fMoveAcceleration{ 9.f };
		_float fMoveDeceleration{ 14.f };
		_float fMoveArrivalDistance{ 0.1f };
		_float fMoveAreaMargin{ 0.75f };
		_float fSideStandbyInset{ 1.5f };
		_float fMatchRestBackwardOffset{ 2.5f };
		_float fAimDelay{ 0.6f };
		_float fScorePullDuration{ 1.35f };
		_float fMaximumAttackEdgeDistance{ 14.f };
		_float fEstimatedAttackPullSpeed{ 18.f };
		_float fAttackEdgeHoldSecondsPerUnit{ 0.04f };
		_float fAttackAimOffsetRatio{ 0.55f };
		_float fMinimumAttackPullDuration{ 1.f };
		_float fMaximumAttackPullDuration{ 2.5f };
		_float fPullTimeout{ 3.f };
		_float fReleaseLeadTime{ 0.12f };
		int32_t iMinimumAttackTargetScore{ 30 };
		_bool bDebugDraw{ true };
	};

private:
	CAccioActivity_NpcController();
	CAccioActivity_NpcController(const CAccioActivity_NpcController& prototype);
	~CAccioActivity_NpcController() override = default;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void FixedUpdate(_float fTimeDelta) override;
	void Update(_float fTimeDelta) override;
	void LateUpdate(_float fTimeDelta) override;
	void UpdateGUI() override;

	void SetInteractionPlayerHandle(const CHandle& hPlayer);
	void BeginDialogue();
	void AdvanceDialogue();
	void CancelDialogue();

protected:
	void OnRegisteredToManager() override;
	void Free() override;

private:
	enum class DIALOGUE_PURPOSE : uint8_t
	{
		NONE,
		START_MATCH,
		MATCH_RESULT
	};

	struct ATTACK_PLAN
	{
		CHandle hTargetBall{};
		_float fLocalNpcX{};
		_float fCollisionDistance{};
		_float fTargetDistanceToEdge{};
		_float fPullDuration{};
	};

	void UpdateNpcTurn(CAccioActivity_Base& activity, _float fTimeDelta);
	void AbortNpcTurn(CAccioActivity_Base& activity);
	void UpdateDialogue(_float fTimeDelta);
	void BeginDialogueSequence(
		const std::vector<DIALOGUE_LINE>& dialogue,
		DIALOGUE_PURPOSE ePurpose);
	void BeginMatchEndDialogue(const CAccioActivity_Base& activity);
	void UpdateDialogueIntro(_float fTimeDelta);
	void ShowFirstDialogueLine();
	void FinishDialogue();
	void BeginDialogueCamera();
	void EndDialogueCamera();
	void SetPlayerMovementLocked(_bool bLocked);
	void SetDialogueExpression(const DIALOGUE_LINE& line);
	void HandleDialogueLineSound(const DIALOGUE_LINE& line) const;
	void ResolveInteractionPlayer();
	_bool IsInteractionPlayerInRange();
	void SyncInteractionPrompt(_bool bShow);
	void UpdatePullRecovery();
	_bool BeginEnterMatch();
	_bool BeginReturnToRest();
	_bool BeginLeaveMatch();
	_bool BeginPlatformPath(
		const _float3& vLocalTarget,
		const _float3& vLocalFacingTarget,
		STATE ePathState);
	void UpdatePlatformPath(_float fTimeDelta);
	_bool MoveAlongReturnPath(_float fTimeDelta);
	_float3 EvaluateReturnPath(_float fRatio) const;
	void ResetTurnState();
	void UpdateAccioEffects(_float fTimeDelta);
	void StopAccioEffects();
	void SanitizeTuning();
	_bool PrepareMoveTarget(
		const CAccioActivity_Base& activity,
		const CAccioBall& ball);
	std::optional<ATTACK_PLAN> BuildAttackPlan(
		const CAccioActivity_Base& activity,
		const CAccioBall& ball,
		const _matrix& inverseMoveArea,
		_float fUsableHalfX) const;
	_float CalculateAttackPullDuration(
		_float fCollisionDistance,
		_float fTargetDistanceToEdge) const;
	_bool IsAtPreparedMoveTarget() const;
	_bool MoveToPreparedTarget(_float fTimeDelta);
	void FaceTowards(const _float3& vWorldPosition);
	_bool AcquireSelectedBall();
	_bool ReleaseSelectedBall();
	CAccioBall* GetSelectedBall() const;
	CAccioBall* GetDisruptionTargetBall() const;
	CAccioActivity_NpcCharacter* GetNpcCharacter() const;
	CHandle GetParticipantHandle() const;
	_float3 GetNpcPosition() const;
	void DrawDebugState() const;
	static const _char* GetStateName(STATE eState);

public:
	static UPtr<CAccioActivity_NpcController> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	CHandle m_hActivity{};
	CHandle m_hPlatform{};
	CHandle m_hNpcCharacter{};
	CHandle m_hInteractionPlayer{};
	CHandle m_hActiveBall{};
	CHandle m_hDisruptionTargetBall{};
	STATE m_eState{ STATE::IDLE };
	TACTIC m_eTactic{ TACTIC::SCORE };
	_float3 m_vMoveTarget{};
	_float3 m_vRestFacingTarget{};
	_float3 m_vReturnPathStart{};
	_float3 m_vReturnPathControlA{};
	_float3 m_vReturnPathControlB{};
	_float3 m_vReturnPathEnd{};
	_string m_sSpeakerName{ "NPC" };
	std::vector<DIALOGUE_LINE> m_Dialogue{};
	std::vector<DIALOGUE_LINE> m_PlayerWinDialogue{};
	std::vector<DIALOGUE_LINE> m_NpcWinDialogue{};
	std::vector<DIALOGUE_LINE> m_DrawDialogue{};
	std::vector<DIALOGUE_LINE> m_ActiveDialogue{};
	size_t m_iDialogueIndex{};
	_float m_fInteractionDistance{ 3.f };
	_float m_fDialogueFadeDuration{ 0.35f };
	_float m_fDialogueFadeHoldDuration{ 0.2f };
	_float m_fDialogueIntroElapsed{};
	_float3 m_vPlayerDialogueOffset{ 0.f, 0.f, 4.2f };
	_float3 m_vDialogueCameraOffset{ 0.f, 2.9f, 3.5f };
	_float m_fDialogueCameraTargetHeight{ 2.65f };
	_float m_fDialogueCameraFovY{ 32.f };
	_float m_fStateElapsed{};
	_float m_fReturnPathElapsed{};
	_float m_fReturnPathDuration{ 0.1f };
	_float m_fTurnStartDelay{ 1.f };
	_float m_fMoveSpeed{ 5.f };
	_float m_fMoveAcceleration{ 9.f };
	_float m_fMoveDeceleration{ 14.f };
	_float m_fCurrentMoveSpeed{};
	_float m_fMoveArrivalDistance{ 0.1f };
	_float m_fMoveAreaMargin{ 0.75f };
	_float m_fSideStandbyInset{ 1.5f };
	_float m_fMatchRestBackwardOffset{ 2.5f };
	_float m_fAimDelay{ 0.6f };
	_float m_fScorePullDuration{ 1.35f };
	_float m_fMaximumAttackEdgeDistance{ 14.f };
	_float m_fEstimatedAttackPullSpeed{ 18.f };
	_float m_fAttackEdgeHoldSecondsPerUnit{ 0.04f };
	_float m_fAttackAimOffsetRatio{ 0.55f };
	_float m_fCurrentAttackAimOffsetRatio{};
	_float m_fMinimumAttackPullDuration{ 1.f };
	_float m_fMaximumAttackPullDuration{ 2.5f };
	_float m_fPullTimeout{ 3.f };
	_float m_fReleaseLeadTime{ 0.12f };
	_float m_fPlannedPullDuration{ 1.35f };
	_float m_fPlannedCollisionDistance{};
	_float m_fPlannedTargetEdgeDistance{};
	int32_t m_iMinimumAttackTargetScore{ 30 };
	_bool m_bHasRestFacingTarget{};
	_bool m_bReturnPathReady{};
	_bool m_bMatchEntered{};
	_bool m_bAtSideStandby{ true };
	_bool m_bRepeatDialogue{};
	_bool m_bTalking{};
	_bool m_bDialogueCompleted{};
	_bool m_bMatchEndDialogueCompleted{};
	_bool m_bInteractionPromptVisible{};
	_bool m_bDialogueCinematicPlaying{};
	_bool m_bDebugDraw{ true };
	EFFECT_INSTANCE_ID m_iPullEffectID{ INVALID_EFFECT_INSTANCE_ID };
	EFFECT_INSTANCE_ID m_iGrabEffectID{ INVALID_EFFECT_INSTANCE_ID };
	_float m_fPullEffectBlend{};
	_float m_fGrabEffectBlend{};

	static constexpr _float PULL_EFFECT_FADE_IN_TIME = 0.1f;
	static constexpr _float PULL_EFFECT_FADE_OUT_TIME = 0.35f;
	static constexpr _float GRAB_EFFECT_FADE_IN_TIME = 0.25f;
	static constexpr _float GRAB_EFFECT_FADE_OUT_TIME = 0.5f;
	static constexpr _float GRAB_EFFECT_MAX_ALPHA = 0.5882353f;

	enum class CONVERSATION_PHASE : uint8_t
	{
		IDLE,
		FADING_OUT,
		HOLDING_BLACK,
		FADING_IN,
		TALKING
	};
	CONVERSATION_PHASE m_eConversationPhase{ CONVERSATION_PHASE::IDLE };

	DIALOGUE_PURPOSE m_eDialoguePurpose{ DIALOGUE_PURPOSE::NONE };
};

NS_END
