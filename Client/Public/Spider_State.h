#pragma once
#include "Client_Defines.h"
#include "EnderDragon_State.h"
#include "BlackBoardKey.h"


NS_BEGIN(Client)
typedef struct stredganimfsm
{
	int32_t iAnimIndex{};
	_float	fBlend{};
}MON_ANIM_FSM;
class CSpider_State final : public CStateMachine
{
public:
	DECLARE_DERIVED_TYPE(CSpider_State, CStateMachine)

private:
	CSpider_State();
	CSpider_State(const CSpider_State& rhs);
	~CSpider_State() override;

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
	MON_STATE					m_eCurState{ MON_STATE::NONE };
	MON_STATE					m_eRequestState{ MON_STATE::NONE };

public:
	static	UPtr<CSpider_State> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

};

NS_END

