#include "pch.h"
#include "Spider_Spawn.h"
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
#include "ComCharacterMoveIntent.h"
#include "ComAnimator.h"
#include "ComModelInstance.h"
NS_USING(Client)
CSpider_Spawn::CSpider_Spawn()
{
}

CSpider_Spawn::~CSpider_Spawn()
{
}
HRESULT CSpider_Spawn::Initialize(const _string& strLevelTag)
{

	return S_OK;
}
void CSpider_Spawn::Enter(CStateMachine* pStateMachine)
{
	CSpider* pSpider = pStateMachine->GetOwner<CSpider>();

	if (nullptr == pSpider)
		return;

	
	pSpider->Set_StateFinished(false);

	m_Anims.push_back(MON_ANIM_FSM{ .iAnimIndex = 
		pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Spawn_BurrowUp_anm.bin"),.fBlend = 0.1f});

}
_bool CSpider_Spawn::Play_Anim(CSpider* pSpider, _float fTimeDelta)
{
	auto pTarget = pSpider->Get_Target();
	if (nullptr == pTarget) return true;

	auto pMove = pSpider->Get_MoveIntent();
	if (nullptr == pMove) return true;
	_float3 vDir{};
	XMStoreFloat3(&vDir, XMVector3Normalize(XMLoadFloat3(&pTarget->GetTransform().GetPosition()) -
		XMLoadFloat3(&pSpider->GetTransform().GetPosition())));

	pMove->SetFacingIntentImmediate(vDir);

	auto pAnimator = pSpider->Get_Animator();
	if (nullptr == pAnimator) return true;

	if (!m_Anims.empty())
	{
		if (pAnimator->GetFinish())
			return true;

	}

	MON_ANIM_FSM EdgAnim = m_Anims.front();

	pAnimator->Play_Anim(EdgAnim.iAnimIndex, false, EdgAnim.fBlend);

	return false;
}
void CSpider_Spawn::Exit(CStateMachine* pStateMachine)
{
	auto pSpider = pStateMachine->GetOwner<CSpider>();
	if (nullptr == pSpider) return;


}

void CSpider_Spawn::PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta)
{
}

void CSpider_Spawn::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto pSpiderFsm = Cast<CMon_State>(pStateMachine);
	if (nullptr == pSpiderFsm) return;

	auto pSpider = pStateMachine->GetOwner<CSpider>();
	if (nullptr == pSpider) return;

	auto pBB = pSpider->Get_BlackBoard();
	if (nullptr == pBB) return;

	if (Play_Anim(pSpider, fTimeDelta))
		pSpiderFsm->Request_State(MON_STATE::COMBAT);

}

SPtr<CSpider_Spawn> CSpider_Spawn::Create(const _string& strLevelTag)
{
	auto pInstance = ToSPtr(new CSpider_Spawn{});
	if (FAILED(pInstance->Initialize(strLevelTag)))
	{
		MSG_BOX("Failed to create CSpider_Spawn");
		return nullptr;
	}

	return pInstance;
}
