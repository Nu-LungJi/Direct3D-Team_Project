#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"

NS_BEGIN(Client)

class CPlayer_Locomotion_State final : public CState
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_Locomotion_State, CState)

	static constexpr int32_t INVALID_ANIMATION = -1;

	enum class MOVE_DIRECTION : uint32_t
	{
		FRONT,
		RIGHT_45,
		RIGHT_90,
		RIGHT_135,
		BACKWARD,
		LEFT_135,
		LEFT_90,
		LEFT_45,
		END,
	};

	// The cleaned Test asset has these five locomotion groups.
	enum class MOVE_TYPE : uint32_t
	{
		WALK_SLOW,
		WALK,
		WALK_FAST,
		JOG,
		SPRINT,
		END,
	};

private:
	CPlayer_Locomotion_State();
	~CPlayer_Locomotion_State() override = default;

public:
	void Enter(CStateMachine* pStateMachine) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;

	void SetFreeIdleAnimation(int32_t iAnimationIndex);
	void SetFreeAnimation(
		MOVE_TYPE eMoveType,
		MOVE_DIRECTION eDirection,
		int32_t iAnimationIndex);

	int32_t GetFreeIdleAnimation() const;
	int32_t GetFreeAnimation(
		MOVE_TYPE eMoveType,
		MOVE_DIRECTION eDirection) const;

	static SPtr<CPlayer_Locomotion_State> Create();

private:
	void InitializeAnimationTable(class CPlayer& player);
	void SetFreeAnimationByName(
		class CPlayer& player,
		MOVE_TYPE eMoveType,
		MOVE_DIRECTION eDirection,
		_string_view sAnimationName);

private:
	_bool m_bAnimationTableInitialized = false;
	int32_t m_iFreeIdleAnimation = INVALID_ANIMATION;
	std::array<
		std::array<int32_t, ETOUI(MOVE_DIRECTION::END)>,
		ETOUI(MOVE_TYPE::END)> m_FreeAnimations{};
};

NS_END
