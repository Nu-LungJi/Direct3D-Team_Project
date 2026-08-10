#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"
#include "EnderDragon.h"
NS_BEGIN(Client)
class CEdg_Phase : public CState
{
public:
	DECLARE_DERIVED_TYPE(CEdg_Phase, CState)
private:
	CEdg_Phase();
	~CEdg_Phase() override;

private:
	HRESULT	Initialize(const _string& strLevelTag);

public:
	void Enter(CStateMachine* pStateMachine)override;
	void Exit(CStateMachine* pStateMachine)override;

	void PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;

private:
	HRESULT		Load_Phase(const _string& PhaseName, std::list<_float3>& PhasePoses);
	_bool		MovePhase(CEnderDragon* pDragon, _float fTimeDelta);

	_bool		MovePhase3(CEnderDragon* pDragon, _float fTimeDelta);
	void		Phase_Change_Action(CEnderDragon* pDragon, _float fTimeDelta);
	void		Effect(CEnderDragon* pDragon, _float fTimeDelta);
	
private:
	DRAGON_PHASE			m_ePhase{};
	DRAGON_PHASE			m_eNextPhase{};
	uint32_t				m_iEffectID{};
	
	_bool					m_bNext{};
	_float					m_fTick{}, m_fSpawnTick{}, m_fAngle{};
	_float3					m_vNextDir{}, m_vLastDir{};
	std::list<_float3>	m_PhasePos[ETOUI(DRAGON_PHASE::END)];

	std::list<EDG_ANIM_FSM> m_Anims[ETOUI(DRAGON_PHASE::END)];
public:
	static SPtr<CEdg_Phase> Create(const _string& strLevelTag);
};

NS_END

