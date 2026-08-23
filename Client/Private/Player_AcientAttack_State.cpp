#include "pch.h"
#include "Player_AcientAttack_State.h"

#include "Player.h"
#include "ComAnimator.h"
#include "ComCharacterMoveIntent.h"
#include "PlayerAnimationRatioGuard.h"
#include "Player_Weapon.h"
#include "Monster.h"
#include "ClientEvents.h"
#include "Trail_CPU.h"
#include "PropBarrel.h"
#include "PlayerThirdPersonCamera.h"

NS_USING(Client)

namespace
{
	const StringID ACIENT_THROW_TIME_SCALE_TAG{ "AncientThrow_PreLaunch" };
	const StringID ACIENT_THROW_WAND_TRAIL_TAG{
		"AncientThrow_WandTrail_CPU" };
}

void CPlayer_AcientAttack_State::Enter(CStateMachine* pStateMachine)
{
	// 이전 시전이 비정상 종료된 경우 이 상태가 소유한 요청만 정리한다.
	EndThrowSlowMotion(true);

	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	m_hThrowBarrel = pPlayer->ConsumeAncientThrowTarget();
	const _bool bThrowBranch = m_hThrowBarrel.has_value();
	if (bThrowBranch)
	{
		auto* pTrail = dynamic_cast<CTrail_CPU*>(
			CGameInstance::Get().GetParticle(
				ACIENT_THROW_WAND_TRAIL_TAG,
				ACIENT_THROW_WAND_TRAIL_TAG));
		if (pTrail)
		{
			pTrail->Clear(pPlayer->GetHandle());
			pTrail->SetColor({ 0.28f, 0.48f, 1.f, 0.92f });
			pTrail->SetEmissive({ 0.16f, 0.3f, 1.f, 3.2f });
		}
	}
	else
	{
		auto* pTrail = dynamic_cast<CTrail_CPU*>(
			CGameInstance::Get().GetParticle(
				"Lightning_Trail", "Lightning_Trail"));
		if (pTrail)
		{
			pTrail->SetColor({
				67.f / 255.f, 97.f / 255.f, 174.f / 255.f, 1.f });
			pTrail->SetEmissive({
				51.f / 255.f, 77.f / 255.f, 126.f / 255.f, 4.f });
		}
	}
	if (bThrowBranch)
		m_hThrowDestination = pPlayer->GetTargetHandle();
	if (!bThrowBranch && !HasValidTarget(*pPlayer))
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	CacheAnimationIndices(*pPlayer);
	const auto iSkillIndex = ETOUI(ACIENT_SKILL::ACIENT_LIGHTENING);
	if ((bThrowBranch && m_iAncientThrowLeftAnimation < 0 &&
		m_iAncientThrowRightAnimation < 0) ||
		(!bThrowBranch && (m_AcientCast_Animations[iSkillIndex] < 0 ||
		m_AcientEnd_Animations[iSkillIndex] < 0)))
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

	SetSkillControl(*pPlayer, true, true, false);
	pPlayer->SetCurrentMoveSpeed(0.f);
	pPlayer->SetPlayerCurSKill(
		bThrowBranch ? PLAYER_SKILL_TYPE::DEFAULT : PLAYER_SKILL_TYPE::ACIENT_LIGHTNING);

	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
	m_fAcientElapsed = 0.f;
	m_bThrowReleased = false;
	m_fThrowSequenceElapsed = 0.f;
	m_fThrowPostLaunchUnscaledElapsed = 0.f;
	if (bThrowBranch)
	{
		auto* pBarrel = CGameInstance::Get()
			.GetGameObjectByHandleT<CPropBarrel>(*m_hThrowBarrel);
		if (!pBarrel || pBarrel->GetPendingDestroy())
		{
			RequestLocomotion(pStateMachine);
			return;
		}

		// 끌어온 오브젝트는 항상 플레이어 오른쪽에서 대기하므로
		// 오른손 투척 애니메이션을 우선 사용한다.
		int32_t animation = m_iAncientThrowRightAnimation;
		if (animation < 0)
			animation = m_iAncientThrowLeftAnimation >= 0
				? m_iAncientThrowLeftAnimation
				: m_iAncientThrowRightAnimation;
		m_vThrowPullStartCenter = pBarrel->GetVisualCenterPosition();
		m_vThrowPullStartRotation = pBarrel->GetTransform().GetQuaternion();
		if (!pBarrel->BeginAncientThrowControl())
		{
			RequestLocomotion(pStateMachine);
			return;
		}
		pAnimator->Play_Anim(animation, false, 0.18f);
		DEBUG_LOG("[AncientMagic] Throw target connected; animation started.\n");
		return;
	}

	// 고대마법 발동 이벤트 발행

		CGameInstance::Get().EventPublish<FAcientMagicStart>();
	}

