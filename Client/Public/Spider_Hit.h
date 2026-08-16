#pragma once
#include "Client_Defines.h"
#include "Spider_State.h"
#include "Spider.h"
NS_BEGIN(Client)

enum class HIT_TYPE{NORMAL, LAUNCH ,KNOCKBACK, SLAM, GODAE, END};
enum class HIT_MOTION{NORMAL, LAND, AIR, GROUND, GODAE, BLOWBACK, GROUND_SLAM, FALLING, REBOUND, END};

enum class HIT_STEP{START,LOOP,END};
typedef struct hitalltable
{
	HIT_MOTION eHitMotion{};
	HIT_TYPE eHitType{};
	PLAYER_SKILL_TYPE eSkillType{};

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
	_float3					TargetDir(CSpider* pSpider, _bool bFront);
	void					MoveIntent(CSpider* pSpider, _float3 vDir,_float fSpeed);
	void					Set_Gravity(_bool bGravity,CSpider* pSpider);
	
	HIT_TYPE				Reactive_OnlyType(PLAYER_SKILL_TYPE eType);
	HIT_TYPE				Reactive_TableMotion(PLAYER_SKILL_TYPE eType);
	void					Check_PendingHit(CSpider* pSpider);
	void					MotionToPlay(CSpider* pSpider, CComAnimator* pAnimator, CSpider_State* pSpiderState);
	_bool					PlayAnim(CComAnimator* pAnimator,_bool bLoop = false);
	void					Finishied(CSpider_State* pSpiderState);
	void					ChangeMotion(HIT_MOTION eMotion);
private:
	uint32_t						m_iAnimIndex{};
	HIT_STEP						m_HitStep{};
	NEW_HIT_TABLE					m_HitTable{};
	std::vector<MON_ANIM_FSM>		m_Anims[ETOUI(HIT_TYPE::END)][ETOUI(HIT_MOTION::END)];
public:
	static SPtr<CSpider_Hit> Create(const _string& strLevelTag, CSpider* pSpider);
};

NS_END

