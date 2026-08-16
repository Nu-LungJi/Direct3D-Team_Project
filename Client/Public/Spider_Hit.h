#pragma once
#include "Client_Defines.h"
#include "Spider_State.h"
#include "Spider.h"
NS_BEGIN(Client)

enum class HIT_TYPE{NORMAL, LAUNCH ,KNOCKBACK, SLAM, GODAE, END};
enum class HIT_MOTION{NORMAL, LAND, AIR, GROUND, GODAE, BLOWBACK, GROUND_SLAM, FALLING,END};

enum class HIT_STEP{START,LOOP,END};
typedef struct hitalltable
{
	HIT_MOTION eHitMotion{};
	HIT_TYPE eHitType{};
	MON_ANIM_FSM Anim{};
}NEW_HIT_TABLE;
class CSpider_Hit : public CState
{
public:
	DECLARE_DERIVED_TYPE(CSpider_Hit, CState)
private:
	CSpider_Hit();
	~CSpider_Hit() override;
private:
	HRESULT Initialize(const _string& strLevelTag, CSpider* pSpider);
public:
	void Enter(CStateMachine* pStateMachine)override;
	void Exit(CStateMachine* pStateMachine)override;

	void PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;

private:
	HIT_TYPE				Reactive_Table(PLAYER_SKILL_TYPE eType);
	void					Check_PendingHit(CSpider* pSpider);
	void					Hit_Step_Start(CSpider* pSpider);
	void					Hit_Step_Loop(CSpider* pSpider);
	void					Hit_Step_End(CSpider* pSpider);

	_bool					PlayAnim(CComAnimator* pAnimator,_bool bLoop = false);
private:
	uint32_t						m_iAnimIndex{};
	HIT_STEP						m_HitStep{};
	NEW_HIT_TABLE					m_HitTable{};
	std::vector<MON_ANIM_FSM>		m_Anims[ETOUI(HIT_TYPE::END)][ETOUI(HIT_MOTION::END)];
public:
	static SPtr<CSpider_Hit> Create(const _string& strLevelTag, CSpider* pSpider);
};

NS_END

