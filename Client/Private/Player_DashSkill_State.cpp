#include "pch.h"
#include "Player_DashSkill_State.h"

#include "Player.h"
#include "ComCharacterMoveIntent.h"

#include "ComAnimator.h"
#include "PlayerAnimationRatioGuard.h"
#include "Trail_CPU.h"

#include "ComSound.h"
NS_USING(Client)

void CPlayer_DashSkill_State::Enter(CStateMachine* pStateMachine)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	CacheAnimationIndices(*pPlayer);
	if (m_iDashAnimIndex < 0 || m_iDashEndAnimIndex < 0)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	_vector vDashDirection = pPlayer->HasRawMoveInput() ? XMLoadFloat3(&pPlayer->GetRawMoveDirection())
			: XMVectorSetY(pPlayer->GetTransform().GetState(STATE::LOOK),0.f);

	auto* pAnimator = pPlayer->GetAnimator();
	if (!pAnimator)
	{
		RequestLocomotion(pStateMachine);
		return;

	}
	if (XMVectorGetX(XMVector3LengthSq(vDashDirection)) <=std::numeric_limits<_float>::epsilon())
	{
		vDashDirection = XMVectorSet(0.f, 0.f, 1.f, 0.f);
	}

	XMStoreFloat3(&m_vDashDirection,XMVector3Normalize(vDashDirection));

	SetSkillControl( *pPlayer, true, false, false);

	if (auto* pMoveIntent = pPlayer->GetMoveIntent())
		pMoveIntent->SetFacingIntentImmediate(m_vDashDirection);

	pPlayer->SetCurrentMoveSpeed(0.f);

	pAnimator->Play_Anim(m_iDashAnimIndex,false, 0.1f);  


	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
	m_fScaleTime = 0.f;
	m_fDashElapsed = 0.f;

	if (auto* pSound = pPlayer->GetSound())
	{
		pSound->PlaySlot2D(
			E::StringID{ "PLAYER_SKILL_DASH" },
			"./Resources/SampleClient/Sound/Player/SkillEffect/Dash/Dash.wav",
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::SFX,
				.fVolume = 0.4f,
				.fPitch = 1.f,
				.iPriority = 64,
				.bLoop = false
			});
	}
	pPlayer->SetInvincible(true);
	auto k = pPlayer->GetTransform().GetWorldMatrix();
	m_iDashBodyEffectID = CGameInstance::Get().PlayEffect("PlayerBodyDash", *pPlayer->GetTransform().GetWorldMatrix(), _vector{},
		[this, pPlayer](EFFECT_INSTANCE_ID effectId, EFFECT_FINISH_REASON reason)
		{

			if (effectId != m_iDashBodyEffectID)
				return;
			char msg[256];
			sprintf_s(msg, "[DashEffect Callback] effectId=%llu, memberId=%llu, reason=%d\n",
				static_cast<unsigned long long>(effectId),
				static_cast<unsigned long long>(m_iDashBodyEffectID),
				static_cast<int>(reason));
			OutputDebugStringA(msg);
			m_iDashBodyEffectID = INVALID_EFFECT_INSTANCE_ID;
			pPlayer->SetBodyEffectID(INVALID_EFFECT_INSTANCE_ID);
		});
	pPlayer->SetBodyEffectID(m_iDashBodyEffectID);

}


