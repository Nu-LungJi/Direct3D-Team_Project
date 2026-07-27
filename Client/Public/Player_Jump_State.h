#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"

NS_BEGIN(Client)

class CPlayer;

class CPlayer_Jump_State final : public CState
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_Jump_State, CState)

private:
	enum class PHASE : uint32_t
	{
		START,
		FALL,
		LAND,
	};


	enum class JUMP_STATE : uint8_t
	{
		IDLE,
		JOG,
		SPRINT,
		END
	};
private:
	CPlayer_Jump_State() = default;
	~CPlayer_Jump_State() override = default;

public:
	void Enter(CStateMachine* pStateMachine) override;
	void Exit(CStateMachine* pStateMachine) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;

	static SPtr<CPlayer_Jump_State> Create();

private:
	int32_t FindAnimationIndex(const CPlayer& player,_string_view sAnimationName) const;
	void PlayFall(CPlayer& player);
	void PlayLand(CPlayer& player);


private:
	static constexpr size_t JUMP_STATE_COUNT = static_cast<size_t>(JUMP_STATE::END);

private:
	PHASE m_ePhase{ PHASE::START };

	int32_t m_iFallAnimation{ -1 };

	_bool m_bWasAirborne{};
	_float m_fFallStartVerticalSpeed{ -3.f };
	_float m_fJumpStartEnd{ 1.f };



	std::array<int32_t, JUMP_STATE_COUNT> m_LandAnimations{};
	std::array<int32_t, JUMP_STATE_COUNT> m_StartAnimations{};
};

NS_END
