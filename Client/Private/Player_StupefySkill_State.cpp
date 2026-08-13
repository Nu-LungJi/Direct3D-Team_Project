#include "pch.h"
#include "Player_StupefySkill_State.h"
#include "Player.h"
#include "ComAnimator.h"
#include "PlayerAnimationRatioGuard.h"

NS_USING(Client)

void CPlayer_StupefySkill_State::Enter(CStateMachine* pStateMachine)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer || !pPlayer->GetAnimator()) { RequestLocomotion(pStateMachine); return; }
	_float3 vParryPosition{};
	if (!pPlayer->ConsumeParryCounter(vParryPosition)) { RequestLocomotion(pStateMachine); return; }
	CacheAnimationIndices(*pPlayer);
	SetSkillControl(*pPlayer, true, true, true);
	pPlayer->SetCurrentMoveSpeed(0.f);
	pPlayer->SetPlayerCurSKill(PLAYER_SKILL_TYPE::ATTACK);
	m_bSpeedRestored = false;
	m_bProjectileReleased = false;
	m_fPreviousAnimRatio = 0.f;
	if (!PlayCounterAnimation(*pPlayer, vParryPosition)) RequestLocomotion(pStateMachine);
}

void CPlayer_StupefySkill_State::Update(CStateMachine* pStateMachine, _float)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	auto* pAnimator = pPlayer ? pPlayer->GetAnimator() : nullptr;
	if (!pPlayer || !pAnimator) { RequestLocomotion(pStateMachine); return; }
	const _float fRatio = PlayerAnimationRatioGuard::Sanitize(pAnimator->GetPlayAnimRatio());
	pPlayer->SetCurrentMoveSpeed(0.f);
	if (!m_bSpeedRestored && fRatio >= TURN_END_RATIO)
	{
		pAnimator->GetCurAnimState().fSpeed = ATTACK_SPEED;
		m_bSpeedRestored = true;
	}
	if (!m_bProjectileReleased && m_fPreviousAnimRatio < PROJECTILE_RELEASE_RATIO && fRatio >= PROJECTILE_RELEASE_RATIO)
	{
		m_bProjectileReleased = true;
		// [Stupefy Effect] 전용 시전/발사 이펙트와 투사체 생성 위치.
		// 현재는 기존 피격 연결을 유지하기 위해 마법 탄환만 발사한다.
		pPlayer->Attack_Magic_Bullet();
	}
	m_fPreviousAnimRatio = fRatio;
	if (pAnimator->GetFinish()) RequestLocomotion(pStateMachine);
}

void CPlayer_StupefySkill_State::Exit(CStateMachine* pStateMachine)
{
	if (auto* pPlayer = GetPlayer(pStateMachine)) ResetSkillControl(*pPlayer);
	m_bSpeedRestored = false;
	m_bProjectileReleased = false;
	m_fPreviousAnimRatio = 0.f;
}

void CPlayer_StupefySkill_State::CacheAnimationIndices(const CPlayer& player)
{
	if (m_bAnimationsCached) return;
	m_Animations[0] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Parry_Counter_Atk_Fwd_2_Spin_Rht_anm.bin");
	m_Animations[1] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Parry_Counter_Atk_Lft_90_Spin_Lft_Slam_anm.bin");
	m_Animations[2] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Parry_Counter_Atk_Rht_90_Spin_Rht_Slam_anm.bin");
	m_Animations[3] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Parry_Counter_Atk_Lft_180_Spin_Rht_Send_anm.bin");
	m_Animations[4] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Parry_Counter_Atk_Rht_180_Spin_Rht_anm.bin");
	m_bAnimationsCached = true;
}

_bool CPlayer_StupefySkill_State::PlayCounterAnimation(CPlayer& player, const _float3& vParryPosition)
{
	auto* pAnimator = player.GetAnimator();
	if (!pAnimator) return false;
	_vector vLook = XMVectorSetY(player.GetTransform().GetState(STATE::LOOK), 0.f);
	_vector vRight = XMVectorSetY(player.GetTransform().GetState(STATE::RIGHT), 0.f);
	const _vector vPlayerPosition = player.GetTransform().GetState(STATE::POSITION);
	_vector vDirection{};
	if (auto* pTarget = CGameInstance::Get().GetGameObjectByHandle(player.GetTargetHandle()); pTarget && !pTarget->GetPendingDestroy())
		vDirection = XMVectorSetY(pTarget->GetTransform().GetState(STATE::POSITION) - vPlayerPosition, 0.f);
	else
		vDirection = XMVectorSetY(XMLoadFloat3(&vParryPosition) - vPlayerPosition, 0.f);
	if (XMVectorGetX(XMVector3LengthSq(vDirection)) <= FLT_EPSILON) vDirection = vLook;
	vLook = XMVector3Normalize(vLook);
	vRight = XMVector3Normalize(vRight);
	vDirection = XMVector3Normalize(vDirection);
	const _float fAngle = XMConvertToDegrees(std::atan2(XMVectorGetX(XMVector3Dot(vRight, vDirection)), XMVectorGetX(XMVector3Dot(vLook, vDirection))));
	const _float fAbsAngle = std::abs(fAngle);
	size_t iDirection = 0;
	if (fAbsAngle >= 135.f) iDirection = fAngle < 0.f ? 3 : 4;
	else if (fAbsAngle >= 45.f) iDirection = fAngle < 0.f ? 1 : 2;
	const int32_t iAnimation = m_Animations[iDirection];
	if (iAnimation < 0) return false;
	pAnimator->Play_Anim(iAnimation, false, BLEND_DURATION);
	pAnimator->GetCurAnimState().fSpeed = TURN_SPEED;
	return true;
}

SPtr<CPlayer_StupefySkill_State> CPlayer_StupefySkill_State::Create()
{
	return ToSPtr(new CPlayer_StupefySkill_State{});
}
