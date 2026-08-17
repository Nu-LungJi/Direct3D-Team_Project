#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"
#include "EnderDragon.h"
NS_BEGIN(Client)
typedef struct strrandomball
{
	_float3 vPos{};
	_float  fDist{};
}RAND_BALL_DESC;
class CEdg_Combat : public CState
{
public:
	DECLARE_DERIVED_TYPE(CEdg_Combat, CState)
private:
	CEdg_Combat();
	~CEdg_Combat() override;
public:

	void Enter(CStateMachine* pStateMachine)override;
	void Exit(CStateMachine* pStateMachine)override;

	void PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
private:
	HRESULT		Initialize();
	void		RandomBall(CEnderDragon* pDragon, _vector vPos, _float fDis);
	void		PlaySound(CEnderDragon* pDragon);
private:
	std::vector<RAND_BALL_DESC>		m_RandomBalls[ETOUI(DRAGON_PHASE::END)];
	DRAGON_PHASE					m_ePhase{};
	_float							m_fTick{}, m_fMaxTick{};
	SOUND_ID						m_iWingSound{};
public:
	static SPtr<CEdg_Combat> Create();
};

NS_END