void CPlayer_AcientAttack_State::CacheAnimationIndices(const CPlayer& player)
{
	if (m_bAnimationIndicesCached)
		return;

	const auto iSkillIndex = ETOUI(ACIENT_SKILL::ACIENT_LIGHTENING);
	m_AcientCast_Animations[iSkillIndex] = FindAnimationIndex(
		player,
		"AN_ProfessorSharp_MasterRig_DW_Cmbt_Atk_AOE_Lightning_Cast_Start_anm.bin");
	//m_AcientEnd_Animations[iSkillIndex] = FindAnimationIndex(
	//	player,
	//	"AN_ProfessorSharp_MasterRig_DW_Cmbt_Atk_AOE_Lightning_Cast_End_anm.bin");
	//m_AcientCast_Animations[iSkillIndex] = FindAnimationIndex(
	//	player,
	//	"AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Slam_Dwn_anm.bin");
	m_AcientEnd_Animations[iSkillIndex] = FindAnimationIndex(
		player,
		"AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Slam_Dwn_anm.bin");

	m_iAncientThrowLeftAnimation = FindAnimationIndex(
		player,
		"AN_ProfessorSharp_MasterRig_Hu_Cmbt_Parry_Counter_Atk_Fwd_ArmLft_Spin_Lft_Send_anm.bin");
	m_iAncientThrowRightAnimation = FindAnimationIndex(
		player,
		"AN_ProfessorSharp_MasterRig_Hu_Cmbt_Parry_Counter_Atk_Fwd_ArmRht_Spin_Rht_Send_anm.bin");

	m_bAnimationIndicesCached =
		m_AcientCast_Animations[iSkillIndex] >= 0 &&
		m_AcientEnd_Animations[iSkillIndex] >= 0 &&
		(m_iAncientThrowLeftAnimation >= 0 ||
		m_iAncientThrowRightAnimation >= 0);
}

_bool CPlayer_AcientAttack_State::UpdateThrowPull(
	CPlayer& player,
	_float fPullRatio)
{
	if (!m_hThrowBarrel)
		return false;

	auto* pBarrel = CGameInstance::Get().
		GetGameObjectByHandleT<CPropBarrel>(*m_hThrowBarrel);
	if (!pBarrel || pBarrel->GetPendingDestroy() ||
		pBarrel->GetBarrelState() != CPropBarrel::BARREL_STATE::CREATED)
	{
		return false;
	}

	_vector vLook = XMVectorSetY(
		player.GetTransform().GetState(STATE::LOOK), 0.f);
	_vector vRight = XMVectorSetY(
		player.GetTransform().GetState(STATE::RIGHT), 0.f);
	if (XMVectorGetX(XMVector3LengthSq(vLook)) <= FLT_EPSILON)
		vLook = XMVectorSet(0.f, 0.f, 1.f, 0.f);
	else
		vLook = XMVector3Normalize(vLook);
	if (XMVectorGetX(XMVector3LengthSq(vRight)) <= FLT_EPSILON)
		vRight = XMVector3Normalize(XMVector3Cross(
			XMVectorSet(0.f, 1.f, 0.f, 0.f), vLook));
	else
		vRight = XMVector3Normalize(vRight);

	const _vector vPlayerPosition =
		player.GetTransform().GetState(STATE::POSITION);
	const _vector vHoldPosition =
		vPlayerPosition +
		vRight * ACIENT_THROW_HOLD_SIDE_OFFSET +
		XMVectorSet(0.f, ACIENT_THROW_HOLD_HEIGHT, 0.f, 0.f) +
		vLook * ACIENT_THROW_HOLD_FORWARD_OFFSET;

	const _float fRawRatio = std::clamp(fPullRatio, 0.f, 1.f);
	const _float fSmoothedRatio =
		fRawRatio * fRawRatio * (3.f - 2.f * fRawRatio);
	_vector vPullPosition = XMVectorLerp(
		XMLoadFloat3(&m_vThrowPullStartCenter),
		vHoldPosition,
		fSmoothedRatio);
	vPullPosition = XMVectorSetY(
		vPullPosition,
		XMVectorGetY(vPullPosition) +
		std::sin(XM_PI * fSmoothedRatio) * ACIENT_THROW_PULL_ARC_HEIGHT);

	const _vector vSpinAxis = XMVector3Normalize(
		vRight * 0.75f + XMVectorSet(0.f, 1.f, 0.f, 0.f) * 0.25f);
	const _vector qSpin = XMQuaternionRotationAxis(
		vSpinAxis,
		XM_2PI * ACIENT_THROW_PULL_SPIN_TURNS * fSmoothedRatio);
	const _vector qRotation = XMQuaternionNormalize(XMQuaternionMultiply(
		qSpin,
		XMLoadFloat4(&m_vThrowPullStartRotation)));

	_float3 pullPosition{};
	_float4 pullRotation{};
	XMStoreFloat3(&pullPosition, vPullPosition);
	XMStoreFloat4(&pullRotation, qRotation);
	return pBarrel->SetAncientThrowVisualPose(pullPosition, pullRotation);
}

