#include "pch.h"
#include "Spider_Dead.h"
#include "ComBeHavior.h"
#include "ComAnimator.h"
NS_USING(Client)
CSpider_Dead::CSpider_Dead()
{
}

CSpider_Dead::~CSpider_Dead()
{
}
HRESULT		CSpider_Dead::Initialize()
{
	return S_OK;
}
void CSpider_Dead::Enter(CStateMachine* pStateMachine)
{
	CSpider* pSpider= pStateMachine->GetOwner<CSpider>();

	if (nullptr == pSpider) return;
	auto pAnimator = pSpider->Get_Animator();
	if (nullptr == pAnimator) return;

	int32_t iIndex = pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Rct_Death_v04_anm.bin");
	
	pAnimator->Play_Anim(iIndex, false, 0.1f);
}

void CSpider_Dead::Exit(CStateMachine* pStateMachine)
{
}

void CSpider_Dead::PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta)
{

}

void CSpider_Dead::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	CSpider* pSpider = pStateMachine->GetOwner<CSpider>();

	if (nullptr == pSpider) return;

	auto pBT = pSpider->GetComponent<CComBeHavior>("Com_BT");
	if (nullptr == pBT) return;
	auto pAnimator = pSpider->Get_Animator();
	if (nullptr == pAnimator) return;


	if (pAnimator->GetFinish())
	{
		m_fTick += fTimeDelta;

		_float t = std::min(m_fTick / 1.f, 1.f);
		pSpider->Set_Dissolve(1.f * t);

		if (t >= 1.f)
			pSpider->Set_EndGame();
	}
}
SPtr<CSpider_Dead> CSpider_Dead::Create()
{
	auto pInstance = ToSPtr(new CSpider_Dead{});
	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to create CSpider_Dead");
		return nullptr;
	}

	return pInstance;
}
