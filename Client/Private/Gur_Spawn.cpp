#include "pch.h"
#include "Gur_Spawn.h"
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
#include "ComCharacterMoveIntent.h"
#include "ComAnimator.h"
#include "ComModelInstance.h"
NS_USING(Client)
CGur_Spawn::CGur_Spawn()
{
}

CGur_Spawn::~CGur_Spawn()
{
}
HRESULT CGur_Spawn::Initialize(const _string& strLevelTag)
{

	return S_OK;
}
void CGur_Spawn::Enter(CStateMachine* pStateMachine)
{
	CTmbGurdian* pTmb = pStateMachine->GetOwner<CTmbGurdian>();

	if (nullptr == pTmb)
		return;

	pTmb->Set_StateFinished(false);

}

void CGur_Spawn::Exit(CStateMachine* pStateMachine)
{
	auto pSpider = pStateMachine->GetOwner<CTmbGurdian>();
	if (nullptr == pSpider) return;


}

void CGur_Spawn::PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta)
{
}

void CGur_Spawn::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto pGurFsm = Cast<CMon_State>(pStateMachine);
	if (nullptr == pGurFsm) return;

	auto pGur = pStateMachine->GetOwner<CTmbGurdian>();
	if (nullptr == pGur) return;

	auto pBB = pGur->Get_BlackBoard();
	if (nullptr == pBB) return;


	if (pGur->Is_StateFinished())
		pGurFsm->Request_State(MON_STATE::COMBAT);
}

SPtr<CGur_Spawn> CGur_Spawn::Create(const _string& strLevelTag)
{
	auto pInstance = ToSPtr(new CGur_Spawn{});
	if (FAILED(pInstance->Initialize(strLevelTag)))
	{
		MSG_BOX("Failed to create CGur_Spawn");
		return nullptr;
	}

	return pInstance;
}
