#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"
#include "EnderDragon.h"
NS_BEGIN(Client)
class CTroll_Hit : public CState
{
public:
	DECLARE_DERIVED_TYPE(CTroll_Hit, CState)
private:
	CTroll_Hit();
	~CTroll_Hit() override;
public:
	void Enter(CStateMachine* pStateMachine)override;
	void Exit(CStateMachine* pStateMachine)override;

	void PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;

private:

	HRESULT		Initialize();
	_bool Play_Hit_Anim(CEnderDragon* pDragon);
	_bool Is_Finished(CEnderDragon* pDragon);
private:
	MON_HIT_INFO				m_eHitInfo{};
	uint32_t					m_iIndex{}, m_iSound{};
public:
	static SPtr<CTroll_Hit> Create();
};

NS_END

