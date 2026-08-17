#pragma once
#include "Client_Defines.h"
#include "Spider_State.h"
#include "Spider.h"
NS_BEGIN(Client)

class CSpider_Combat : public CState
{
public:
	DECLARE_DERIVED_TYPE(CSpider_Combat, CState)
private:
	CSpider_Combat();
	~CSpider_Combat() override;
private:
	HRESULT Initialize(const _string& strLevelTag);
public:
	void Enter(CStateMachine* pStateMachine)override;
	void Exit(CStateMachine* pStateMachine)override;

	void PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
public:
	static SPtr<CSpider_Combat> Create(const _string& strLevelTag);
};

NS_END

