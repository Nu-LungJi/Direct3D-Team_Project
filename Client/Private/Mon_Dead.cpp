#include "pch.h"
#include "Monster.h"
#include "Mon_Dead.h"
#include "ComBeHavior.h"
#include "ComAnimator.h"
NS_USING(Client)
CMon_Dead::CMon_Dead()
{
}

CMon_Dead::~CMon_Dead()
{
}
HRESULT		CMon_Dead::Initialize(const _string& DeadAnim, CMonster* pMonster)
{
	m_iIndex = pMonster->Find_AnimIndex(DeadAnim);

	return S_OK;
}
void CMon_Dead::Enter(CStateMachine* pStateMachine)
{
	CMonster* pMonster= pStateMachine->GetOwner<CMonster>();

	if (nullptr == pMonster) return;
	auto pAnimator = pMonster->Get_Animator();
	if (nullptr == pAnimator) return;


	pMonster->Destory_Child();
	if (m_iIndex != -1)
	{
		pAnimator->Play_Anim(m_iIndex, false, 0.1f);
		MONSOUND Sound_Desc{};
		_float3 vPos = pMonster->GetTransform().GetPosition();
		Sound_Desc.SoundKey = "Dead";
		Sound_Desc.SoundPlay = SOUND_PLAY_DESC{ .fVolume = 0.8f,.bLoop = false, };
		Sound_Desc.str3DSound = SOUND_3D_DESC{ .vPosition = vPos ,.fMinDistance = 1.f, .fMaxDistance = 200.f, .eRolloff = SOUND_3D_ROLLOFF::LINEAR };
		pMonster->Play_Sound(Sound_Desc);
	}

}

void CMon_Dead::Exit(CStateMachine* pStateMachine)
{
}

void CMon_Dead::PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta)
{

}

void CMon_Dead::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	CMonster* pMonster = pStateMachine->GetOwner<CMonster>();

	if (nullptr == pMonster) return;

	auto pBT = pMonster->GetComponent<CComBeHavior>("Com_BT");
	if (nullptr == pBT) return;
	auto pAnimator = pMonster->Get_Animator();
	if (nullptr == pAnimator) return;


	if (pAnimator->GetFinish())
	{
		m_fTick += fTimeDelta;

		_float t = std::min(m_fTick / 1.f, 1.f);
		pMonster->Set_Dissolve(1.f * t);

		if (t >= 1.f)
			pMonster->Set_EndGame();
	}
}
SPtr<CMon_Dead> CMon_Dead::Create(const _string& DeadAnim, CMonster* pMonster)
{
	auto pInstance = ToSPtr(new CMon_Dead{});
	if (FAILED(pInstance->Initialize(DeadAnim, pMonster)))
	{
		MSG_BOX("Failed to create CMon_Dead");
		return nullptr;
	}

	return pInstance;
}