_bool CPlayer_AcientAttack_State::LaunchThrow(CPlayer& player)
{
	if (!m_hThrowBarrel || !m_hThrowDestination)
		return false;

	auto* pBarrel = CGameInstance::Get().
		GetGameObjectByHandleT<CPropBarrel>(*m_hThrowBarrel);
	if (!pBarrel || pBarrel->GetPendingDestroy() ||
		pBarrel->GetBarrelState() != CPropBarrel::BARREL_STATE::CREATED)
	{
		return false;
	}

	_float3 targetPosition{};
	if (auto* pMonster = CGameInstance::Get().
		GetGameObjectByHandleT<CMonster>(*m_hThrowDestination))
	{
		targetPosition = pMonster->GetHurtBoxPosition();
	}
	else if (auto* pDestination = CGameInstance::Get().
		GetGameObjectByHandle(*m_hThrowDestination);
		pDestination && !pDestination->GetPendingDestroy())
	{
		targetPosition = pDestination->GetTransform().GetPosition();
	}
	else
	{
		return false;
	}

	const _float3 barrelPosition = pBarrel->GetVisualCenterPosition();
	const _vector vDisplacement =
		XMLoadFloat3(&targetPosition) - XMLoadFloat3(&barrelPosition);
	if (XMVectorGetX(XMVector3LengthSq(vDisplacement)) <= FLT_EPSILON)
		return false;

	const _vector vLaunchDirection = XMVector3Normalize(vDisplacement);
	const _vector vLaunchVelocity =
		vLaunchDirection * ACIENT_THROW_DIRECT_SPEED;
	const _vector vHorizontalDisplacement = XMVectorSetY(vDisplacement, 0.f);
	const _float fHorizontalDistance = XMVectorGetX(
		XMVector3Length(vHorizontalDisplacement));

	_vector vSpinAxis{};
	if (fHorizontalDistance > FLT_EPSILON)
	{
		vSpinAxis = XMVector3Normalize(XMVector3Cross(
			XMVectorSet(0.f, 1.f, 0.f, 0.f),
			vHorizontalDisplacement));
	}
	else
	{
		vSpinAxis = player.GetTransform().GetState(STATE::RIGHT);
		vSpinAxis = XMVectorSetY(vSpinAxis, 0.f);
		if (XMVectorGetX(XMVector3LengthSq(vSpinAxis)) <= FLT_EPSILON)
			vSpinAxis = XMVectorSet(1.f, 0.f, 0.f, 0.f);
		else
			vSpinAxis = XMVector3Normalize(vSpinAxis);
	}

	_float3 linearVelocity{};
	_float3 angularVelocity{};
	XMStoreFloat3(&linearVelocity, vLaunchVelocity);
	XMStoreFloat3(
		&angularVelocity,
		vSpinAxis * ACIENT_THROW_SPIN_SPEED);
	return pBarrel->Launch(linearVelocity, angularVelocity);
}

