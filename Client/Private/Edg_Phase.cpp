#include "pch.h"
#include "Edg_Phase.h"
#include "BTBlackBoard.h"
#include "BlackBoardKey.h"
#include "EnderDragon_State.h"
#include "ComCharacterMoveIntent.h"
#include "ComAnimator.h"
#include "ComBeHavior.h"
NS_USING(Client)
CEdg_Phase::CEdg_Phase()
{
}

CEdg_Phase::~CEdg_Phase()
{
}
HRESULT CEdg_Phase::Initialize(const _string& strLevelTag)
{
	if (strLevelTag != MagicEnumToStringView(LEVEL::LAST_BOSS_RANROK))
		return S_OK;

	if (FAILED(Load_Phase("PHASE2", m_PhasePos[ETOUI(DRAGON_PHASE::PHASE2)])))
		return E_FAIL;
	if (FAILED(Load_Phase("PHASE3", m_PhasePos[ETOUI(DRAGON_PHASE::PHASE3)])))
		return E_FAIL;
	if (FAILED(Load_Phase("PHASE4", m_PhasePos[ETOUI(DRAGON_PHASE::PHASE4)])))
		return E_FAIL;
	if (FAILED(Load_Phase("PHASE5", m_PhasePos[ETOUI(DRAGON_PHASE::PHASE5)])))
		return E_FAIL;
	//if (FAILED(Load_Phase("PHASE6", m_PhasePos[ETOUI(DRAGON_PHASE::PHASE6)])))
	//	return E_FAIL;



	return S_OK;
}
void CEdg_Phase::Enter(CStateMachine* pStateMachine)
{
	CEnderDragon* pDragon = pStateMachine->GetOwner<CEnderDragon>();

	if (nullptr == pDragon) return;

	auto pBB = pDragon->Get_BlackBoard();
	if (nullptr == pBB) return;

	auto pPhase = pBB->Get_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE);
	if (nullptr == pPhase) return;

	m_ePhase = *pPhase;
	m_eNum = EDG_SPAWN_NUMBER::FIRST;
	m_eNextPhase = DRAGON_PHASE::END;
	m_bNext = false;
	switch (m_ePhase)
	{
	case DRAGON_PHASE::PHASE2:
		m_Anims[ETOUI(DRAGON_PHASE::PHASE2)].push_back(EDG_ANIM_FSM{ .iAnimIndex =
		pDragon->Find_AnimIndex("AN_SK_ConjuredDragon_LOD0_Skeleton_Drgn_Cnjrd_FireBurst_Reel_anm.bin"),.fBlend = 0.1f });
		break;
	case DRAGON_PHASE::PHASE3:
		m_Anims[ETOUI(DRAGON_PHASE::PHASE3)].push_back(EDG_ANIM_FSM{ .iAnimIndex =
		pDragon->Find_AnimIndex("AN_SK_ConjuredDragon_LOD0_Skeleton_Drgn_Cnjrd_Hover_Loop_anm.bin"),.fBlend = 0.1f });
		break;
	case DRAGON_PHASE::PHASE4:
		pDragon->Set_HideOnBush(true);
		break;
	case DRAGON_PHASE::PHASE5:
		m_Anims[ETOUI(DRAGON_PHASE::PHASE5)].push_back(EDG_ANIM_FSM{ .iAnimIndex =
		pDragon->Find_AnimIndex("AN_SK_ConjuredDragon_LOD0_Skeleton_Drgn_Cmbt_Hover_Land_anm.bin"),.fBlend = 0.1f });

		pDragon->Set_HideOnBush(true);
		break;
	}
	m_iEffectID = INVALID_EFFECT_INSTANCE_ID;
}

