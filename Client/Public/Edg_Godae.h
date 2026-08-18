#pragma once
#include "Client_Defines.h"
#include "Mon_State.h"
#include "EnderDragon.h"
NS_BEGIN(Client)

class CEdg_Godae : public CState
{
public:
	DECLARE_DERIVED_TYPE(CEdg_Godae, CState)
private:
	CEdg_Godae();
	~CEdg_Godae() override;
private:
	HRESULT Initialize(class CMonster* pMonster);
public:
	void Enter(CStateMachine* pStateMachine)override;
	void Exit(CStateMachine* pStateMachine)override;

	void PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;

private:
	DRAGON_PHASE			m_ePhase{};
	int32_t m_iAnimIndex[ETOUI(DRAGON_PHASE::END)];
	_float  m_fTime{};
public:
	static SPtr<CEdg_Godae> Create(class CMonster* pMonster);
};

NS_END

