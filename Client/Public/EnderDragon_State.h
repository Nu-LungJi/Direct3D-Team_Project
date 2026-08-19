#pragma once
#include "Client_Defines.h"
#include "Mon_State.h"
#include "BlackBoardKey.h"


NS_BEGIN(Client)
class CEnderDragon_State final : public CMon_State
{
public:
	DECLARE_DERIVED_TYPE(CEnderDragon_State, CMon_State)

private:
	CEnderDragon_State() ;
	CEnderDragon_State(const CEnderDragon_State& rhs);
	~CEnderDragon_State() override ;

private:
	HRESULT		Initialize(void* pArg) override;

public:
	_bool		Add_State(MON_STATE eState, SPtr<CState> pState)override;
	_bool		Initialize_State(MON_STATE eState)override;
	_bool		Request_State(MON_STATE eState)override;
	
	void		ApplyStateRequest()override;
	void		PriorityUpdate(_float fTimeDelta)override;
	MON_STATE	GetCurState() { return m_eCurState; }
private:
	_bool		IsRegistered(MON_STATE eState) override;
public:
	static	UPtr<CEnderDragon_State> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

};

NS_END

