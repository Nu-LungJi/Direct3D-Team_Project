#include "pch.h"
#include "Player_Locomotion_State.h"
#include "Player.h"
#include "ComAnimator.h"
#include "ComCharacterMoveIntent.h"

NS_USING(Client)

CPlayer_Locomotion_State::CPlayer_Locomotion_State()
{
	for (auto& directions : m_FreeAnimations)
		directions.fill(INVALID_ANIMATION);
}

void CPlayer_Locomotion_State::Enter(CStateMachine* pStateMachine)
{

	if (!m_bAnimationTableInitialized)
		InitializeAnimationTable(*CGameInstance::Get().GetGameObjectByHandleT<CPlayer>(pStateMachine->GetOwnerHandle()));
}

void CPlayer_Locomotion_State::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto* player = CGameInstance::Get().GetGameObjectByHandleT<CPlayer>(pStateMachine->GetOwnerHandle()) ;
	if (!player)
		return;

	auto* animator = player->GetAnimator();
	auto* moveIntent = player->GetMoveIntent();
	if (!animator || !moveIntent)
		return;

	const auto& move = moveIntent->GetOutput();

	int32_t animIndex = m_iFreeIdleAnimation;

	if (move.bMoveRequested)
	{
		// 우선은 전진 Jog 하나로 확인
		// PriorityUpdate에서 move.vMoveDirection을 카메라 축으로 만들었으므로,
		// 여기서도 같은 축을 사용해야 A/D가 LEFT_90/RIGHT_90으로 일관되게 매핑된다.
		auto* pCamera = CGameInstance::Get().GetActiveCamera("CREATURE_ANIM_PLAYER_CAMERA");
		_vector vLook = pCamera
			? pCamera->GetTransform().GetState(STATE::LOOK)
			: player->GetTransform().GetState(STATE::LOOK);
		_vector vRight = pCamera
			? pCamera->GetTransform().GetState(STATE::RIGHT)
			: player->GetTransform().GetState(STATE::RIGHT);
		_vector vMove = XMLoadFloat3(&move.vMoveDirection);

		vLook = XMVector3Normalize(XMVectorSetY(vLook, 0.f));
		vRight = XMVector3Normalize(XMVectorSetY(vRight, 0.f));
		vMove = XMVector3Normalize(XMVectorSetY(vMove, 0.f));

		const _float fForward = XMVectorGetX(XMVector3Dot(vMove, vLook));
		const _float fRight = XMVectorGetX(XMVector3Dot(vMove, vRight));
		const _float fAngle = XMConvertToDegrees(atan2f(fRight, fForward));

		MOVE_DIRECTION eDirection{};
		if (fAngle >= -22.5f && fAngle < 22.5f)
			eDirection = MOVE_DIRECTION::FRONT;
		else if (fAngle < 67.5f)
			eDirection = MOVE_DIRECTION::RIGHT_45;
		else if (fAngle < 112.5f)
			eDirection = MOVE_DIRECTION::RIGHT_90;
		else if (fAngle < 157.5f)
			eDirection = MOVE_DIRECTION::RIGHT_135;
		else if (fAngle >= 157.5f || fAngle < -157.5f)
			eDirection = MOVE_DIRECTION::BACKWARD;
		else if (fAngle < -112.5f)
			eDirection = MOVE_DIRECTION::LEFT_135;
		else if (fAngle < -67.5f)
			eDirection = MOVE_DIRECTION::LEFT_90;
		else
			eDirection = MOVE_DIRECTION::LEFT_45;

		animIndex = GetFreeAnimation(MOVE_TYPE::JOG, eDirection);
	}

	if (animIndex != INVALID_ANIMATION)
		animator->Play_Anim(animIndex, true, 0.15f);
}

void CPlayer_Locomotion_State::SetFreeIdleAnimation(int32_t iAnimationIndex)
{
	m_iFreeIdleAnimation = iAnimationIndex;
}

void CPlayer_Locomotion_State::SetFreeAnimation(MOVE_TYPE eMoveType,MOVE_DIRECTION eDirection,int32_t iAnimationIndex)
{
	if (eMoveType == MOVE_TYPE::END || eDirection == MOVE_DIRECTION::END)
		return;

	m_FreeAnimations[ETOUI(eMoveType)][ETOUI(eDirection)] = iAnimationIndex;
}

