#include "pch.h"
#include "Player_ProtegoSkill_State.h"

#include "GameInstance.h"
#include "Player.h"

NS_USING(Client)

void CPlayer_ProtegoSkill_State::Enter(CStateMachine* pStateMachine)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer)
		return;

	m_fElapsed = 0.f;
	m_iShieldEffectID = INVALID_EFFECT_INSTANCE_ID;

	SetSkillControl(*pPlayer, true, false, false);
	pPlayer->SetProtegoActive(true);

	_float4x4 shieldWorld = *pPlayer->GetTransform().GetWorldMatrix();
	shieldWorld._42 += 1.f;
	m_iShieldEffectID = CGameInstance::Get().PlayEffect(
		"Protego_Shield", shieldWorld, XMVectorZero(),
		[this](EFFECT_INSTANCE_ID effectId, EFFECT_FINISH_REASON)
		{
			if (effectId == m_iShieldEffectID)
				m_iShieldEffectID = INVALID_EFFECT_INSTANCE_ID;
		});
}

void CPlayer_ProtegoSkill_State::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer)
		return;

	m_fElapsed += fTimeDelta;

	if (m_iShieldEffectID != INVALID_EFFECT_INSTANCE_ID)
	{
		_float4x4 shieldWorld = *pPlayer->GetTransform().GetWorldMatrix();
		shieldWorld._42 += 1.f;
		CGameInstance::Get().SetEffectWorldMatrix(m_iShieldEffectID, shieldWorld);
	}

	if (m_fElapsed >= PROTEGO_DURATION)
	{
		pPlayer->PrepareLocomotionResume();
		RequestLocomotion(pStateMachine);
	}
}

void CPlayer_ProtegoSkill_State::Exit(CStateMachine* pStateMachine)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (pPlayer)
	{
		pPlayer->SetProtegoActive(false);
		ResetSkillControl(*pPlayer);
	}

	if (m_iShieldEffectID != INVALID_EFFECT_INSTANCE_ID)
	{
		const EFFECT_INSTANCE_ID oldEffectID = m_iShieldEffectID;
		m_iShieldEffectID = INVALID_EFFECT_INSTANCE_ID;
		CGameInstance::Get().StopEffect(oldEffectID);
	}
}

SPtr<CPlayer_ProtegoSkill_State> CPlayer_ProtegoSkill_State::Create()
{
	return ToSPtr(new CPlayer_ProtegoSkill_State{});
}
