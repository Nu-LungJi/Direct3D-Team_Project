#include "pch.h"
#include "Troll_Hit.h"
#include "BTBlackBoard.h"
#include "BlackBoardKey.h"
#include "EnderDragon_State.h"
#include "ComAnimator.h"
NS_USING(Client)
CTroll_Hit::CTroll_Hit()
{
}

CTroll_Hit::~CTroll_Hit()
{

}

HRESULT		CTroll_Hit::Initialize()
{

	return S_OK;
}
void CTroll_Hit::Enter(CStateMachine* pStateMachine)
{
	CEnderDragon* pDragon = pStateMachine->GetOwner<CEnderDragon>();

	if (nullptr == pDragon) return;
	auto pBB = pDragon->Get_BlackBoard();
	if (nullptr == pBB) return;
	if (false == pDragon->Activate_PendingHit()) return;

	m_eHitInfo = pDragon->Get_ActiveHitInfo();

	switch (m_eHitInfo.eHitType)
	{
	case PLAYER_SKILL_TYPE::DESTORY:
		break;
			//if (*pPhase != DRAGON_PHASE::PHASE5)
			//{
			//	m_Hits[ETOUI(m_eHitInfo.eHitType)][ETOUI(*pPhase)].push_back(EDG_ANIM_FSM{ .iAnimIndex =
			//	pDragon->Find_AnimIndex("AN_SK_ConjuredDragon_LOD0_Skeleton_Drgn_Cnjrd_Rct_Large_anm.bin"),.fBlend = 0.1f });
			//}
			//else
			//{
			//	m_Hits[ETOUI(m_eHitInfo.eHitType)][ETOUI(*pPhase)].push_back(EDG_ANIM_FSM{ .iAnimIndex =
			//	pDragon->Find_AnimIndex("AN_SK_ConjuredDragon_LOD0_Skeleton_Drgn_Rct_Stumble_Bwd_anm.bin"),.fBlend = 0.1f });
			//}
	}


	//MONSOUND Sound_Desc{};
	//_float3 vPos = pDragon->GetTransform().GetPosition();
	//Sound_Desc.SoundKey = "Hit";
	//Sound_Desc.SoundPlay = SOUND_PLAY_DESC{ .fVolume = 0.8f,.bLoop = false, };
	//Sound_Desc.str3DSound = SOUND_3D_DESC{ .vPosition = vPos ,.fMinDistance = 1.f, .fMaxDistance = 200.f, .eRolloff = SOUND_3D_ROLLOFF::LINEAR };
	//pDragon->Play_Sound(Sound_Desc);

	m_iIndex = 0;
}

void CTroll_Hit::Exit(CStateMachine* pStateMachine)
{
	auto pDragon = pStateMachine->GetOwner<CEnderDragon>();
	if (nullptr == pDragon) return;

	pDragon->Set_Break(false);
	pDragon->ReActiveTable();
}

void CTroll_Hit::PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta)
{
}

void CTroll_Hit::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto pDragonFsm = Cast<CEnderDragon_State>(pStateMachine);
	if (nullptr == pDragonFsm) return;

	auto pDragon = pStateMachine->GetOwner<CEnderDragon>();
	if (nullptr == pDragon) return;

	if (Is_Finished(pDragon))
		pDragonFsm->Request_State(MON_STATE::COMBAT);

}

_bool CTroll_Hit::Play_Hit_Anim(CEnderDragon* pDragon)
{
	//윽
	_bool bFinished{ false };


	return false;
}

_bool CTroll_Hit::Is_Finished(CEnderDragon* pDragon)
{
	auto pBB = pDragon->Get_BlackBoard();
	if (nullptr == pBB) return true;

	auto pPhase = pBB->Get_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE);
	if (nullptr == pPhase) return true;

	auto pAnimator = pDragon->Get_Animator();
	if (nullptr == pAnimator) return true;

	if (pAnimator->GetFinish())
		++m_iIndex;

	return false;
}

SPtr<CTroll_Hit> CTroll_Hit::Create()
{
	auto pInstance = ToSPtr(new CTroll_Hit{});
	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to create CTroll_Hit");
		return nullptr;
	}

	return pInstance;
}