void CEdg_Phase::Exit(CStateMachine* pStateMachine)
{
	auto pDragon = pStateMachine->GetOwner<CEnderDragon>();
	if (nullptr == pDragon) return;

	auto pBB = pDragon->Get_BlackBoard();
	if (nullptr == pBB) return;

	pDragon->ReActiveTable();
	pDragon->Set_StateFinished(false);
	pDragon->Set_HideOnBush(false);

	pDragon->Set_Break(false);
	pDragon->ReActiveTable();
	if (m_ePhase != DRAGON_PHASE::PHASE2)
	{
		_float3 vLeftPos{}, vRightPos{};
		//좌우 무빙
		_float3 vPos = pDragon->GetTransform().GetPosition();
		_vector vDir = XMVector3Normalize(pDragon->GetTransform().GetState(STATE::RIGHT));
		XMStoreFloat3(&vLeftPos, XMLoadFloat3(&vPos) + -vDir * 15.f);
		XMStoreFloat3(&vRightPos, XMLoadFloat3(&vPos) + vDir * 15.f);

		pBB->Set_Value<_float3>(EDG_KEY::LPATROL, vLeftPos);
		pBB->Set_Value<_float3>(EDG_KEY::RPATROL, vRightPos);
	}
}

void CEdg_Phase::PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta)
{
	
}

