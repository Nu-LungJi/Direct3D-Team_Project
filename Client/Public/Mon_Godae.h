#pragma once
#include "Client_Defines.h"
#include "Mon_State.h"
#include "Spider.h"
NS_BEGIN(Client)

class CMon_Godae : public CState
{
public:
	DECLARE_DERIVED_TYPE(CMon_Godae, CState)
private:
	CMon_Godae();
	~CMon_Godae() override;
private:
	HRESULT Initialize(const _string& strAnim, class CMonster* pMonster);
public:
	void Enter(CStateMachine* pStateMachine)override;
	void Exit(CStateMachine* pStateMachine)override;

	void PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;

private:
	int32_t m_iAnimIndex{-1};
	_float  m_fTime{};
public:
	static SPtr<CMon_Godae> Create(const _string& strAnim,class CMonster* pMonster);
};

NS_END

