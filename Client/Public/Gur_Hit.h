#pragma once
#include "Client_Defines.h"
#include "Mon_State.h"
#include "TmbGurdian.h"
NS_BEGIN(Client)


typedef struct hitalltable
{
	HIT_MOTION eHitMotion{};
	HIT_TYPE eHitType{};
	PLAYER_SKILL_TYPE eSkillType{};

}NEW_HIT_TABLE;
class CGur_Hit : public CState
{
public:
	DECLARE_DERIVED_TYPE(CGur_Hit, CState)
private:
	CGur_Hit();
	~CGur_Hit() override;
private:
	HRESULT Initialize(const _string& strLevelTag, CTmbGurdian* pSpider);
public:
	void Enter(CStateMachine* pStateMachine)override;
	void Exit(CStateMachine* pStateMachine)override;

	void PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;

private:
	_float3					TargetDir(CTmbGurdian* pTmb, _bool bFront);
	void					MoveIntent(CTmbGurdian* pTmb, _float3 vDir, _float fSpeed);
	void					Set_Gravity(_bool bGravity, CTmbGurdian* pTmb);
	void					Jump(CTmbGurdian* pTmb, _float fPower, _bool bUp = true);
	HIT_TYPE				Reactive_TableMotion(PLAYER_SKILL_TYPE eType, _bool bIsGround, CTmbGurdian* pTmb = nullptr);
	void					Check_PendingHit(CTmbGurdian* pTmb);
	void					MotionToPlay(CTmbGurdian* pTmb, CComAnimator* pAnimator, CMon_State* pTmbState);
	_bool					PlayAnim(CComAnimator* pAnimator, _bool bLoop = false);
	void					Finishied(CMon_State* pTmbState);
	void					ChangeMotion(HIT_MOTION eMotion);

	void					Effect_Loop(CTmbGurdian* pTmb);
	void					Effect(CTmbGurdian* pTmb,const _string& SkillName);
	void					ResetEffect();
private:

	_bool							m_bTurn{ false };
	uint32_t						m_iAnimIndex{}, m_iSkillEffID{};
	HIT_STEP						m_HitStep{};
	NEW_HIT_TABLE					m_HitTable{};
	std::vector<MON_ANIM_FSM>		m_Anims[ETOUI(HIT_TYPE::END)][ETOUI(HIT_MOTION::END)];
public:
	static SPtr<CGur_Hit> Create(const _string& strLevelTag, CTmbGurdian* pTmb);
};

NS_END

