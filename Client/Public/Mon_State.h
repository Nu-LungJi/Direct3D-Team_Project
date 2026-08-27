#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"
#include "BlackBoardKey.h"


NS_BEGIN(Client)
enum class HIT_TYPE { NORMAL, LAUNCH, KNOCKBACK, SLAM, GODAE,FIRE, END };
enum class HIT_MOTION { NORMAL, LAND, AIR, GROUND, GODAE, BLOWBACK, GROUND_SLAM, FALLING, REBOUND, AIR_LAND, END };
enum class MON_DEF_STATE {IDLE, RUN, END};
enum class HIT_STEP { START, LOOP, END };
typedef struct stredganimfsm
{
	int32_t iAnimIndex{};
	_float	fBlend{};
	_string	  SkillName{};
	_float			  fSkillRatio{ 0.f };
	_bool			  bSkill{ false };
	_bool			  bLoop{ false };
}MON_ANIM_FSM;
typedef struct hitalltable
{
	HIT_MOTION eHitMotion{};
	HIT_TYPE eHitType{};
	PLAYER_SKILL_TYPE eSkillType{};

}NEW_HIT_TABLE;
#define MON_STATE_M  \
X(NONE)\
X(SPAWN)            \
X(COMBAT)            \
X(HIT)               \
X(GROGGY)\
X(PHASE_CHANGE)\
X(DEAD)\
X(GODAE)\
X(NOTHING)\
X(END)
#define X(name) name,
enum class MON_STATE { MON_STATE_M };
#undef X
class CMon_State  : public CStateMachine
{
public:
	DECLARE_DERIVED_TYPE(CMon_State, CStateMachine)

protected:
	CMon_State();
	CMon_State(const CMon_State& rhs);
	~CMon_State() override;

private:
	HRESULT		Initialize(void* pArg) override;

public:
	virtual _bool		Add_State(MON_STATE eState, SPtr<CState> pState);
	virtual _bool		Initialize_State(MON_STATE eState);
	virtual _bool		Request_State(MON_STATE eState);

	virtual void		ApplyStateRequest();
	virtual void		PriorityUpdate(_float fTimeDelta);
	virtual MON_STATE	GetCurState() { return m_eCurState; }
private:
	virtual _bool		IsRegistered(MON_STATE eState);
protected:
	std::unordered_set<uint32_t> m_RegisteredState{};
	MON_STATE					m_eCurState{ MON_STATE::NONE };
	MON_STATE					m_eRequestState{ MON_STATE::NONE };

public:
	static	UPtr<CMon_State> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

};

NS_END

