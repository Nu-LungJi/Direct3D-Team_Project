#include "pch.h"
#include "Troll_Combat.h"
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
#include "ComCharacterMoveIntent.h"
#include "ComAnimator.h"
#include "ComModelInstance.h"
NS_USING(Client)
CTroll_Combat::CTroll_Combat()
{
}

CTroll_Combat::~CTroll_Combat()
{
}
HRESULT CTroll_Combat::Initialize()
{

	return S_OK;
}
void CTroll_Combat::Enter(CStateMachine* pStateMachine)
{
	CTroll* pTroll = pStateMachine->GetOwner<CTroll>();

	if (nullptr == pTroll)
		return;

	pTroll->Set_StateFinished(false);


}

void CTroll_Combat::Exit(CStateMachine* pStateMachine)
{
	auto pTroll = pStateMachine->GetOwner<CTroll>();
	if (nullptr == pTroll) return;


}

void CTroll_Combat::PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta)
{
}

void CTroll_Combat::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto pTrollFsm = Cast<CMon_State>(pStateMachine);
	if (nullptr == pTrollFsm) return;

	auto pTroll = pStateMachine->GetOwner<CTroll>();
	if (nullptr == pTroll) return;

	auto pBB = pTroll->Get_BlackBoard();
	if (nullptr == pBB) return;

}

SPtr<CTroll_Combat> CTroll_Combat::Create()
{
	auto pInstance = ToSPtr(new CTroll_Combat{});
	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to create CTroll_Combat");
		return nullptr;
	}

	return pInstance;
}
