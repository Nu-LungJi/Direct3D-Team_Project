#include "pch.h"
#include "Edg_Dead.h"
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
#include "DragonSkill.h"
#include "EnderDragon_State.h"
#include "ComBeHavior.h"
#include "ComAnimator.h"
NS_USING(Client)
CEdg_Dead::CEdg_Dead()
{
}

CEdg_Dead::~CEdg_Dead()
{
}
HRESULT		CEdg_Dead::Initialize()
{
	return S_OK;
}
void CEdg_Dead::Enter(CStateMachine* pStateMachine)
{
	CEnderDragon* pDragon = pStateMachine->GetOwner<CEnderDragon>();

	if (nullptr == pDragon) return;

	auto pBB = pDragon->Get_BlackBoard();
	if (nullptr == pBB) return;

}

void CEdg_Dead::Exit(CStateMachine* pStateMachine)
{
}

void CEdg_Dead::PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta)
{
	CEnderDragon* pDragon = pStateMachine->GetOwner<CEnderDragon>();

	if (nullptr == pDragon) return;

	auto pBT = pDragon->GetComponent<CComBeHavior>("Com_BT");
	if (nullptr == pBT) return;
	auto pAnimator = pDragon->Get_Animator();
	if (nullptr == pAnimator) return;
	if (pAnimator->GetPlayAnimRatio() >= 0.8f)
	{
		m_fTick += fTimeDelta;

		_float t = std::min(m_fTick /1.f,1.f);
		pDragon->Set_Dissolve(1.f * t);
		
	}
	if (pAnimator->GetFinish())
		pDragon->Set_EndGame();
}

void CEdg_Dead::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{

}
SPtr<CEdg_Dead> CEdg_Dead::Create()
{
	auto pInstance = ToSPtr(new CEdg_Dead{});
	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to create CEdg_Dead");
		return nullptr;
	}

	return pInstance;
}
