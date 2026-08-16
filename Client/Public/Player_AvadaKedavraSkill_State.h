#pragma once
#include "Player_SkillStateBase.h"

NS_BEGIN(Client)

class CPlayer_AvadaKedavraSkill_State final : public CPlayer_SkillStateBase
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_AvadaKedavraSkill_State, CPlayer_SkillStateBase)

private:
	CPlayer_AvadaKedavraSkill_State() = default;
	~CPlayer_AvadaKedavraSkill_State() override = default;

public:
	void Enter(CStateMachine* pStateMachine) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Exit(CStateMachine* pStateMachine) override;

	static SPtr<CPlayer_AvadaKedavraSkill_State> Create();

private:
	enum class PHASE : uint8_t
	{
		CAST_BEGIN,
		RELEASE,
		RECOVERY
	};

	void CacheAnimationIndices(const CPlayer& player);

	int32_t m_iCastAnimation{ -1 };
	_bool m_bAnimationsCached{};
	PHASE m_ePhase{ PHASE::CAST_BEGIN };
	_float m_fAnimRatio{};

	static constexpr _float CAST_BLEND_DURATION = 0.16f;
	static constexpr _float RELEASE_RATIO = 0.52f;
	static constexpr _float RECOVERY_RATIO = 0.76f;
	static constexpr _float RECOVERY_EXIT_RATIO = 0.95f;
};

NS_END
