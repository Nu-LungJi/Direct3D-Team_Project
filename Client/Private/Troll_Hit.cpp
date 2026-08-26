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

HRESULT		CTroll_Hit::Initialize(CTroll* pTroll)
{
	m_Anims[ETOUI(HIT_MOTION::NORMAL)].push_back(MON_ANIM_FSM{
		.iAnimIndex = pTroll->Find_AnimIndex("AN_SK_Troll_ArmoredTroll_CMB_Master_LOD0_Skeleton_Trl_Rct_Dazed_Start_anm.bin"),
	.fBlend = 0.1f, .bLoop = false , });
	
	m_Anims[ETOUI(HIT_MOTION::NORMAL)].push_back(MON_ANIM_FSM{
		.iAnimIndex = pTroll->Find_AnimIndex("AN_SK_Troll_ArmoredTroll_CMB_Master_LOD0_Skeleton_Trl_Rct_Dazed_End_anm.bin"),
	.fBlend = 0.1f, .bLoop = false , });

	return S_OK;
}
void CTroll_Hit::Enter(CStateMachine* pStateMachine)
{
	CTroll* pTroll = pStateMachine->GetOwner<CTroll>();

	if (nullptr == pTroll) return;
	auto pBB = pTroll->Get_BlackBoard();
	if (nullptr == pBB) return;
	if (false == pTroll->Activate_PendingHit()) return;

	MON_HIT_INFO MonHitInfo = pTroll->Get_ActiveHitInfo();

	uint32_t iCurAtt = pTroll->Find_SkillNum(MonHitInfo.eAttType);
	m_HitTable.eSkillType = MonHitInfo.eHitType;
	m_HitTable.eHitMotion = ReActiveTable(static_cast<TROLL_SKILL>(iCurAtt));



	//MONSOUND Sound_Desc{};
	//_float3 vPos = pTroll->GetTransform().GetPosition();
	//Sound_Desc.SoundKey = "Hit";
	//Sound_Desc.SoundPlay = SOUND_PLAY_DESC{ .fVolume = 0.8f,.bLoop = false, };
	//Sound_Desc.str3DSound = SOUND_3D_DESC{ .vPosition = vPos ,.fMinDistance = 1.f, .fMaxDistance = 200.f, .eRolloff = SOUND_3D_ROLLOFF::LINEAR };
	//pTroll->Play_Sound(Sound_Desc);

	m_iIndex = 0;
}

void CTroll_Hit::Exit(CStateMachine* pStateMachine)
{
	auto pTroll = pStateMachine->GetOwner<CTroll>();
	if (nullptr == pTroll) return;

	pTroll->Set_Break(false);
	pTroll->ReActiveTable();
}

void CTroll_Hit::PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta)
{
}

void CTroll_Hit::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto pTrollFsm = Cast<CMon_State>(pStateMachine);
	if (nullptr == pTrollFsm) return;

	auto pTroll = pStateMachine->GetOwner<CTroll>();
	if (nullptr == pTroll) return;

	//if (Is_Finished(pTroll))
		pTrollFsm->Request_State(MON_STATE::COMBAT);

}

_bool CTroll_Hit::Play_Hit_Anim(CTroll* pTroll)
{
	auto pAnimator = pTroll->Get_Animator();
	if (nullptr == pAnimator) return true;
	auto pMove = pTroll->Get_MoveIntent();
	if (nullptr == pMove) return true;
	auto pTarget = pTroll->Get_Target();
	if (nullptr == pTarget) return true;
	if (m_iIndex >= m_Anims[ETOUI(m_HitTable.eHitMotion)].size())
		return true;

	MON_ANIM_FSM Anim = m_Anims[ETOUI(m_HitTable.eHitMotion)][m_iIndex];
	_float fRatio = pAnimator->GetPlayAnimRatio();

	pAnimator->Play_Anim(Anim.iAnimIndex, Anim.bLoop, Anim.fBlend);

	if (pAnimator->GetFinish())
		++m_iIndex;

	return false;
}

_bool CTroll_Hit::Is_Finished(CTroll* pTroll)
{
	auto pBB = pTroll->Get_BlackBoard();
	if (nullptr == pBB) return true;
	auto pAnimator = pTroll->Get_Animator();
	if (nullptr == pAnimator) return true;

	if (pAnimator->GetFinish())
		++m_iIndex;

	return false;
}

HIT_MOTION CTroll_Hit::ReActiveTable(TROLL_SKILL eType)
{
	switch (eType)
	{
	case TROLL_SKILL::DOLJIN:
		return HIT_MOTION::BLOWBACK;
		break;
	default:
		return HIT_MOTION::NORMAL;
	}
}

SPtr<CTroll_Hit> CTroll_Hit::Create(CTroll* pTroll)
{
	auto pInstance = ToSPtr(new CTroll_Hit{});
	if (FAILED(pInstance->Initialize(pTroll)))
	{
		MSG_BOX("Failed to create CTroll_Hit");
		return nullptr;
	}

	return pInstance;
}
