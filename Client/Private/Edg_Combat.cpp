#include "pch.h"
#include "Edg_Combat.h"
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
		m_RandomBalls[ETOUI(DRAGON_PHASE::PHASE5)].push_back(RAND_BALL_DESC{ .vPos = {Randf(-10.f,10.f),Randf(15.f,20.f),Randf(-10.f,10.f)} ,.fDist = Randf(10.f,20.f) });
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
		m_fMaxTick = 15.f;
		break;
	case DRAGON_PHASE::PHASE3:
		m_fMaxTick = 15.f;
		break;
	case DRAGON_PHASE::PHASE4:
		m_fMaxTick = 15.f;
		break;
	case DRAGON_PHASE::PHASE5:
		m_fMaxTick = 15.f;
		break;
	}

	if (m_ePhase != DRAGON_PHASE::PHASE5)
	{
		MONSOUND Sound_Desc{};
		_float3 vPos = pDragon->GetTransform().GetPosition();
		Sound_Desc.SoundKey = "WingDefault";
		Sound_Desc.SoundPlay = SOUND_PLAY_DESC{ .fVolume = 0.8f,.bLoop = true, };
		Sound_Desc.str3DSound = SOUND_3D_DESC{ .vPosition = vPos ,.fMinDistance = 1.f, .fMaxDistance = 200.f, .eRolloff = SOUND_3D_ROLLOFF::LINEAR };
		m_iWingSound = pDragon->Play_Sound(Sound_Desc);

	}
}

void CEdg_Combat::Exit(CStateMachine* pStateMachine)
{
	auto pSoundManager = CGameInstance::Get().GetSoundManager();

	if (m_iWingSound != INVALID_SOUND_ID)
	{
		pSoundManager->Stop(m_iWingSound);
		m_iWingSound = INVALID_SOUND_ID;
	}
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
	PlaySound(pDragon);
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
void CEdg_Combat::PlaySound(CEnderDragon* pDragon)
{
	if (m_iWingSound != INVALID_SOUND_ID)
	{
		CGameInstance::Get().GetSoundManager()->Set3DAttributes(
			m_iWingSound,
			pDragon->GetTransform().GetPosition()
		);
	}

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
