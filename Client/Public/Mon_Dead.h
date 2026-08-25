#pragma once
#include "Client_Defines.h"
#include "Mon_State.h"
NS_BEGIN(Client)
class CMon_Dead : public CState
{
public:
	DECLARE_DERIVED_TYPE(CMon_Dead, CState)
private:
	CMon_Dead();
	~CMon_Dead() override;

public:

	void Enter(CStateMachine* pStateMachine)override;
	void Exit(CStateMachine* pStateMachine)override;
	void PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
private:
	HRESULT		Initialize(const _string& DeadAnim, CMonster* pMonster);
private:
	_float		m_fTick{};
	int32_t		m_iIndex{-1};
public:
	static SPtr<CMon_Dead> Create(const _string& DeadAnim, CMonster* pMonster);
};

NS_END


