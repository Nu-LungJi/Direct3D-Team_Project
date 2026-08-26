#pragma once
#include "Client_Defines.h"
#include "Mon_State.h"
#include "Troll.h"
NS_BEGIN(Client)
enum TROLL_SPAWN{TRS_CHASE,TRS_FINISHE,TRS_END};
class CTroll_Spawn : public CState
{
public:
	DECLARE_DERIVED_TYPE(CTroll_Spawn, CState)
private:
	CTroll_Spawn();
	~CTroll_Spawn() override;
private:
	HRESULT Initialize(const _string& strLevelTag);
public:
	void Enter(CStateMachine* pStateMachine)override;
	void Exit(CStateMachine* pStateMachine)override;

	void PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
private:
	_bool		Play_Anim(CTroll* pTroll, _float fTimeDelta,uint32_t iIndex);
	void		Idle(CTroll* pTroll, _float fTimeDelta);
	void		Run(CTroll* pTroll, _float fTimeDelta);
	void		End(CTroll* pTroll, CMon_State* pMonState, _float fTimeDelta);

private:
	MON_ANIM_FSM			m_Anims[TRS_END];
	MON_DEF_STATE			m_eState{MON_DEF_STATE::IDLE};
	_float3					m_vFirstLook{};
	_float3					m_vStartPos{}, m_vEndPos{};
public:
	static SPtr<CTroll_Spawn> Create(const _string& strLevelTag);
};

NS_END

