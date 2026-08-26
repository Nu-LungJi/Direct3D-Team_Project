#pragma once
#include "WorldAgent.h"

NS_BEGIN(Client)

// 대화를 끝낸 뒤 스펠 미니게임을 시작하는 전용 NPC.
// ExpressionAnim에는 현재 NPC 모델에 실제로 존재하는 애니메이션 클립 이름을 넣는다.
class CMiniGameNpc final : public CWorldAgent
{
public:
	enum class STATE
	{
		IDLE,
		DIALOGUE_INTRO,
		TALKING,
		MOVING,
		MINIGAME
	};

	enum class OUTCOME
	{
		NONE,
		MOVE_TO_DESTINATION,
		SPELL_MINIGAME
	};

	struct DIALOGUE_LINE
	{
		_string Text{};
		_string ExpressionAnim{};
		_bool LoopExpression{ true };
	};

	struct DESC : public WORLD_AGENT_DESC
	{
		_string SpeakerName{ "NPC" };
		std::vector<DIALOGUE_LINE> Dialogue{};
		_string IdleExpressionAnim{};
		_float InteractionDistance{ 3.f };
		_bool SecondSpellMiniGame{ false };
		_bool Repeatable{ false };
		OUTCOME Outcome{ OUTCOME::SPELL_MINIGAME };
		_float FadeDuration{ 0.35f };
		_float FadeHoldDuration{ 0.2f };
		// NPC 로컬 축 기준: x=오른쪽, y=높이, z=앞쪽.
		_float3 PlayerDialogueOffset{ -0.8f, 0.f, 2.2f };
		// 플레이어 기준 오버숄더 카메라: x=오른쪽, y=높이, z=NPC 반대쪽.
		_float3 DialogueCameraOffset{ 0.55f, 1.65f, 1.15f };
		_float DialogueCameraFovY{ 50.f };
		_float3 MoveDestination{};
		_float MoveSpeed{ 2.f };
		_float MoveStopDistance{ 0.2f };
	};

public:
	DECLARE_DERIVED_TYPE(CMiniGameNpc, CWorldAgent)

private:
	CMiniGameNpc() = default;
	~CMiniGameNpc() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void FixedUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;

	void BeginDialogue();
	void AdvanceDialogue();
	void CancelDialogue();
	_bool IsTalking() const { return m_bTalking; }
	STATE GetState() const { return m_eState; }

	static E::UPtr<CMiniGameNpc> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;

private:
	void SetExpression(const _string& animName, _bool loop);
	void FinishDialogue();
	void UpdateDialogueIntro(_float fTimeDelta);
	void ShowFirstDialogueLine();
	void BeginDialogueCamera();
	void EndDialogueCamera();
	void SetPlayerMovementLocked(_bool locked);
	void ExecuteOutcome();
	void UpdateMoveOutcome();
	void BeginMiniGameWorldPause();
	void UpdateMiniGameWorldPause();
	void EndMiniGameWorldPause();
	_bool IsSpellMiniGameRunning() const;
	void ResolvePlayerHandle();
	_bool IsPlayerInRange();
	void SyncInteractionPrompt(_bool show);

private:
	_string m_SpeakerName{ "NPC" };
	std::vector<DIALOGUE_LINE> m_Dialogue{};
	_string m_IdleExpressionAnim{};
	_float m_fInteractionDistance{ 3.f };
	_bool m_bSecondSpellMiniGame{};
	_bool m_bRepeatable{};
	_bool m_bTalking{};
	_bool m_bCompleted{};
	_bool m_bPromptVisible{};
	_bool m_bMovingToDestination{};
	_bool m_bOwnsWorldPause{};
	_bool m_bDialogueCinematicPlaying{};
	size_t m_iDialogueIndex{};
	CHandle m_hInteractionPlayer{};
	std::optional<CHandle> m_hDialogueFade{};
	STATE m_eState{ STATE::IDLE };
	OUTCOME m_eOutcome{ OUTCOME::SPELL_MINIGAME };
	_float m_fFadeDuration{ 0.35f };
	_float m_fFadeHoldDuration{ 0.2f };
	_float m_fIntroElapsed{}; 
	_float3 m_vPlayerDialogueOffset{ -0.8f, 0.f, 2.2f };
	_float3 m_vDialogueCameraOffset{ 0.55f, 1.65f, 1.15f };
	_float m_fDialogueCameraFovY{ 50.f };
	_float3 m_vMoveDestination{};
	_float m_fMoveSpeed{ 2.f };
	_float m_fMoveStopDistance{ 0.2f };

	enum class CONVERSATION_PHASE
	{
		IDLE,
		FADING_OUT,
		HOLDING_BLACK,
		FADING_IN,
		TALKING
	};
	CONVERSATION_PHASE m_eConversationPhase{ CONVERSATION_PHASE::IDLE };
};

NS_END
