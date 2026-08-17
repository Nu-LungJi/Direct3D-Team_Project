#include "pch.h"
#include "Spider_Combat.h"
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
#include "ComCharacterMoveIntent.h"
#include "ComAnimator.h"
#include "ComModelInstance.h"
NS_USING(Client)
CSpider_Combat::CSpider_Combat()
{
}

CSpider_Combat::~CSpider_Combat()
{
}
HRESULT CSpider_Combat::Initialize(const _string& strLevelTag)
{

	return S_OK;
}
void CSpider_Combat::Enter(CStateMachine* pStateMachine)
{
	CSpider* pSpider = pStateMachine->GetOwner<CSpider>();

	if (nullptr == pSpider)
		return;

	pSpider->Set_StateFinished(false);


}

void CSpider_Combat::Exit(CStateMachine* pStateMachine)
{
	auto pSpider = pStateMachine->GetOwner<CSpider>();
	if (nullptr == pSpider) return;


}

void CSpider_Combat::PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta)
{
}

void CSpider_Combat::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto pSpiderFsm = Cast<CMon_State>(pStateMachine);
	if (nullptr == pSpiderFsm) return;

	auto pSpider = pStateMachine->GetOwner<CSpider>();
	if (nullptr == pSpider) return;

	auto pBB = pSpider->Get_BlackBoard();
	if (nullptr == pBB) return;



}

SPtr<CSpider_Combat> CSpider_Combat::Create(const _string& strLevelTag)
{
	auto pInstance = ToSPtr(new CSpider_Combat{});
	if (FAILED(pInstance->Initialize(strLevelTag)))
	{
		MSG_BOX("Failed to create CSpider_Combat");
		return nullptr;
	}

	return pInstance;
}
