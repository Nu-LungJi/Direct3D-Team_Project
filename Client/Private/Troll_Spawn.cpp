#include "pch.h"
#include "Troll_Spawn.h"
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
#include "ComCharacterMoveIntent.h"
#include "ComAnimator.h"
#include "ComModelInstance.h"

NS_USING(Client)
CTroll_Spawn::CTroll_Spawn()
{
}

CTroll_Spawn::~CTroll_Spawn()
{
}
HRESULT CTroll_Spawn::Initialize(const _string& strLevelTag)
{

	return S_OK;
}
void CTroll_Spawn::Enter(CStateMachine* pStateMachine)
{
	CTroll* pTroll = pStateMachine->GetOwner<CTroll>();

	if (nullptr == pTroll)
		return;
	auto* pMove = pTroll->Get_MoveIntent();
	if (nullptr == pMove) return;
	

	pTroll->Set_StateFinished(false);

	m_Anims[TRS_CHASE] = MON_ANIM_FSM{ .iAnimIndex =
		pTroll->Find_AnimIndex("AN_SK_Troll_ArmoredTroll_CMB_Master_LOD0_Skeleton_Trl_Cmbt_Atk_Charge_Loop_anm.bin"),.fBlend = 0.1f,.bLoop = true };

	m_Anims[TRS_FINISHE]=MON_ANIM_FSM{ .iAnimIndex =
		pTroll->Find_AnimIndex("AN_SK_Troll_ArmoredTroll_CMB_Master_LOD0_Skeleton_Trl_Cmbt_Atk_Charge_Stop_Turn_Lft_anm.bin"),.fBlend = 0.1f };

	m_vStartPos = _float3(260.353f, 40.679f, 138.799f);
	m_vEndPos = _float3(300.263f, 36.808f, 100.997f);
	XMStoreFloat3(&m_vFirstLook,
	XMVector3Normalize(XMLoadFloat3(&m_vEndPos) - XMLoadFloat3(&m_vStartPos)));
	pMove->SetFacingIntentImmediate(m_vFirstLook);

	FCinematicPlayOptions option{};
	option.eStartMode = ECinematicStartMode::Immediate;
	option.fStartBlendDuration = 0.f;
	option.eReturnMode = ECinematicReturnMode::Blend;
	option.fReturnBlendDuration = 1.5f;
	option.LookAtTargetHandle = pTroll->GetHandle();/*트롤핸들(optional)*/;

	CGameInstance::Get().PlayCinematic("TrollDoljin", pTroll->GetHandle(), option);
}
_bool CTroll_Spawn::Play_Anim(CTroll* pTroll, _float fTimeDelta, uint32_t iIndex)
{
	if (iIndex >= TRS_END)
		return true;

	auto pAnimator = pTroll->Get_Animator();
	if (nullptr == pAnimator) return true;

	MON_ANIM_FSM EdgAnim = m_Anims[iIndex];
	pAnimator->Play_Anim(EdgAnim.iAnimIndex, EdgAnim.bLoop, EdgAnim.fBlend);

	if (pAnimator->GetFinish())
		return true;

	return false;
}
void CTroll_Spawn::Idle(CTroll* pTroll, _float fTimeDelta)
{
	auto pMove = pTroll->Get_MoveIntent();
	if (nullptr == pMove) return;
	Play_Anim(pTroll, fTimeDelta, TRS_CHASE);

	_vector vSrcPos = XMLoadFloat3(&pTroll->GetTransform().GetPosition());

	_vector vStartPos = XMLoadFloat3(&m_vStartPos);
	_vector vEndPos = XMLoadFloat3(&m_vEndPos);
	_vector vLen = vEndPos - vStartPos;

	_vector vCheckDir = XMVector3Normalize(vEndPos - vSrcPos);
	if (XMVectorGetX(XMVector3Length(vLen)) <= 3.f || XMVectorGetX(XMVector3Dot(vCheckDir,XMLoadFloat3(&m_vFirstLook))) < 0 )
	{
		m_eState = MON_DEF_STATE::RUN;
	}

	pMove->SetMoveIntent(m_vFirstLook, 15.f);
	pMove->SetFacingIntentImmediate(m_vFirstLook);

}
void CTroll_Spawn::Run(CTroll* pTroll, _float fTimeDelta)
{
	if (Play_Anim(pTroll, fTimeDelta, TRS_FINISHE))
		m_eState = MON_DEF_STATE::END;

}
void CTroll_Spawn::End(CTroll* pTroll, CMon_State* pMonState, _float fTimeDelta)
{
	pMonState->Request_State(MON_STATE::COMBAT);
}
void CTroll_Spawn::Exit(CStateMachine* pStateMachine)
{
	auto pTroll = pStateMachine->GetOwner<CTroll>();
	if (nullptr == pTroll) return;


}

void CTroll_Spawn::PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta)
{
}

void CTroll_Spawn::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto pTrollFsm = Cast<CMon_State>(pStateMachine);
	if (nullptr == pTrollFsm) return;

	auto pTroll = pStateMachine->GetOwner<CTroll>();
	if (nullptr == pTroll) return;

	auto pBB = pTroll->Get_BlackBoard();
	if (nullptr == pBB) return;

	switch(m_eState)
	{
	case MON_DEF_STATE::IDLE:
		Idle(pTroll,fTimeDelta);
		break;
	case MON_DEF_STATE::RUN:
		Run(pTroll, fTimeDelta);
		break;
	case MON_DEF_STATE::END:
		End(pTroll, pTrollFsm, fTimeDelta);
		break;

	}

}

SPtr<CTroll_Spawn> CTroll_Spawn::Create(const _string& strLevelTag)
{
	auto pInstance = ToSPtr(new CTroll_Spawn{});
	if (FAILED(pInstance->Initialize(strLevelTag)))
	{
		MSG_BOX("Failed to create CTroll_Spawn");
		return nullptr;
	}

	return pInstance;
}
