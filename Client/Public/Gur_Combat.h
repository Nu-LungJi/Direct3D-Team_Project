#pragma once
#include "Client_Defines.h"
#include "Mon_State.h"
#include "Spider.h"
NS_BEGIN(Client)

class CGur_Combat : public CState
{
public:
	DECLARE_DERIVED_TYPE(CGur_Combat, CState)
private:
	CGur_Combat();
	~CGur_Combat() override;
private:
	HRESULT Initialize(const _string& strLevelTag);
public:
	void Enter(CStateMachine* pStateMachine)override;
	void Exit(CStateMachine* pStateMachine)override;

	void PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
public:
	static SPtr<CGur_Combat> Create(const _string& strLevelTag);
};

NS_END

