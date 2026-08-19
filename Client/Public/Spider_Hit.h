#pragma once
#include "Client_Defines.h"
#include "Mon_State.h"
#include "Spider.h"
NS_BEGIN(Client)


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
	void					Jump(CSpider* pSpider,_float fPower,_bool bUp = true);
	HIT_TYPE				Reactive_TableMotion(PLAYER_SKILL_TYPE eType,_bool bIsGround, CSpider* pSpider);
	void					Check_PendingHit(CSpider* pSpider);
	void					MotionToPlay(CSpider* pSpider, CComAnimator* pAnimator, CMon_State* pSpiderState);
	_bool					PlayAnim(CComAnimator* pAnimator,_bool bLoop = false);
	void					Finishied(CMon_State* pSpiderState);
	void					ChangeMotion(HIT_MOTION eMotion);

	void					Effect_Loop(CSpider* pSpider);
	void					Effect(CSpider* pSpider, const _string& SkillName);
private:
	_bool							m_bTurn{ false };
	uint32_t						m_iAnimIndex{}, m_iSkillEffID{};
	HIT_STEP						m_HitStep{};
	NEW_HIT_TABLE					m_HitTable{};
	std::vector<MON_ANIM_FSM>		m_Anims[ETOUI(HIT_TYPE::END)][ETOUI(HIT_MOTION::END)];
public:
	static SPtr<CSpider_Hit> Create(const _string& strLevelTag, CSpider* pSpider);
};

NS_END

