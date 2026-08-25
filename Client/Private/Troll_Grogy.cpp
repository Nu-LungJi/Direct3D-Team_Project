#include "pch.h"
#include "Troll_Grogy.h"
#include "BTBlackBoard.h"
#include "BlackBoardKey.h"
#include "ComAnimator.h"
#include "ComCharacterMoveIntent.h"
NS_USING(Client)
CTrollGroggy::CTrollGroggy()
{
}

CTrollGroggy::~CTrollGroggy()
{

}

HRESULT		CTrollGroggy::Initialize(CTroll* pTroll)
{
	m_AnimTable[0] = MON_ANIM_FSM{.iAnimIndex = pTroll->Find_AnimIndex("AN_SK_Troll_ArmoredTroll_CMB_Master_LOD0_Skeleton_Trl_Cmbt_Atk_Charge_Knee_Start_anm.bin")
		,.fBlend = 0.1f, .fSkillRatio = 0.6f,.bLoop = false};
	m_AnimTable[1] = MON_ANIM_FSM{ .iAnimIndex = pTroll->Find_AnimIndex("AN_SK_Troll_ArmoredTroll_CMB_Master_LOD0_Skeleton_Trl_Cmbt_Atk_Charge_Knee_Loop_anm.bin")
		,.fBlend = 0.1f, .fSkillRatio = 0.f,.bLoop = false };
	m_AnimTable[2] = MON_ANIM_FSM{ .iAnimIndex = pTroll->Find_AnimIndex("AN_SK_Troll_ArmoredTroll_CMB_Master_LOD0_Skeleton_Trl_Cmbt_Atk_Charge_Knee_End_anm.bin")
			,.fBlend = 0.1f, .fSkillRatio = 0.6f, .bLoop = false };

	return S_OK;
}
void CTrollGroggy::Enter(CStateMachine* pStateMachine)
{
	CTroll* pTroll = pStateMachine->GetOwner<CTroll>();

	if (nullptr == pTroll) return;
	auto pBB = pTroll->Get_BlackBoard();
	if (nullptr == pBB) return;

	//MONSOUND Sound_Desc{};
	//_float3 vPos = pTroll->GetTransform().GetPosition();
	//Sound_Desc.SoundKey = "Hit";
	//Sound_Desc.SoundPlay = SOUND_PLAY_DESC{ .fVolume = 0.8f,.bLoop = false, };
	//Sound_Desc.str3DSound = SOUND_3D_DESC{ .vPosition = vPos ,.fMinDistance = 1.f, .fMaxDistance = 200.f, .eRolloff = SOUND_3D_ROLLOFF::LINEAR };
	//pTroll->Play_Sound(Sound_Desc);

	m_iIndex = 0;
}

void CTrollGroggy::Exit(CStateMachine* pStateMachine)
{
	auto pTroll = pStateMachine->GetOwner<CTroll>();
	if (nullptr == pTroll) return;

	pTroll->Set_Break(false);
	pTroll->ReActiveTable();
}

void CTrollGroggy::PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta)
{
}

void CTrollGroggy::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto pTrollFsm = Cast<CMon_State>(pStateMachine);
	if (nullptr == pTrollFsm) return;

	auto pTroll = pStateMachine->GetOwner<CTroll>();
	if (nullptr == pTroll) return;

	if (Play_Hit_Anim(pTroll))
		pTrollFsm->Request_State(MON_STATE::COMBAT);

}

_bool CTrollGroggy::Play_Hit_Anim(CTroll* pTroll)
{
	auto pAnimator = pTroll->Get_Animator();
	if (nullptr == pAnimator) return true;
	auto pMove = pTroll->Get_MoveIntent();
	if (nullptr == pMove) return true;
	auto pTarget = pTroll->Get_Target();
	if (nullptr == pTarget) return true;
		if (m_iIndex >= 3)
		return true;
	
	MON_ANIM_FSM Anim = m_AnimTable[m_iIndex];
	_float fRatio = pAnimator->GetPlayAnimRatio();
	if (m_iIndex == 0)
	{
		if (Anim.fSkillRatio >= fRatio)
		{
			_float3 vDir{};

			XMStoreFloat3(&vDir, -pTroll->GetTransform().GetState(STATE::LOOK));
			pMove->SetMoveIntent(vDir, 8.f);
		}
	}
	if (m_iIndex == 2)
	{
		if (Anim.fSkillRatio <= fRatio)
		{
			_vector vTargetPos = XMLoadFloat3(&pTarget->GetTransform().GetPosition());
			_vector vSrcPos = XMLoadFloat3(&pTroll->GetTransform().GetPosition());

			_float3 vDir{};

			XMStoreFloat3(&vDir, XMVector3Normalize(vTargetPos - vSrcPos));
			pMove->SetFacingIntent(vDir, 60.f);

		}
	}
	pAnimator->Play_Anim(Anim.iAnimIndex, Anim.bLoop,Anim.fBlend);
	
	if (pAnimator->GetFinish())
		++m_iIndex;

	return false;
}

SPtr<CTrollGroggy> CTrollGroggy::Create(CTroll* pTroll)
{
	auto pInstance = ToSPtr(new CTrollGroggy{});
	if (FAILED(pInstance->Initialize(pTroll)))
	{
		MSG_BOX("Failed to create CTrollGroggy");
		return nullptr;
	}

	return pInstance;
}
