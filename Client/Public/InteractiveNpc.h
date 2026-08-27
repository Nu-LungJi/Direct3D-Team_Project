#pragma once
#include "WorldAgent.h"

NS_BEGIN(Client)

// 플레이어와 대화하고, 대화 결과에 따라 이동하거나 미니게임을 시작하는 상호작용 NPC.
// ExpressionAnim에는 현재 NPC 모델에 실제로 존재하는 애니메이션 클립 이름을 넣는다.
class CInteractiveNpc final : public CWorldAgent
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
		// Resources/json/Cinematics에서 불러와 재생할 대화 카메라 JSON 이름.
		_string DialogueCinematicName{ "InteractiveNpcDialogue" };
		_float3 MoveDestination{};
		_float MoveSpeed{ 2.f };
		_float MoveStopDistance{ 0.2f };
	};

public:
	DECLARE_DERIVED_TYPE(CInteractiveNpc, CWorldAgent)

private:
	// 프로토타입 및 복제 생성에서만 객체를 만들 수 있도록 생성자를 제한한다.
	CInteractiveNpc() = default;
	// 남아 있는 상호작용 UI, 페이드 UI와 시간 정지 요청을 정리한다.
	~CInteractiveNpc() override;

public:
	// 상호작용 NPC 프로토타입이 공통으로 사용할 리소스를 준비한다.
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	// 설명자에 따라 모델, 충돌, 대화, 이동 결과 상태를 초기화한다.
	HRESULT Initialize(void* pArg) override;
	// 고정 물리 프레임마다 캐릭터 모터와 중력 이동을 처리한다.
	void FixedUpdate(E::_float fTimeDelta) override;
	// 거리 판정, 입력, 대화 연출, 이동 및 미니게임 상태를 갱신한다.
	void Update(E::_float fTimeDelta) override;

	// 상호작용을 시작하고 화면 암전 및 대화 준비 상태로 전환한다.
	void BeginDialogue();
	// 현재 대사를 넘기고 마지막 대사라면 설정된 결과를 실행한다.
	void AdvanceDialogue();
	// 진행 중인 대화를 취소하고 카메라와 UI, 플레이어 제어를 복구한다.
	void CancelDialogue();
	// 현재 대화 진행 여부를 반환한다.
	_bool IsTalking() const { return m_bTalking; }
	// 현재 상호작용 NPC 상태를 반환한다.
	STATE GetState() const { return m_eState; }

	// 상호작용 NPC 프로토타입 객체를 생성한다.
	static E::UPtr<CInteractiveNpc> Create();
	// 전달된 설명자로 실제 상호작용 NPC 인스턴스를 복제한다.
	E::UPtr<E::CPrototype> Clone(void* pArg) override;

private:
	// 이름으로 표정 또는 대화 애니메이션을 재생한다.
	void SetExpression(const _string& animName, _bool loop);
	// 대화를 종료하고 설정된 이동 또는 미니게임 결과로 전환한다.
	void FinishDialogue();
	// 암전, 플레이어 이동, 카메라 전환, 화면 복귀 순서를 갱신한다.
	void UpdateDialogueIntro(_float fTimeDelta);
	// 첫 대사와 해당 표정 애니메이션을 화면에 표시한다.
	void ShowFirstDialogueLine();
	// 플레이어를 NPC 앞에 배치하고 대화용 시네마틱 카메라를 시작한다.
	void BeginDialogueCamera();
	// 대화용 시네마틱을 종료하고 기존 플레이어 카메라로 복귀한다.
	void EndDialogueCamera();
	// 대화 중 플레이어 이동 입력의 잠금 상태를 변경한다.
	void SetPlayerMovementLocked(_bool locked);
	// 설명자에 지정된 대화 종료 결과를 실행한다.
	void ExecuteOutcome();
	// 완전 암전 후 플레이어를 목적지로 옮기고 화면을 복구한다.
	void UpdateMoveOutcome();
	// 스펠 미니게임 동안 UI를 제외한 월드 시간을 정지한다.
	void BeginMiniGameWorldPause();
	// 미니게임 종료를 감지해 월드와 카메라 및 UI를 복구한다.
	void UpdateMiniGameWorldPause();
	// 이 NPC가 요청한 월드 시간 정지를 해제한다.
	void EndMiniGameWorldPause();
	// 현재 스펠 미니게임 객체가 실행 중인지 확인한다.
	_bool IsSpellMiniGameRunning() const;
	// 상호작용 대상 플레이어 핸들이 없으면 월드에서 다시 찾는다.
	void ResolvePlayerHandle();
	// 플레이어가 설정된 상호작용 거리 안에 있는지 확인한다.
	_bool IsPlayerInRange();
	// 대화 시작용 F 상호작용 UI의 표시 상태를 동기화한다.
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
	std::optional<CHandle> m_hMoveFade{};
	STATE m_eState{ STATE::IDLE };
	OUTCOME m_eOutcome{ OUTCOME::SPELL_MINIGAME };
	_float m_fFadeDuration{ 0.35f };
	_float m_fFadeHoldDuration{ 0.2f };
	_float m_fMoveFadeInDuration{ 2.f };
	_float m_fMoveFadeOutDuration{ 1.f };
	_float m_fIntroElapsed{}; 
	_float m_fMoveOutcomeElapsed{};
	_float3 m_vPlayerDialogueOffset{ -0.8f, 0.f, 2.2f };
	_string m_DialogueCinematicName{ "InteractiveNpcDialogue" };
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
