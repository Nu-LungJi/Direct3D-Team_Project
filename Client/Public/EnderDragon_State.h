#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"
#include "BlackBoardKey.h"


NS_BEGIN(Client)
#define MON_STATE_M  \
X(NONE)\
X(SPAWN)            \
X(COMBAT)            \
X(HIT)               \
X(GROGGY)\
X(PHASE_CHANGE)\
X(DEAD)\
X(END)
#define X(name) name,
enum class MON_STATE{ MON_STATE_M};
#undef X
class CEnderDragon_State final : public CStateMachine
{
public:
	DECLARE_DERIVED_TYPE(CEnderDragon_State, CStateMachine)

private:
	CEnderDragon_State() ;
	CEnderDragon_State(const CEnderDragon_State& rhs);
	~CEnderDragon_State() override ;

private:
	HRESULT		Initialize(void* pArg) override;

public:
	_bool		Add_State(MON_STATE eState, SPtr<CState> pState);
	_bool		Initialize_State(MON_STATE eState);
	_bool		Request_State(MON_STATE eState);
	
	void		ApplyStateRequest();
	void		PriorityUpdate(_float fTimeDelta);
	MON_STATE	GetCurState() { return m_eCurState; }
private:
	_bool		IsRegistered(MON_STATE eState);
private:
	std::unordered_set<uint32_t> m_RegisteredState{};
	MON_STATE					m_eCurState	   { MON_STATE::NONE};
	MON_STATE					m_eRequestState{ MON_STATE::NONE };

public:
	static	UPtr<CEnderDragon_State> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

};

NS_END