void CEdg_Phase::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto pDragonFsm = Cast<CEnderDragon_State>(pStateMachine);
	if (nullptr == pDragonFsm) return;

	auto pDragon = pDragonFsm->GetOwner<CEnderDragon>();
	if (nullptr == pDragon) return;

	//반대로했네 귀찮아..
	switch (m_eNum)
	{
	case EDG_SPAWN_NUMBER::FIRST:
		Phase_After_Action(pDragon, fTimeDelta);
		break;
	case EDG_SPAWN_NUMBER::SECOND:
		Phase_Change_Action(pDragon, fTimeDelta);
		break;
	case EDG_SPAWN_NUMBER::THIRD:
		Phase_Before_Action(pDragon, fTimeDelta);
		break;
	case EDG_SPAWN_NUMBER::FOUR:
		auto pBB = pDragon->Get_BlackBoard();
		if (nullptr == pBB) return;
		End(pDragonFsm, pBB);
		break;
	}

}
HRESULT CEdg_Phase::Load_Phase(const _string& PhaseName, std::list<_float3>& PhasePoses)
{
	auto pRes = CGameInstance::Get().GetResourceFirst<CResJson>("EDGWAYPT", PhaseName);
	if (nullptr == pRes)
	{
		MSG_BOX("Load Failed Json To EDGWAYPT Phase");
		return E_FAIL;
	}
	auto json = pRes->Get_Json();
	JsonSaveLoadManager::LoadJsonTypeFloat3list(json, PhaseName, PhasePoses);
	return S_OK;
}
_bool CEdg_Phase::MovePhase(CEnderDragon* pDragon, _float fTimeDelta)
{
	auto pMoveIntent = pDragon->Get_MoveIntent();
	if (nullptr == pMoveIntent)return false;
	
	if (m_PhasePos[ETOUI(m_ePhase)].empty()) return true;
	
	_vector vNextPos = XMLoadFloat3(&m_PhasePos[ETOUI(m_ePhase)].front());
	_vector vCurPos  = XMLoadFloat3(&pDragon->GetTransform().GetPosition());

	_vector vToNext = vNextPos - vCurPos;
	Effect_All(pDragon, fTimeDelta);
	if (!m_bNext)
	{
		XMStoreFloat3(&m_vLastDir, XMVector3Normalize(pDragon->GetTransform().GetState(STATE::LOOK)));
		XMStoreFloat3(&m_vNextDir, XMVector3Normalize(vNextPos - vCurPos));
		m_bNext = true;
	}
	_float fDot = XMVectorGetX(XMVector3Dot(XMVector3Normalize(vToNext), XMLoadFloat3(&m_vNextDir)));
	_float fDist = XMVectorGetX(XMVector3Length(vNextPos - vCurPos));
	if (fDist <= 0.5f || fDot < 0.f)
	{
		m_PhasePos[ETOUI(m_ePhase)].pop_front();
		m_bNext = false;
		m_fTick = 0.f;
	}

	m_fTick += fTimeDelta;
	_float t = std::min(m_fTick / 0.5f, 1.f);

	_float3 vLerpDir{};
	XMStoreFloat3(&vLerpDir, XMVector3Normalize(XMVectorLerp(XMLoadFloat3(&m_vLastDir), XMLoadFloat3(&m_vNextDir), t)));
	
	pMoveIntent->SetMoveIntent(vLerpDir, 50.f);
	pMoveIntent->SetFacingIntent(vLerpDir, 60.f);
	return false;
}
_bool CEdg_Phase::MovePhase3(CEnderDragon* pDragon, _float fTimeDelta)
{
	auto pMoveIntent = pDragon->Get_MoveIntent();
	if (nullptr == pMoveIntent)return false;
	auto pAnimator = pDragon->Get_Animator();
	if (nullptr == pAnimator) return false;
	
	auto pTarget = pDragon->Get_Target();
	if (nullptr == pTarget) return false;

	if (m_PhasePos[ETOUI(m_ePhase)].empty()) return true;


	_vector vNextPos = XMLoadFloat3(&m_PhasePos[ETOUI(m_ePhase)].front());
	_vector vCurPos = XMLoadFloat3(&pDragon->GetTransform().GetPosition());
	_vector vToNext = vNextPos - vCurPos;
	_vector vTargetPos = XMLoadFloat3(&pTarget->GetTransform().GetPosition());
	_float3 vTargetDir{};
	XMStoreFloat3(&vTargetDir, XMVector3Normalize(vTargetPos - vCurPos));
	if (!m_Anims[ETOUI(m_ePhase)].empty())
	{
		pAnimator->Play_Anim(m_Anims[ETOUI(m_ePhase)].front().iAnimIndex, true, m_Anims[ETOUI(m_ePhase)].front().fBlend);
	}
		
	if (!m_bNext)
	{
		XMStoreFloat3(&m_vNextDir, XMVector3Normalize(vNextPos - vCurPos));
		m_bNext = true;
	}
	_float fDot = XMVectorGetX(XMVector3Dot(XMVector3Normalize(vToNext), XMLoadFloat3(&m_vNextDir)));
	_float fDist = XMVectorGetX(XMVector3Length(vNextPos - vCurPos));
	if (fDist <= 0.5f || fDot < 0.f)
	{
		m_PhasePos[ETOUI(m_ePhase)].pop_front();
		m_bNext = false;
		m_fTick = 0.f;
	}

	
	pMoveIntent->SetMoveIntent(m_vNextDir, 20.f);
	pMoveIntent->SetFacingIntent(vTargetDir, 60.f);
	return false;
}
void CEdg_Phase::Befor_Action2(CEnderDragon* pDragon, _float fTimeDelta)
{
	auto pAnimator = pDragon->Get_Animator();
	if (nullptr == pAnimator) return;

	_float t = std::min(m_fTick / 3.f, 1.f);
	m_fTick += fTimeDelta;

	_float fLerp = 1.f - (0.f + 1.f) * t;
	pDragon->Set_Dissolve(fLerp);
	pDragon->Set_HideOnBush(false);
	if (t >= 1.f )
	{
		m_fTick = 0.f;
		m_eNum = EDG_SPAWN_NUMBER::FOUR;
		return;
	}

	if (m_iEffectID == INVALID_EFFECT_INSTANCE_ID)
		Effect_Single(pDragon, "RanrokStaySmoke");
	
}
void CEdg_Phase::Before_Action5(CEnderDragon* pDragon, _float fTimeDelta)
{
	auto pAnimator = pDragon->Get_Animator();
	if (nullptr == pAnimator) return;

	EDG_ANIM_FSM eAnim = m_Anims[ETOUI(m_ePhase)].front();
	pAnimator->Play_Anim(eAnim.iAnimIndex, false, eAnim.fBlend);

	pDragon->Set_HideOnBush(false);
	if (pAnimator->GetFinish())
	{
		m_eNum = EDG_SPAWN_NUMBER::FOUR;
		return;
	}

	if (m_iEffectID == INVALID_EFFECT_INSTANCE_ID)
		Effect_Single(pDragon, "RanrokStaySmoke");
}
void CEdg_Phase::After_Action2(CEnderDragon* pDragon, _float fTimeDelta)
{
	auto pAnimator = pDragon->Get_Animator();
	if (nullptr == pAnimator) return;

	m_fTick += fTimeDelta;
	_float t = std::min(m_fTick / 3.f, 1.f);

	_float fLerp = 0.f + (1.f - 0.f) * t;
	pDragon->Set_Dissolve(fLerp);

	if (t >=1.f)
	{
		pDragon->Set_HideOnBush(true);
		m_fTick = 0.f;
		m_eNum = EDG_SPAWN_NUMBER::SECOND;
		return;
	}

	EDG_ANIM_FSM eAnim = m_Anims[ETOUI(m_ePhase)].front();
	pAnimator->Play_Anim(eAnim.iAnimIndex, false, eAnim.fBlend);

	if (m_iEffectID == INVALID_EFFECT_INSTANCE_ID)
		Effect_Single(pDragon, "RanrokStaySmoke");

	
}
void CEdg_Phase::Phase_Before_Action(CEnderDragon* pDragon, _float fTimeDelta)
{
	switch (m_ePhase)
	{
	case DRAGON_PHASE::PHASE1:
		m_eNum = EDG_SPAWN_NUMBER::FOUR;
		break;
	case DRAGON_PHASE::PHASE2:
		Befor_Action2(pDragon, fTimeDelta);
		break;
	case DRAGON_PHASE::PHASE3:
		m_eNum = EDG_SPAWN_NUMBER::FOUR;
		break;
	case DRAGON_PHASE::PHASE4:
		m_eNum = EDG_SPAWN_NUMBER::FOUR;
		break;
	case DRAGON_PHASE::PHASE5:
		Before_Action5(pDragon, fTimeDelta);
		break;
	}
}

