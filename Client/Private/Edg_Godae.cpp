#include "pch.h"
#include "Edg_Godae.h"
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
#include "ComCharacterMoveIntent.h"
#include "ComAnimator.h"
#include "ComModelInstance.h"
NS_USING(Client)
CEdg_Godae::CEdg_Godae()
{
}

CEdg_Godae::~CEdg_Godae()
{
}
HRESULT CEdg_Godae::Initialize(CMonster* pMonster)
{
	int32_t iIndex = pMonster->Find_AnimIndex("AN_SK_ConjuredDragon_LOD0_Skeleton_Drgn_Cnjrd_Rct_Large_anm.bin");
	m_iAnimIndex[ETOUI(DRAGON_PHASE::PHASE1)] = m_iAnimIndex[ETOUI(DRAGON_PHASE::PHASE2)] = m_iAnimIndex[ETOUI(DRAGON_PHASE::PHASE3)] =
		m_iAnimIndex[ETOUI(DRAGON_PHASE::PHASE4)] = iIndex;
	m_iAnimIndex[ETOUI(DRAGON_PHASE::PHASE5)] = pMonster->Find_AnimIndex("AN_SK_ConjuredDragon_LOD0_Skeleton_Drgn_Rct_Stumble_Bwd_anm.bin");

	return S_OK;
}
void CEdg_Godae::Enter(CStateMachine* pStateMachine)
{
	CEnderDragon* pDragon = pStateMachine->GetOwner<CEnderDragon>();

	if (nullptr == pDragon) return;


	auto pBB = pDragon->Get_BlackBoard();
	if (nullptr == pBB) return;

	auto pPhase = pBB->Get_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE);
	if (nullptr == pPhase) return;

	m_ePhase = *pPhase;
	m_fTime = 0.f;
}

void CEdg_Godae::Exit(CStateMachine* pStateMachine)
{


}

void CEdg_Godae::PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta)
{
}

void CEdg_Godae::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto pState = Cast<CMon_State>(pStateMachine);
	if (nullptr == pState) return;

	auto pMonster = pStateMachine->GetOwner<CMonster>();
	if (nullptr == pMonster) return;

	auto pAnimator = pMonster->Get_Animator();
	if (nullptr == pAnimator) return;

	pAnimator->Play_Anim(m_iAnimIndex[ETOUI(m_ePhase)], true, 0.1f);
	m_fTime += fTimeDelta;

	if (m_fTime > 1.5f)
		pState->Request_State(MON_STATE::COMBAT);
}

SPtr<CEdg_Godae> CEdg_Godae::Create( CMonster* pMonster)
{
	auto pInstance = ToSPtr(new CEdg_Godae{});
	if (FAILED(pInstance->Initialize( pMonster)))
	{
		MSG_BOX("Failed to create CEdg_Godae");
		return nullptr;
	}

	return pInstance;
}
