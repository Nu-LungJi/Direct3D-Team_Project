
#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"

NS_BEGIN(Client)

class CPlayer;

class CPlayer_Hit_State final : public CState
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_Hit_State, CState)

private:
	enum class HIT_DIRECTION : uint8_t
	{
		FWD,
		BWD,
		LFT,
		RHT,
		END
	};

private:
	CPlayer_Hit_State() = default;
	~CPlayer_Hit_State() override = default;

public:
	void Enter(CStateMachine* pStateMachine) override;
	void Exit(CStateMachine* pStateMachine) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;

	static SPtr<CPlayer_Hit_State> Create();

private:
	void CacheAnimationIndices(const CPlayer& player);
	HIT_DIRECTION ResolveHitDirection(const CPlayer& player) const;
	int32_t GetHitAnimation(HIT_DIRECTION eDirection) const;
	int32_t FindAnimationIndex(const CPlayer& player, _string_view sAnimationName) const;

private:
	static constexpr size_t HIT_DIRECTION_COUNT = static_cast<size_t>(HIT_DIRECTION::END);
	static constexpr _float HIT_BLEND_DURATION = 0.1f;

	std::array<int32_t, HIT_DIRECTION_COUNT> m_HitAnimations{};
	_bool m_bAnimationIndicesCached{};
};

NS_END
