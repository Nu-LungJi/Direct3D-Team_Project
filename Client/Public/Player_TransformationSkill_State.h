#pragma once
#include "Player_SkillStateBase.h"

NS_BEGIN(Client)

// [LSY] 변신 스킬의 시전 연출과 타깃을 오크통으로 교체하는 시점을 관리한다.
class CPlayer_TransformationSkill_State final : public CPlayer_SkillStateBase
{
public:
	enum class PHASE : uint8_t
	{
		CAST_BEGIN,
		RELEASE,
		RECOVERY
	};
public:
	DECLARE_DERIVED_TYPE(CPlayer_TransformationSkill_State, CPlayer_SkillStateBase)

private:
	CPlayer_TransformationSkill_State() = default;
	~CPlayer_TransformationSkill_State() override = default;

public:
	void Enter(CStateMachine* pStateMachine) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Exit(CStateMachine* pStateMachine) override;

	static SPtr<CPlayer_TransformationSkill_State> Create();

private:
	void TransformTargetToBarrel();

	PHASE m_ePhase{ PHASE::CAST_BEGIN };
	_float m_fAnimRatio{};
	_float m_fTransformationElapsed{};
	_bool m_bTransformationResolved{};
	CHandle m_hTransformationTarget{};

	// 주문 방출 Cue: 타깃 판정, 실제 변신 적용, 발사/피격 연출 시작 지점.
	static constexpr _float RELEASE_RATIO = 0.28f;
	// 후딜 진입 Cue: 방출 연출 종료 및 조작 복구 준비 지점.
	static constexpr _float RECOVERY_RATIO = 0.38f;
	// 상태 종료 Cue: Locomotion으로 복귀해 일반 조작을 다시 허용하는 지점.
	static constexpr _float RECOVERY_EXIT_RATIO = 0.52f;
	// 타깃 이펙트가 전개된 뒤 실제 교체가 일어나는 시점.
	static constexpr _float TRANSFORMATION_DELAY = 0.5f;

	uint32_t m_iEffectID = INVALID_EFFECT_INSTANCE_ID;
};

NS_END
