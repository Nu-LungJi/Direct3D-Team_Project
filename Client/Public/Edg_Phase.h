#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"
#include "EnderDragon.h"
NS_BEGIN(Client)
class CEdg_Phase : public CState
{
public:
	DECLARE_DERIVED_TYPE(CEdg_Phase, CState)
private:
	CEdg_Phase();
	~CEdg_Phase() override;
public:
	void Enter(CStateMachine* pStateMachine)override;
	void Exit(CStateMachine* pStateMachine)override;

	void PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;

private:
	void		Phase_Change_Action();
	_bool        Phase_Second();
	_bool        Phase_Third();
	_bool        Phase_Four();
	
private:
	DRAGON_PHASE			m_ePhase{};
	DRAGON_PHASE			m_eNextPhase{};
public:
	static SPtr<CEdg_Phase> Create();
};

NS_END

