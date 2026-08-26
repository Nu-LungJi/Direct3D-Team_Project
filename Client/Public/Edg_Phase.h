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
	void		Befor_Action2(CEnderDragon* pDragon, _float fTimeDelta);
	void		Before_Action5(CEnderDragon* pDragon, _float fTimeDelta);
	_bool		After_Action2(CEnderDragon* pDragon, _float fTimeDelta);

	void		Phase_Before_Action(CEnderDragon* pDragon, _float fTimeDelta);
	void		Phase_Change_Action(CEnderDragon* pDragon, _float fTimeDelta);
	void		Phase_After_Action(CEnderDragon* pDragon, _float fTimeDelta);
	
	
	void		Effect_All(CEnderDragon* pDragon, _float fTimeDelta);
	void		Effect_Single(CEnderDragon* pDragon, const _string& strName);
	void		Cinematic_SmokeMove(CEnderDragon* pDragon,_float3 vOffset);
	void		End(CEnderDragon_State* pStateMachine, CBTBlackBoard* pBlackBoard);
private:
	DRAGON_PHASE			m_ePhase{};
	DRAGON_PHASE			m_eNextPhase{};
	uint32_t				m_iEffectID{};
	int32_t					 m_iDefaultAnimIndex{ -1 }, m_iBoneIndex{-1};

	_bool					m_bNext{}, m_bShake{ false }, m_bSound{ false };
	_float					m_fTick{}, m_fSpawnTick{}, m_fAngle{};
	_float3					m_vNextDir{}, m_vLastDir{};
	EDG_SPAWN_NUMBER		m_eNum{EDG_SPAWN_NUMBER::FIRST};

	std::list<_float3>	m_PhasePos[ETOUI(DRAGON_PHASE::END)];

	std::list<EDG_ANIM_FSM> m_Anims[ETOUI(DRAGON_PHASE::END)];
public:
	static SPtr<CEdg_Phase> Create(const _string& strLevelTag);
};

NS_END

