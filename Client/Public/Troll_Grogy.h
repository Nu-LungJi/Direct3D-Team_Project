#pragma once
#include "Client_Defines.h"
#include "Mon_State.h"
#include "Troll.h"
NS_BEGIN(Client)
class CTrollGroggy : public CState
{
public:
	DECLARE_DERIVED_TYPE(CTrollGroggy, CState)
private:
	CTrollGroggy();
	~CTrollGroggy() override;
public:
	void Enter(CStateMachine* pStateMachine)override;
	void Exit(CStateMachine* pStateMachine)override;

	void PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;

private:

	HRESULT		Initialize(CTroll* pTroll);
	_bool Play_Hit_Anim(CTroll* pTroll);
private:
	MON_ANIM_FSM				m_AnimTable[3];
	uint32_t					m_iIndex{}, m_iSound{};
public:
	static SPtr<CTrollGroggy> Create(CTroll* pTroll);
};

NS_END