void CPlayer_AcientAttack_State::EmitThrowWandTrail(CPlayer& player) const
{
	auto* pWeapon = CGameInstance::Get().
		GetGameObjectByHandleT<CPlayer_Weapon>(player.GetWeaponHandle());
	if (!pWeapon)
		return;

	const _float4x4 wandWorld = pWeapon->GetSpawnWorldMatrix();
	const _vector vWandPosition = XMVectorSet(
		wandWorld._41,
		wandWorld._42,
		wandWorld._43,
		1.f);
	_vector vRibbonAxis = XMVectorSet(
		wandWorld._11,
		wandWorld._12,
		wandWorld._13,
		0.f);
	if (XMVectorGetX(XMVector3LengthSq(vRibbonAxis)) <= FLT_EPSILON)
		vRibbonAxis = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	else
		vRibbonAxis = XMVector3Normalize(vRibbonAxis);

	constexpr _float THROW_WAND_TRAIL_HALF_WIDTH = 0.1f;
	_float3 trailStart{};
	_float3 trailEnd{};
	XMStoreFloat3(
		&trailStart,
		vWandPosition + vRibbonAxis * THROW_WAND_TRAIL_HALF_WIDTH);
	XMStoreFloat3(
		&trailEnd,
		vWandPosition - vRibbonAxis * THROW_WAND_TRAIL_HALF_WIDTH);
	CGameInstance::Get().AddTrailPoint(
		ACIENT_THROW_WAND_TRAIL_TAG,
		ACIENT_THROW_WAND_TRAIL_TAG,
		player.GetHandle(),
		trailStart,
		trailEnd);
}

void CPlayer_AcientAttack_State::BeginThrowSlowMotion()
{
	if (m_bThrowSlowMotionActive)
		return;

	TIME_SCALE_REQUEST_DESC Desc{};
	Desc.fTargetScale = ACIENT_THROW_SLOW_SCALE;
	Desc.fBlendIn = ACIENT_THROW_SLOW_BLEND_IN;
	Desc.fMaxUnscaledDuration = ACIENT_THROW_SLOW_MAX_UNSCALED_DURATION;
	Desc.fSafetyBlendOut = ACIENT_THROW_SLOW_BLEND_OUT;
	Desc.sTag = ACIENT_THROW_TIME_SCALE_TAG;
	m_bThrowSlowMotionActive = CGameInstance::Get().BeginTimeScale(Desc);
	if (m_bThrowSlowMotionActive)
	{
		if (auto* pCamera = dynamic_cast<CPlayerThirdPersonCamera*>(
			CGameInstance::Get().GetActiveCamera());
			pCamera && pCamera->BeginFovOverride(
				ACIENT_THROW_FOV_Y,
				ACIENT_THROW_FOV_BLEND_IN_RESPONSE))
		{
			m_hThrowFovCamera = pCamera->GetHandle();
		}
	}
}

void CPlayer_AcientAttack_State::EndThrowSlowMotion(_bool bImmediate)
{
	auto& GameInstance = CGameInstance::Get();
	if (m_hThrowFovCamera)
	{
		if (auto* pCamera = GameInstance.GetGameObjectByHandleT<
			CPlayerThirdPersonCamera>(*m_hThrowFovCamera))
		{
			pCamera->EndFovOverride(
				ACIENT_THROW_FOV_BLEND_OUT_RESPONSE);
		}
		m_hThrowFovCamera.reset();
	}

	if (!m_bThrowSlowMotionActive &&
		!GameInstance.IsTimeScaleActive(ACIENT_THROW_TIME_SCALE_TAG))
	{
		return;
	}

	if (bImmediate)
		GameInstance.CancelTimeScale(ACIENT_THROW_TIME_SCALE_TAG);
	else
		GameInstance.EndTimeScale(
			ACIENT_THROW_TIME_SCALE_TAG,
			ACIENT_THROW_SLOW_BLEND_OUT);

	m_bThrowSlowMotionActive = false;
}

