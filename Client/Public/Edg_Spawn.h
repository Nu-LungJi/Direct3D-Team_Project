#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"
#include "EnderDragon.h"
NS_BEGIN(Client)

class CEdg_Spawn : public CState
{
public:
	DECLARE_DERIVED_TYPE(CEdg_Spawn, CState)
private:
	CEdg_Spawn();
	~CEdg_Spawn() override;
private:
	HRESULT Initialize(const _string& strLevelTag);
public:
	void Enter(CStateMachine* pStateMachine)override;
	void Exit(CStateMachine* pStateMachine)override;

	void PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
private:
	void		SpawnSkill(CEnderDragon* pDragon,const _string& strName);
	_bool		MoveSpawn(CEnderDragon* pDragon, _float fTimeDelta);
	void		Play_Anim(CEnderDragon* pDragon, _float fTimeDelta);
	void		Play_AnimMoveSpawn(CEnderDragon* pDragon, _float fTimeDelta);
	void		Effect(CEnderDragon* pDragon, _float fTimeDelta);
	
private:
	uint32_t				m_iEffectID{};
	SOUND_ID				m_iSound{}, m_iSoundHouling{};
	_bool					m_bNext{}, m_bSound{ false }, m_bSoundH{ false }, m_bEffectStop{ false };
	_float					m_fTick{}, m_fSpawnTick{}, m_fAngle{};
	_float3					m_vNextDir{}, m_vLastDir{};
	EDG_SPAWN_NUMBER		m_eSpawn{EDG_SPAWN_NUMBER::FIRST};

	std::list<EDG_ANIM_FSM> m_Anims[4];
	std::list<_float3>	m_PhasePos;
public:
	static SPtr<CEdg_Spawn> Create(const _string& strLevelTag);
};

NS_END

