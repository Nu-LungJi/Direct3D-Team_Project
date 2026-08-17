#include "pch.h"
#include "Gur_Combat.h"
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
#include "ComCharacterMoveIntent.h"
#include "ComAnimator.h"
#include "ComModelInstance.h"
NS_USING(Client)
CGur_Combat::CGur_Combat()
{
}

CGur_Combat::~CGur_Combat()
{
}
HRESULT CGur_Combat::Initialize(const _string& strLevelTag)
{

	return S_OK;
}
void CGur_Combat::Enter(CStateMachine* pStateMachine)
{
	CSpider* pSpider = pStateMachine->GetOwner<CSpider>();

	if (nullptr == pSpider)
		return;

	pSpider->Set_StateFinished(false);


}

void CGur_Combat::Exit(CStateMachine* pStateMachine)
{
	auto pSpider = pStateMachine->GetOwner<CSpider>();
	if (nullptr == pSpider) return;


}

void CGur_Combat::PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta)
{
}

void CGur_Combat::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto pSpiderFsm = Cast<CMon_State>(pStateMachine);
	if (nullptr == pSpiderFsm) return;

	auto pSpider = pStateMachine->GetOwner<CSpider>();
	if (nullptr == pSpider) return;

	auto pBB = pSpider->Get_BlackBoard();
	if (nullptr == pBB) return;



}

SPtr<CGur_Combat> CGur_Combat::Create(const _string& strLevelTag)
{
	auto pInstance = ToSPtr(new CGur_Combat{});
	if (FAILED(pInstance->Initialize(strLevelTag)))
	{
		MSG_BOX("Failed to create CGur_Combat");
		return nullptr;
	}

	return pInstance;
}