void CPlayer_AcientAttack_State::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	pPlayer->SetCurrentMoveSpeed(0.f);
	if (m_hThrowBarrel)
	{
		auto* pAnimator = pPlayer->GetAnimator();
		if (!pAnimator)
		{
			RequestLocomotion(pStateMachine);
			return;
		}

		const _float throwAnimRatio = PlayerAnimationRatioGuard::Sanitize(
			pAnimator->GetPlayAnimRatio());
		EmitThrowWandTrail(*pPlayer);
		m_fThrowSequenceElapsed += std::max(0.f, fTimeDelta);
		const _float throwPullRatio = std::clamp(
			m_fThrowSequenceElapsed / ACIENT_THROW_PULL_DURATION,
			0.f,
			1.f);
		const _bool bThrowPullReady =
			m_fThrowSequenceElapsed >= ACIENT_THROW_PULL_DURATION;

		if (!m_bThrowReleased &&
			!UpdateThrowPull(*pPlayer, throwPullRatio))
		{
			DEBUG_LOG("[AncientMagic] Failed to pull throw target.\n");
			EndThrowSlowMotion();
			RequestLocomotion(pStateMachine);
			return;
		}

		// 접근 동작이 보이도록 플레이어에게 가까워지는 후반부터 슬로우를 건다.
		if (!m_bThrowReleased &&
			throwPullRatio >= ACIENT_THROW_SLOW_START_PULL_RATIO)
			BeginThrowSlowMotion();

		if (!m_bThrowReleased && !bThrowPullReady &&
			throwAnimRatio >= ACIENT_THROW_LAUNCH_RATIO)
		{
			// 발사 포즈는 유지하되 완전히 정지하지 않고 천천히 진행한다.
			pAnimator->GetCurAnimState().fSpeed =
				ACIENT_THROW_WAIT_ANIM_SPEED;
		}

		if (!m_bThrowReleased && bThrowPullReady)
		{
			pAnimator->GetCurAnimState().fSpeed = 1.f;
			if (!LaunchThrow(*pPlayer))
			{
				DEBUG_LOG("[AncientMagic] Failed to launch throw target.\n");
				EndThrowSlowMotion();
				RequestLocomotion(pStateMachine);
				return;
			}
			m_bThrowReleased = true;
			m_fThrowPostLaunchUnscaledElapsed = 0.f;
		}

		if (m_bThrowReleased && m_bThrowSlowMotionActive)
		{
			m_fThrowPostLaunchUnscaledElapsed +=
				CGameInstance::Get().GetUnscaledDelta();
			if (m_fThrowPostLaunchUnscaledElapsed >=
				ACIENT_THROW_SLOW_POST_LAUNCH_DURATION)
			{
				EndThrowSlowMotion();
			}
		}

		if (auto* pMoveIntent = pPlayer->GetMoveIntent())
		{
			if (throwAnimRatio < ACIENT_THROW_FACING_END_RATIO && m_hThrowDestination)
			{
				if (auto* pDestination = CGameInstance::Get().GetGameObjectByHandle(
					*m_hThrowDestination);
					pDestination && !pDestination->GetPendingDestroy())
				{
					_vector direction =
						pDestination->GetTransform().GetState(STATE::POSITION) -
						pPlayer->GetTransform().GetState(STATE::POSITION);
					direction = XMVectorSetY(direction, 0.f);
					if (XMVectorGetX(XMVector3LengthSq(direction)) > FLT_EPSILON)
					{
						_float3 facingDirection{};
						XMStoreFloat3(&facingDirection, XMVector3Normalize(direction));
						pMoveIntent->SetFacingIntent(
							facingDirection, ACIENT_THROW_TURN_SPEED);
					}
				}
			}
			else
			{
				pMoveIntent->ClearFacingIntent();
			}
		}

		if (throwAnimRatio >= ACIENT_THROW_STATE_RELEASE_RATIO ||
			pAnimator->GetFinish())
		{
			RequestLocomotion(pStateMachine);
		}
		return;
	}

	auto* pAnimator = pPlayer->GetAnimator();
	if (!pAnimator)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	m_fAnimRatio =PlayerAnimationRatioGuard::Sanitize(	pAnimator->GetPlayAnimRatio());

	switch (m_ePhase)
	{
	case PHASE::CAST:
		m_ePhase = PHASE::ATTACK;
		pAnimator->Play_Anim(
			m_AcientCast_Animations[ETOUI(ACIENT_SKILL::ACIENT_LIGHTENING)],
			false,
			0.24f);

		CGameInstance::Get().PlayEffect("LightningSound", *pPlayer->GetTransform().GetWorldMatrix());
		// 스킬 컷신 재생
		{
			FCinematicPlayOptions options{};
			options.eStartMode = ECinematicStartMode::Blend;
			options.fStartBlendDuration = 1.f;
			options.eReturnMode = ECinematicReturnMode::Blend;
			options.fReturnBlendDuration = 1.f;
			CGameInstance::Get().PlayCinematic("Lightning", pPlayer->GetHandle(), options);
		}
		break;

	case PHASE::ATTACK:
		m_fAcientElapsed += std::max(0.f, fTimeDelta);



		if (!m_bOnceLighting) {
			{
				m_fSpawnDelay += fTimeDelta;
				auto* pWeapon = CGameInstance::Get().GetGameObjectByHandleT<CPlayer_Weapon>(pPlayer->GetWeaponHandle());

				if (!pWeapon)
					return;

				const _float4x4 spawnWorld = pWeapon->GetSpawnWorldMatrix();
				_float3 vstart, vend;
				vstart = _float3(spawnWorld._41, spawnWorld._42 + 0.2f, spawnWorld._43);
				vend = _float3(spawnWorld._41, spawnWorld._42 - 0.2f, spawnWorld._43);
				CGameInstance::Get().AddTrailPoint("Lightning_Trail", "Lightning_Trail", pPlayer->GetHandle(), vstart, vend);


				if (m_fSpawnDelay > 0.03f) {
					CGameInstance::Get().PlayEffect("Lightning_Trail_Particle", pWeapon->GetSpawnWorldMatrix());
					m_fSpawnDelay = 0.f;
				}
			}
		}
		if (m_fAcientElapsed >= ACIENT_LIGHTENING_ATTACK_DURATION)
		{
			if (!m_bOnceLighting) {
				auto* pWeapon = CGameInstance::Get().GetGameObjectByHandleT<CPlayer_Weapon>(pPlayer->GetWeaponHandle());

				if (!pWeapon)
					return;
				CGameInstance::Get().PlayEffect("Lightning_Wand", pWeapon->GetSpawnWorldMatrix());

				m_bOnceLighting = true;
			}
			
			pAnimator->GetCurAnimState().fSpeed = 0.2f;
			if (m_fAcientElapsed >= ACIENT_LIGHTENING_ATTACK_STOP_DURATION) {
				m_ePhase = PHASE::RECOVERY;

				auto* Target = CGameInstance::Get().GetGameObjectByHandleT<CMonster>(pPlayer->GetTargetHandle());

				if (!Target)
					return;
				CGameInstance::Get().PlayEffect("Player_Lightning", *Target->GetTransform().GetWorldMatrix());
				pAnimator->Play_Anim(m_AcientEnd_Animations[ETOUI(ACIENT_SKILL::ACIENT_LIGHTENING)], false, 0.25f);
				pAnimator->GetCurAnimState().fSpeed = 1.f;

			}
		
		}
		break;

	case PHASE::RECOVERY:
		if (m_fAnimRatio >= RECOVERY_EXIT_RATIO) {
			//내리고 있는ㅇㅋ
	
			RequestLocomotion(pStateMachine);
		}
		
		if (m_fAnimRatio >= ACIENT_LIGHTENING_LAST_ATTACK && !m_bOnceLastLighting) {
			if (auto pMonster = CGameInstance::Get().GetGameObjectByHandleT<CMonster>(pPlayer->GetTargetHandle()))
				pMonster->Check_Table(PLAYER_SKILL_TYPE::ACIENT_LIGHTNING);
			m_bOnceLastLighting = true;
		
			//CGameInstance::Get().EventPublish(FRequestPlayerCameraShake
			//	{
			//	   1.f, // 강도 0 ~ 1
			//	   1.f, // 지속시간
			//	   15.f, // 초당 진동횟수
			//	});
		}
		
		break;
	}
}

