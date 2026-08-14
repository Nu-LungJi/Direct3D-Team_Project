#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"
#include "EnderDragon.h"
NS_BEGIN(Client)
class CEdg_Dead : public CState
{
public:
	DECLARE_DERIVED_TYPE(CEdg_Dead, CState)
private:
	CEdg_Dead();
	~CEdg_Dead() override;
public:

	void Enter(CStateMachine* pStateMachine)override;
	void Exit(CStateMachine* pStateMachine)override;

	void PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
private:
	HRESULT		Initialize();
private:
	_float		m_fTick{};
public:
	static SPtr<CEdg_Dead> Create();
};

NS_END