void CPlayer_DashSkill_State::CacheAnimationIndices(const CPlayer& player)
{
	if (m_bAnimationIndicesCached)
		return;

	m_iDashAnimIndex = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_DdgeRll_Blink_2Cmbt_anm.bin");
	//m_iDashAnimIndex = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_DdgeRll_Blink_anm.bin");
	//m_iDashAnimIndex = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_DdgeRll_Blink1_anm.bin");
	//m_iDashAnimIndex = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_DdgeRll_Blink1_Loop_anm.bin");
	m_iDashEndAnimIndex = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_DdgeRll_Blink1_anm.bin");

	m_bAnimationIndicesCached =
		m_iDashAnimIndex >= 0 &&
		m_iDashEndAnimIndex >= 0;

}
void CPlayer_DashSkill_State::Update(CStateMachine* pStateMachine,_float fTimeDelta)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	auto* pAnimator = pPlayer->GetAnimator();
	if (!pAnimator)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	m_fAnimRatio =
		PlayerAnimationRatioGuard::Sanitize(
			pAnimator->GetPlayAnimRatio());

	m_fScaleTime += fTimeDelta;

	const float fDuration = 0.3f; 
	float fRatio = std::clamp(m_fScaleTime / fDuration, 0.f, 1.f);

	_float3 vScale;


	switch (m_ePhase)
	{
	case PHASE::CAST:
		if (m_fAnimRatio >= CAST_START_RATIO)
		{
			// DASH 시작 ---------------------------------------------------------------------------------------------------
			m_ePhase = PHASE::DASH;    
			pPlayer->SetRenderInfluence(true);



		}
		break;

	case PHASE::DASH:
	
		if (fRatio <= 1.f)
		{
			vScale.x = std::lerp(vNormalScale.x, vSmallScale.x, fRatio);
			vScale.y = std::lerp(vNormalScale.y, vSmallScale.y, fRatio);
			vScale.z = std::lerp(vNormalScale.z, vSmallScale.z, fRatio);
			pPlayer->GetTransform().SetScale(vScale);
		}
	
		if (pPlayer->HasRawMoveInput())
		{
			_vector vDashDirection = XMLoadFloat3(&pPlayer->GetRawMoveDirection());
			vDashDirection = XMVectorSetY(vDashDirection, 0.f);

			if (XMVectorGetX(XMVector3LengthSq(vDashDirection)) >std::numeric_limits<_float>::epsilon())
			{
				XMStoreFloat3(&m_vDashDirection,XMVector3Normalize(vDashDirection));

				if (auto* pMoveIntent = pPlayer->GetMoveIntent())
				{
					pMoveIntent->SetFacingIntentImmediate(m_vDashDirection);
				}
			}
		}

		{
			const _float fRemainingTime = std::max(0.f, DASH_DURATION - m_fDashElapsed);
			const _float fMoveTime = std::min(fTimeDelta, fRemainingTime);

			if (fMoveTime > 0.f)
			{
				pPlayer->ApplyDirectionalMovement(
					m_vDashDirection,
					DASH_SPEED,
					fMoveTime);
				m_fDashElapsed += fMoveTime;
			}
		}

		{

			// 이펙트 발동
			_float4 fpos = _float4(pPlayer->GetTransform().GetPosition().x, pPlayer->GetTransform().GetPosition().y, pPlayer->GetTransform().GetPosition().z, 1);
			_vector pos = XMVectorSet(fpos.x, fpos.y, fpos.z, fpos.w);
			_vector lastSpawnPos = XMVectorSet(m_vSpwanPos.x, m_vSpwanPos.y, m_vSpwanPos.z, 1.f);
			_float distance = XMVectorGetX(
				XMVector3Length(pos - lastSpawnPos));
			//_float3 vstart, vend;
			//vstart = _float3(fpos.x, fpos.y + 2.5f, fpos.z);
			//vend = _float3(fpos.x, fpos.y - 2.5f, fpos.z);
			//CGameInstance::Get().AddTrailPoint("PlayerDashTrail1_CPU", "PlayerDashTrail1_CPU", pPlayer->GetHandle(), vstart, vend);
			_float3 deltaPos;
			XMStoreFloat3(&deltaPos, lastSpawnPos - pos);
			if (distance > m_fDistanceOffeset) {

				CGameInstance::Get().PlayEffect(
					"PlayerDashSmoke", *pPlayer->GetTransform().GetWorldMatrix(), pos);
				m_vSpwanPos = pPlayer->GetTransform().GetPosition();	
			
			}
			///
		}

		if (m_fDashElapsed >= DASH_DURATION)
		{
			// DASH 끝---------------------------------------------------------------------------------------------------
			m_ePhase = PHASE::RECOVERY;   
			pPlayer->PrepareLocomotionResume();
			ResetSkillControl(*pPlayer);
			pAnimator->Play_Anim(m_iDashEndAnimIndex, false, 0.24f);
			pPlayer->SetRenderInfluence(false);
			pAnimator->GetCurAnimState().fSpeed = 2.f;
			m_fScaleTime = 0.f;
			CGameInstance::Get().StopEffect(m_iDashBodyEffectID);


		}
		break;

	case PHASE::RECOVERY:
		if (fRatio <= 1.f)
		{
			vScale.x = std::lerp(vSmallScale.x, vNormalScale.x, fRatio);
			vScale.y = std::lerp(vSmallScale.y, vNormalScale.y, fRatio);
			vScale.z = std::lerp(vSmallScale.z, vNormalScale.z, fRatio);
			pPlayer->GetTransform().SetScale(vScale);
		}

		if (pPlayer->HasRawMoveInput())
		{
			_vector vRecoveryDirection = XMLoadFloat3(&pPlayer->GetRawMoveDirection());
			vRecoveryDirection = XMVectorSetY(vRecoveryDirection, 0.f);

			if (XMVectorGetX(XMVector3LengthSq(vRecoveryDirection)) >std::numeric_limits<_float>::epsilon())
			{
				XMStoreFloat3(&m_vDashDirection,XMVector3Normalize(vRecoveryDirection));

				if (auto* pMoveIntent = pPlayer->GetMoveIntent())
				{
					pMoveIntent->SetFacingIntent(m_vDashDirection, 360.f);
				}
			}
		}
		{
			_float4 fpos = _float4(pPlayer->GetTransform().GetPosition().x, pPlayer->GetTransform().GetPosition().y , pPlayer->GetTransform().GetPosition().z, 1);
			_vector pos = XMVectorSet(fpos.x, fpos.y, fpos.z, fpos.w);
			_vector lastSpawnPos = XMVectorSet(m_vSpwanPos.x, m_vSpwanPos.y, m_vSpwanPos.z, 1.f);
			_float distance = XMVectorGetX(
				XMVector3Length(pos - lastSpawnPos));

			//if (distance > 2) {
			//
			//	CGameInstance::Get().PlayEffect(
			//		"PlayerDashSmoke", *pPlayer->GetTransform().GetWorldMatrix(), pos);
			//	m_vSpwanPos = pPlayer->GetTransform().GetPosition();
			//}
			_float3 vstart, vend;
			vstart = _float3(fpos.x, fpos.y + 2.5f, fpos.z);
			vend = _float3(fpos.x, fpos.y - 2.5f, fpos.z);
			//CGameInstance::Get().AddTrailPoint("PlayerDashTrail1_CPU", "PlayerDashTrail1_CPU", vstart, vend);


			//if (m_iDashBodyEffectID != INVALID_EFFECT_INSTANCE_ID)
			//	CGameInstance::Get().SetEffectWorldMatrix(m_iDashBodyEffectID, *pPlayer->GetTransform().GetWorldMatrix());
		}

		if (m_fAnimRatio >= RECOVERY_EXIT_RATIO)
		{
			pPlayer->GetTransform().SetScale(vNormalScale);
			RequestLocomotion(pStateMachine);
		}
		break;
	}

}

void CPlayer_DashSkill_State::Exit(CStateMachine* pStateMachine)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (pPlayer)
	{
		ResetSkillControl(*pPlayer);
		pPlayer->GetTransform().SetScale(_float3{ 1.f, 1.f, 1.f });
	}
	pPlayer->SetInvincible(false);
	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
	m_vDashDirection = {};
	m_fScaleTime = 0.f;
	m_fDashElapsed = 0.f;
}

SPtr<CPlayer_DashSkill_State> CPlayer_DashSkill_State::Create()
{
	return ToSPtr(new CPlayer_DashSkill_State{});
}