void CPlayer_AcientAttack_State::Exit(CStateMachine* pStateMachine)
{
	EndThrowSlowMotion();

	if (m_hThrowBarrel && !m_bThrowReleased)
	{
		if (auto* pBarrel = CGameInstance::Get().
			GetGameObjectByHandleT<CPropBarrel>(*m_hThrowBarrel))
		{
			pBarrel->CancelAncientThrowControl();
		}
	}

	if (auto* pPlayer = GetPlayer(pStateMachine))
	{
		if (auto* pAnimator = pPlayer->GetAnimator())
			pAnimator->GetCurAnimState().fSpeed = 1.f;
		if (auto* pMoveIntent = pPlayer->GetMoveIntent())
			pMoveIntent->ClearFacingIntent();
		ResetSkillControl(*pPlayer);
	}
	m_bOnceLighting = false;
	m_bOnceLastLighting = false;
	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
	m_fAcientElapsed = 0.f;
	m_hThrowBarrel.reset();
	m_hThrowDestination.reset();
	m_bThrowReleased = false;
	m_bThrowSlowMotionActive = false;
	m_hThrowFovCamera.reset();
	m_fThrowSequenceElapsed = 0.f;
	m_fThrowPostLaunchUnscaledElapsed = 0.f;
}

SPtr<CPlayer_AcientAttack_State> CPlayer_AcientAttack_State::Create()
{
	return ToSPtr(new CPlayer_AcientAttack_State{});
}
