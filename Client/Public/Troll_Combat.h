#pragma once
#include "Client_Defines.h"
#include "Mon_State.h"
#include "Troll.h"
NS_BEGIN(Client)

class CTroll_Combat : public CState
{
public:
	DECLARE_DERIVED_TYPE(CTroll_Combat, CState)
private:
	CTroll_Combat();
	~CTroll_Combat() override;
private:
	HRESULT Initialize();
public:
	void Enter(CStateMachine* pStateMachine)override;
	void Exit(CStateMachine* pStateMachine)override;

	void PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
public:
	static SPtr<CTroll_Combat> Create();
};

NS_END

