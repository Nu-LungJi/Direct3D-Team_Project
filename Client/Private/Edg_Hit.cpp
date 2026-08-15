#include "pch.h"
#include "Edg_Hit.h"
#include "BTBlackBoard.h"
#include "BlackBoardKey.h"
#include "EnderDragon_State.h"
#include "ComAnimator.h"
NS_USING(Client)
CEdg_Hit::CEdg_Hit()
{
}

CEdg_Hit::~CEdg_Hit()
{
	
}

HRESULT		CEdg_Hit::Initialize()
{
	
	return S_OK;
}
void CEdg_Hit::Enter(CStateMachine* pStateMachine)
{
	CEnderDragon* pDragon = pStateMachine->GetOwner<CEnderDragon>();

	if (nullptr == pDragon) return;
	auto pBB = pDragon->Get_BlackBoard();
	if (nullptr == pBB) return;
	if (false == pDragon->Activate_PendingHit()) return;

	m_eHitInfo = pDragon->Get_ActiveHitInfo();
	
	auto pPhase = pBB->Get_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE);
	if (nullptr == pPhase) return;

	switch (m_eHitInfo.eHitType)
	{
	case PLAYER_SKILL_TYPE::DESTORY:
		if (m_Hits[ETOUI(m_eHitInfo.eHitType)][ETOUI(*pPhase)].empty())
		{
			if (*pPhase != DRAGON_PHASE::PHASE5)
			{
				m_Hits[ETOUI(m_eHitInfo.eHitType)][ETOUI(*pPhase)].push_back(EDG_ANIM_FSM{ .iAnimIndex =
				pDragon->Find_AnimIndex("AN_SK_ConjuredDragon_LOD0_Skeleton_Drgn_Cnjrd_Rct_Large_anm.bin"),.fBlend = 0.1f });
			}
			else
			{
				m_Hits[ETOUI(m_eHitInfo.eHitType)][ETOUI(*pPhase)].push_back(EDG_ANIM_FSM{ .iAnimIndex =
				pDragon->Find_AnimIndex("AN_SK_ConjuredDragon_LOD0_Skeleton_Drgn_Rct_Stumble_Bwd_anm.bin"),.fBlend = 0.1f });
			}
		}
		break;
	}

	
	m_iIndex = 0;
}

void CEdg_Hit::Exit(CStateMachine* pStateMachine)
{
	auto pDragon = pStateMachine->GetOwner<CEnderDragon>();
	if (nullptr == pDragon) return;

	pDragon->Set_Break(false);
	pDragon->ReActiveTable();
}

void CEdg_Hit::PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta)
{
}

void CEdg_Hit::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto pDragonFsm = Cast<CEnderDragon_State>(pStateMachine);
	if (nullptr == pDragonFsm) return;

	auto pDragon = pStateMachine->GetOwner<CEnderDragon>();
	if (nullptr == pDragon) return;

	if (Is_Finished(pDragon))
		pDragonFsm->Request_State(MON_STATE::COMBAT);

}

_bool CEdg_Hit::Play_Hit_Anim(CEnderDragon* pDragon)
{
	//윽
	_bool bFinished{ false };
	

	return false;
}

_bool CEdg_Hit::Is_Finished(CEnderDragon* pDragon)
{
	auto pBB = pDragon->Get_BlackBoard();
	if (nullptr == pBB) return true;

	auto pPhase = pBB->Get_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE);
	if (nullptr == pPhase) return true;

	auto& pHits = m_Hits[ETOUI(m_eHitInfo.eHitType)][ETOUI(*pPhase)];

	auto pAnimator = pDragon->Get_Animator();
	if (nullptr == pAnimator) return true;

	
	if (m_iIndex >= pHits.size())
		return true;

	EDG_ANIM_FSM eAnim = pHits[m_iIndex];

	pAnimator->Play_Anim(eAnim.iAnimIndex, false, eAnim.fBlend);


	if (pAnimator->GetFinish())
		++m_iIndex;

	return false;
}

SPtr<CEdg_Hit> CEdg_Hit::Create()
{
	auto pInstance = ToSPtr(new CEdg_Hit{});
	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to create CEdg_Hit");
		return nullptr;
	}

	return pInstance;
}
