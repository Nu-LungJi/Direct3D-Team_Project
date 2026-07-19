
#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"
#include "Player.h"

NS_BEGIN(Client)

class CPlayer_FSM final : public CStateMachine
{
public:
	enum PLAYER_STATE
	{
		IDLE = 1 << 0,
		HAND_UP = 1 << 1,

		WALK = 1 << 2,
		LEFT_WALK = 1 << 3,
		RIGHT_WALK = 1 << 4,
		WALK_BACKWARD = 1 << 5,

		HAND_UP_AND_WALK = 1 << 6,
		HAND_UP_AND_LEFT = 1 << 7,
		HAND_UP_AND_BACKWARD = 1 << 8,
		HAND_UP_AND_RIGHT = 1 << 9,

		TPOSE = 1 << 10,
		ROLL = 1 << 11,

		HIT = 1 << 12,
		HIT_REVERSE = 1 << 13,

		RUN = 1 << 14,
		END = 1 << 15,
	};

private:
	CPlayer_FSM();

public:
	virtual ~CPlayer_FSM() = default;

public:
	HRESULT Initialize(uint32_t* eModelState);

public:
	void Change_State(PLAYER_STATE eState);
	void SetModelState(const uint32_t _eModelState) { *m_eModelState = _eModelState; }
public:

private:
	PLAYER_STATE m_eCurrentState = PLAYER_STATE::END;
	uint32_t* m_eModelState;
	_bool				m_bShift = { false };
public:
	static  SPtr<CPlayer_FSM> Create();
};

NS_END
