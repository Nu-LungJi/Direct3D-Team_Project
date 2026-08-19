#pragma once
#include "Client_Defines.h"
#include "Mon_State.h"
#include "Spider.h"
NS_BEGIN(Client)
class CSpider_Dead : public CState
{
public:
	DECLARE_DERIVED_TYPE(CSpider_Dead, CState)
private:
	CSpider_Dead();
	~CSpider_Dead() override;
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
	static SPtr<CSpider_Dead> Create();
};

NS_END