int32_t CPlayer_Locomotion_State::GetFreeIdleAnimation() const
{
	return m_iFreeIdleAnimation;
}

int32_t CPlayer_Locomotion_State::GetFreeAnimation(MOVE_TYPE eMoveType, MOVE_DIRECTION eDirection) const
{
	if (eMoveType == MOVE_TYPE::END || eDirection == MOVE_DIRECTION::END)
		return INVALID_ANIMATION;

	return m_FreeAnimations[ETOUI(eMoveType)][ETOUI(eDirection)];
}

SPtr<CPlayer_Locomotion_State> CPlayer_Locomotion_State::Create()
{
	return ToSPtr(new CPlayer_Locomotion_State{});
}

void CPlayer_Locomotion_State::InitializeAnimationTable(CPlayer& player)
{
	SetFreeIdleAnimation(player.FindAnimationIndex("AN_IDLE0.bin"));

	SetFreeAnimationByName(player, MOVE_TYPE::WALK_SLOW, MOVE_DIRECTION::FRONT, "AN_Walk_Slow.bin");
	SetFreeAnimationByName(player, MOVE_TYPE::WALK_FAST, MOVE_DIRECTION::FRONT, "AN_Walk_Fast.bin");

	SetFreeAnimationByName(player, MOVE_TYPE::WALK, MOVE_DIRECTION::BACKWARD, "AN_Walk_Back.bin");
	SetFreeAnimationByName(player, MOVE_TYPE::WALK, MOVE_DIRECTION::RIGHT_45, "AN_Walk_Right_45.bin");
	SetFreeAnimationByName(player, MOVE_TYPE::WALK, MOVE_DIRECTION::RIGHT_90, "AN_Walk_Right_90.bin");
	SetFreeAnimationByName(player, MOVE_TYPE::WALK, MOVE_DIRECTION::RIGHT_135, "AN_Walk_Right_135.bin");
	SetFreeAnimationByName(player, MOVE_TYPE::WALK, MOVE_DIRECTION::LEFT_135, "AN_Walk_Left_135.bin");
	SetFreeAnimationByName(player, MOVE_TYPE::WALK, MOVE_DIRECTION::LEFT_90, "AN_Walk_Left_90.bin");
	SetFreeAnimationByName(player, MOVE_TYPE::WALK, MOVE_DIRECTION::LEFT_45, "AN_Walk_Left_45.bin");

	SetFreeAnimationByName(player, MOVE_TYPE::JOG, MOVE_DIRECTION::FRONT, "AN_Jog_Front.bin");
	SetFreeAnimationByName(player, MOVE_TYPE::JOG, MOVE_DIRECTION::RIGHT_45, "AN_Jog_Right_45.bin");
	SetFreeAnimationByName(player, MOVE_TYPE::JOG, MOVE_DIRECTION::RIGHT_90, "AN_Jog_Right_90.bin");
	SetFreeAnimationByName(player, MOVE_TYPE::JOG, MOVE_DIRECTION::RIGHT_135, "AN_Jog_Right_135.bin");
	SetFreeAnimationByName(player, MOVE_TYPE::JOG, MOVE_DIRECTION::BACKWARD, "AN_Jog_BackWard.bin");
	SetFreeAnimationByName(player, MOVE_TYPE::JOG, MOVE_DIRECTION::LEFT_135, "AN_Jog_Left_135.bin");
	SetFreeAnimationByName(player, MOVE_TYPE::JOG, MOVE_DIRECTION::LEFT_90, "AN_Jog_Left_90.bin");
	SetFreeAnimationByName(player, MOVE_TYPE::JOG, MOVE_DIRECTION::LEFT_45, "AN_Jog_Left_45.bin");

	SetFreeAnimationByName(player, MOVE_TYPE::SPRINT, MOVE_DIRECTION::FRONT, "AN_Sprint.bin");

	m_bAnimationTableInitialized = true;
}

void CPlayer_Locomotion_State::SetFreeAnimationByName(CPlayer& player,MOVE_TYPE eMoveType,MOVE_DIRECTION eDirection,_string_view sAnimationName)
{
	const int32_t iAnimationIndex = player.FindAnimationIndex(sAnimationName);
	if (iAnimationIndex != INVALID_ANIMATION)
		SetFreeAnimation(eMoveType, eDirection, iAnimationIndex);
}