void CEdg_Phase::Phase_Change_Action(CEnderDragon* pDragon, _float fTimeDelta)
{
	if (m_ePhase == DRAGON_PHASE::PHASE3)
	{
		if (MovePhase3(pDragon, fTimeDelta))
		{
			m_fTick = 0.f;
			m_eNextPhase = m_ePhase;
			m_eNum = EDG_SPAWN_NUMBER::THIRD;
		}
	}
	else
	{
		if (MovePhase(pDragon, fTimeDelta))
		{
			m_fTick = 0.f;
			m_eNextPhase = m_ePhase;
			m_eNum = EDG_SPAWN_NUMBER::THIRD;
			if (m_ePhase == DRAGON_PHASE::PHASE5)
			{
				pDragon->Set_HideOnBush(false);
				auto pBT = pDragon->GetComponent<CComBeHavior>("Com_BT");
				if (nullptr == pBT) return;
				pBT->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::ADD);
			}
		}
	}
}
void CEdg_Phase::Phase_After_Action(CEnderDragon* pDragon, _float fTimeDelta)
{
	switch (m_ePhase)
	{
	case DRAGON_PHASE::PHASE1:
		m_eNum = EDG_SPAWN_NUMBER::SECOND;
		break;
	case DRAGON_PHASE::PHASE2:
		After_Action2(pDragon, fTimeDelta);
		break;
	case DRAGON_PHASE::PHASE3:
		m_eNum = EDG_SPAWN_NUMBER::SECOND;
		break;
	case DRAGON_PHASE::PHASE4:

		m_eNum = EDG_SPAWN_NUMBER::SECOND;
		break;
	case DRAGON_PHASE::PHASE5:

		m_eNum = EDG_SPAWN_NUMBER::SECOND;
		break;
	}
	
}
void CEdg_Phase::Effect_All(CEnderDragon* pDragon, _float fTimeDelta)
{
	m_fAngle += 180.f * fTimeDelta * 2.f;

	_matrix matWorld = XMMatrixRotationZ(XMConvertToRadians(m_fAngle)) * pDragon->GetTransform().GetLoadedWorldMatrix();

	auto TransformTrailPoint = [&matWorld](const _float3& localPoint)
		{
			_float3 worldPoint{};
			XMStoreFloat3(&worldPoint, XMVector3TransformCoord(XMLoadFloat3(&localPoint), matWorld));
			return worldPoint;
		};

	_float3 vstart{};
	_float3 vend{};

	{
		vstart = TransformTrailPoint({ 0.f, 3.5f, 0.f });
		vend = TransformTrailPoint({ 0.f, 2.5f, 0.f });
		CGameInstance::Get().AddTrailPoint("RanrokTrail1", "RanrokTrail1", pDragon->GetHandle(), vstart, vend);

		vstart = TransformTrailPoint({ 0.f, 1.5f, -3.f });
		vend = TransformTrailPoint({ 0.f, 0.5f, -3.f });
		CGameInstance::Get().AddTrailPoint("RanrokTrail2", "RanrokTrail2", pDragon->GetHandle(), vstart, vend);

		vstart = TransformTrailPoint({ 0.f, 1.5f, 3.f });
		vend = TransformTrailPoint({ 0.f, 0.5f, 3.f });
		CGameInstance::Get().AddTrailPoint("RanrokTrail3", "RanrokTrail3", pDragon->GetHandle(), vstart, vend);

		vstart = TransformTrailPoint({ 0.f, -0.5f, -2.f });
		vend = TransformTrailPoint({ 0.f, -1.5f, -2.f });
		CGameInstance::Get().AddTrailPoint("RanrokTrail4", "RanrokTrail4", pDragon->GetHandle(), vstart, vend);

		vstart = TransformTrailPoint({ 0.f, -0.5f, 2.f });
		vend = TransformTrailPoint({ 0.f, -1.5f, 2.f });
		CGameInstance::Get().AddTrailPoint("RanrokTrail5", "RanrokTrail5", pDragon->GetHandle(), vstart, vend);
	}
	

	m_fSpawnTick += fTimeDelta;

	if (m_fSpawnTick > 0.1f)
	{
		Effect_Single(pDragon, "RanrokMoveSmoke");
		m_fSpawnTick = 0.f;
	}
	else
	{
		if (m_iEffectID != INVALID_EFFECT_INSTANCE_ID)
			CGameInstance::Get().SetEffectWorldMatrix(m_iEffectID, *pDragon->GetTransform().GetWorldMatrix());
	}
}
void CEdg_Phase::Effect_Single(CEnderDragon* pDragon, const _string& strName)
{
	m_iEffectID = CGameInstance::Get().PlayEffect(strName, *pDragon->GetTransform().GetWorldMatrix(), _vector{},
		[this](EFFECT_INSTANCE_ID effectId, EFFECT_FINISH_REASON reason)
		{
			if (effectId != m_iEffectID)
				return;
			m_iEffectID = INVALID_EFFECT_INSTANCE_ID;
		});
}
void CEdg_Phase::End(CEnderDragon_State* pStateMachine, CBTBlackBoard* pBlackBoard)
{
	if (m_eNextPhase != DRAGON_PHASE::END)
	{
		pBlackBoard->Set_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE, m_eNextPhase);
		pStateMachine->Request_State(EDG_STATE::COMBAT);
	}
}
SPtr<CEdg_Phase> CEdg_Phase::Create(const _string& strLevelTag)
{
	auto pInstance = ToSPtr(new CEdg_Phase{});
	if (FAILED(pInstance->Initialize(strLevelTag)))
	{
		MSG_BOX("Failed to create CEdg_Phase");
		return nullptr;
	}

	return pInstance;
}
