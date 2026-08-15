#pragma once
#include "Client_Defines.h"
#include "Spider_State.h"
#include "Spider.h"
NS_BEGIN(Client)

class CSpider_Spawn : public CState
{
public:
	DECLARE_DERIVED_TYPE(CSpider_Spawn, CState)
private:
	CSpider_Spawn();
	~CSpider_Spawn() override;
private:
	HRESULT Initialize(const _string& strLevelTag);
public:
	void Enter(CStateMachine* pStateMachine)override;
	void Exit(CStateMachine* pStateMachine)override;

	void PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
private:
	_bool		Play_Anim(CSpider* pDragon, _float fTimeDelta);
private:
	std::vector<MON_ANIM_FSM>			m_Anims;
public:
	static SPtr<CSpider_Spawn> Create(const _string& strLevelTag);
};

NS_END

