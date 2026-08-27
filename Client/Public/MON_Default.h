#pragma once
#include "Client_Defines.h"
#include "Mon_State.h"
NS_BEGIN(Client)

class CMon_Default : public CState
{
public:
	DECLARE_DERIVED_TYPE(CMon_Default, CState)
private:
	CMon_Default();
	~CMon_Default() override;
private:
	HRESULT Initialize();
public:
	void Enter(CStateMachine* pStateMachine)override;
	void Exit(CStateMachine* pStateMachine)override;

	void PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;

public:
	static SPtr<CMon_Default> Create();
};

NS_END

