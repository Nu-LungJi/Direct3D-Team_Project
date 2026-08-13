#include "pch.h"
#include "Edg_Combat.h"
#include "EnderDragon.h"
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
#include "DragonSkill.h"
NS_USING(Client)
CEdg_Combat::CEdg_Combat()
{
}

CEdg_Combat::~CEdg_Combat()
{
}
HRESULT		CEdg_Combat::Initialize()
{
	for (uint32_t i = 0; i < 10; ++i)
	{
		m_RandomBalls[ETOUI(DRAGON_PHASE::PHASE1)].push_back(RAND_BALL_DESC{ .vPos = {Randf(-10.f,10.f),Randf(15.f,20.f),0} ,.fDist = Randf(10.f,20.f)});
		m_RandomBalls[ETOUI(DRAGON_PHASE::PHASE3)].push_back(RAND_BALL_DESC{ .vPos = {Randf(-10.f,10.f),Randf(15.f,20.f),0} ,.fDist = Randf(10.f,20.f) });
		m_RandomBalls[ETOUI(DRAGON_PHASE::PHASE4)].push_back(RAND_BALL_DESC{ .vPos = {Randf(-10.f,10.f),Randf(15.f,20.f),0} ,.fDist = Randf(10.f,20.f) });

	}

	
	return S_OK;
}
void CEdg_Combat::Enter(CStateMachine* pStateMachine)
{
	CEnderDragon* pDragon = pStateMachine->GetOwner<CEnderDragon>();

	if (nullptr == pDragon) return;

	auto pBB = pDragon->Get_BlackBoard();
	if (nullptr == pBB) return;

	auto pPhase = pBB->Get_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE);
	if (nullptr == pPhase) return;

	m_ePhase = *pPhase;
	
	switch (m_ePhase)
	{
	case DRAGON_PHASE::PHASE1:
		m_fMaxTick = 2.f;
		break;
	case DRAGON_PHASE::PHASE3:
		m_fMaxTick = 6.f;
		break;
	case DRAGON_PHASE::PHASE4:
		m_fMaxTick = 6.f;
		break;
	case DRAGON_PHASE::PHASE5:
		m_fMaxTick = 4.f;
		break;
	}

}

void CEdg_Combat::Exit(CStateMachine* pStateMachine)
{
}

void CEdg_Combat::PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta)
{
}

void CEdg_Combat::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto pDragon = pStateMachine->GetOwner<CEnderDragon>();
	if (nullptr == pDragon) return;

	auto pBB = pDragon->Get_BlackBoard();
	if (pBB == nullptr) return;


	m_fTick += fTimeDelta;

	if (m_fTick > m_fMaxTick)
	{
		auto& Ball = m_RandomBalls[ETOUI(m_ePhase)];
		if (Ball.empty())
			return;
		uint32_t iRand = rand() % Ball.size();

		RandomBall(pDragon, XMLoadFloat3(&Ball[iRand].vPos), Ball[iRand].fDist);
		m_fTick = 0.f;
	}

	
}
void CEdg_Combat::RandomBall(CEnderDragon* pDragon, _vector vPos, _float fDis)
{
	
	EDG_SKILL_INFO SkillInfo = pDragon->Get_SkillInfo(DRAGON_SKILL::RANDOMBALL);
	CDragonSkill::EDG_SKILL_DESC SkillDesc{};
	SkillDesc.hOwner = pDragon->GetHandle();
	SkillDesc.iBoneIndex = SkillInfo.iBoneIndex;
	SkillDesc.iOffsetBoneIndex = SkillInfo.iOffsetBoneIndex;
	SkillDesc.eType = SkillInfo.eType;
	auto SkillHandle = CGameInstance::Get().AddGameObjectToLayer(SkillInfo.LevelTag,
		SkillInfo.ProtoTag, SkillInfo.NameTag, &SkillDesc);
	if (!SkillHandle) return;

	auto pSkill = CGameInstance::Get().GetGameObjectByHandleT<CDragonSkill>(SkillHandle.value());
	if (nullptr == pSkill)
		return;
	EDG_ACSKT_DESC ACTable{};
	ACTable.fDist = fDis;
	ACTable.eType = DRAGON_SKILL::RANDOMBALL;
	ACTable.fLifeTime = m_fMaxTick;
	ACTable.SkillName = pDragon->Get_SkillName(static_cast<ATTMON>(ACTable.eType));
	pSkill->Active(ACTable, vPos);
}
SPtr<CEdg_Combat> CEdg_Combat::Create()
{
	auto pInstance = ToSPtr(new CEdg_Combat{});
	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to create CEdg_Combat");
		return nullptr;
	}

	return pInstance;
}
