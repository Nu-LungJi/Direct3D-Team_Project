#include "pch.h"
#include "Mon_Godae.h"
#include "Monster.h"
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
#include "ComCharacterMoveIntent.h"
#include "ComAnimator.h"
#include "Monster.h"
#include "ComModelInstance.h"
NS_USING(Client)
CMon_Godae::CMon_Godae()
{
}

CMon_Godae::~CMon_Godae()
{
}
HRESULT CMon_Godae::Initialize(const _string& strAnim, CMonster* pMonster)
{
	m_iAnimIndex = pMonster->Find_AnimIndex(strAnim);

	if (m_iAnimIndex < 0) return E_FAIL;

	return S_OK;
}
void CMon_Godae::Enter(CStateMachine* pStateMachine)
{
	CMonster* pMon = pStateMachine->GetOwner<CMonster>();

	if (nullptr == pMon) return;

	m_fTime = 0.f;
}

void CMon_Godae::Exit(CStateMachine* pStateMachine)
{
	auto pMonster = pStateMachine->GetOwner<CMonster>();
	if (nullptr == pMonster) return;


}

void CMon_Godae::PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta)
{
}

void CMon_Godae::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto pState = Cast<CMon_State>(pStateMachine);
	if (nullptr == pState) return;

	auto pMonster = pStateMachine->GetOwner<CMonster>();
	if (nullptr == pMonster) return;

	auto pAnimator = pMonster->Get_Animator();
	if (nullptr == pAnimator) return;

	pAnimator->Play_Anim(m_iAnimIndex, true, 0.1f);
	m_fTime += fTimeDelta;

	if (m_fTime > 1.5f)
		pState->Request_State(MON_STATE::COMBAT);
}


SPtr<CMon_Godae> CMon_Godae::Create(const _string& strLevelTag, CMonster* pMonster)
{
	auto pInstance = ToSPtr(new CMon_Godae{});
	if (FAILED(pInstance->Initialize(strLevelTag, pMonster)))
	{
		MSG_BOX("Failed to create CMon_Godae");
		return nullptr;
	}

	return pInstance;
}
