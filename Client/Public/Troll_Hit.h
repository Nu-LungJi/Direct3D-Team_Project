#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"
#include "Troll.h"
#include "Mon_State.h"
NS_BEGIN(Client)
class CTroll_Hit : public CState
{
public:
	DECLARE_DERIVED_TYPE(CTroll_Hit, CState)
private:
	CTroll_Hit();
	~CTroll_Hit() override;
public:
	void Enter(CStateMachine* pStateMachine)override;
	void Exit(CStateMachine* pStateMachine)override;

	void PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;

private:

	HRESULT			Initialize(CTroll* pTroll);
	_bool			Play_Hit_Anim(CTroll* pTroll);
	_bool			Is_Finished(CTroll* pTroll);
	HIT_MOTION		ReActiveTable(TROLL_SKILL eType);
private:
	NEW_HIT_TABLE					m_HitTable{};
	std::vector<MON_ANIM_FSM>		m_Anims[ETOUI(HIT_MOTION::END)];
	uint32_t						m_iIndex{}, m_iSound{};
public:
	static SPtr<CTroll_Hit> Create(CTroll* pTroll);
};

NS_END

