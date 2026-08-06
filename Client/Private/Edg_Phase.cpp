#include "pch.h"
#include "Edg_Phase.h"
#include "BTBlackBoard.h"
#include "BlackBoardKey.h"
#include "EnderDragon_State.h"
NS_USING(Client)
CEdg_Phase::CEdg_Phase()
{
}

CEdg_Phase::~CEdg_Phase()
{
}

void CEdg_Phase::Enter(CStateMachine* pStateMachine)
{
	CEnderDragon* pDragon = pStateMachine->GetOwner<CEnderDragon>();

	if (nullptr == pDragon) return;

	auto pBB = pDragon->Get_BlackBoard();
	if (nullptr == pBB) return;

	auto pPhase = pBB->Get_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE);
	if (nullptr == pPhase) return;

	m_ePhase = *pPhase;
	m_eNextPhase = DRAGON_PHASE::END;
}

void CEdg_Phase::Exit(CStateMachine* pStateMachine)
{
	auto pDragon = pStateMachine->GetOwner<CEnderDragon>();
	if (nullptr == pDragon) return;

	pDragon->ReActiveTable();
	pDragon->Set_StateFinished(false);
}

void CEdg_Phase::PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta)
{
}

void CEdg_Phase::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto pDragonFsm = Cast<CEnderDragon_State>(pStateMachine);
	if (nullptr == pDragonFsm) return;

	auto pDragon = pDragonFsm->GetOwner<CEnderDragon>();
	if (nullptr == pDragon) return;

	auto pAnimator = pDragon->Get_Animator();
	if (nullptr == pAnimator) return;

	auto pBB = pDragon->Get_BlackBoard();
	if (nullptr == pBB) return;

	Phase_Change_Action();

	//도망치는게 끝나면 다시 상태전환 하기
	if (m_eNextPhase != DRAGON_PHASE::END)
	{
		pBB->Set_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE, m_eNextPhase);
		pDragonFsm->Request_State(EDG_STATE::COMBAT);
	}
}
void CEdg_Phase::Phase_Change_Action()
{
	//뭔가 도망 치는
	switch (m_ePhase)
	{
	case DRAGON_PHASE::PHASE1:
		break;

	case DRAGON_PHASE::PHASE2:
		if (Phase_Second())
			m_eNextPhase = DRAGON_PHASE::PHASE2;
		break;

	case DRAGON_PHASE::PHASE3:
		if (Phase_Third())
			m_eNextPhase = DRAGON_PHASE::PHASE3;
		break;

	case DRAGON_PHASE::PHASE4:
		if (Phase_Four())
			m_eNextPhase = DRAGON_PHASE::PHASE4;
		break;

	case DRAGON_PHASE::PHASE5:
		break;
	}
	//뭐.. 뭐,....죽어
}
_bool CEdg_Phase::Phase_Second()
{
	return false;
}
_bool CEdg_Phase::Phase_Third()
{
	return false;
}
_bool CEdg_Phase::Phase_Four()
{
	return false;
}
SPtr<CEdg_Phase> CEdg_Phase::Create()
{
	auto pInstance = ToSPtr(new CEdg_Phase{});
	if (!pInstance)
	{
		MSG_BOX("Failed to create CEdg_Phase");
		return nullptr;
	}

	return pInstance;
}
