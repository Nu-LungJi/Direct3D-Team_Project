#pragma once
#include "Client_Defines.h"
#include "Mon_State.h"
#include "TmbGurDian.h"
NS_BEGIN(Client)

class CGur_Spawn : public CState
{
public:
	DECLARE_DERIVED_TYPE(CGur_Spawn, CState)
private:
	CGur_Spawn();
	~CGur_Spawn() override;
private:
	HRESULT Initialize(const _string& strLevelTag);
public:
	void Enter(CStateMachine* pStateMachine)override;
	void Exit(CStateMachine* pStateMachine)override;

	void PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
public:
	static SPtr<CGur_Spawn> Create(const _string& strLevelTag);
};

NS_END

