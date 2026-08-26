#include "pch.h"
#include "AccioActivity_Npc.h"

#include "AccioActivity_Base.h"
#include "AccioActivity_Platform.h"
#include "AccioBall.h"
#include "ComPxRigidBody.h"
#include "DbgLineRender.h"
#include "GameInstance.h"

NS_USING(Client)

CAccioActivity_Npc::CAccioActivity_Npc() = default;

CAccioActivity_Npc::CAccioActivity_Npc(const CAccioActivity_Npc& prototype)
	: CGameObject{ prototype }
{
}

HRESULT CAccioActivity_Npc::InitializePrototype(void*)
{
	return S_OK;
}

HRESULT CAccioActivity_Npc::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc || FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	m_hActivity = pDesc->hActivity;
	m_hPlatform = pDesc->hPlatform;
	m_fTurnStartDelay = pDesc->fTurnStartDelay;
	m_fMoveSpeed = pDesc->fMoveSpeed;
	m_fMoveAcceleration = pDesc->fMoveAcceleration;
	m_fMoveDeceleration = pDesc->fMoveDeceleration;
	m_fMoveArrivalDistance = pDesc->fMoveArrivalDistance;
	m_fMoveAreaMargin = pDesc->fMoveAreaMargin;
	m_fAimDelay = pDesc->fAimDelay;
	m_fScorePullDuration = pDesc->fScorePullDuration;
	m_fMaximumAttackEdgeDistance = pDesc->fMaximumAttackEdgeDistance;
	m_fEstimatedAttackPullSpeed = pDesc->fEstimatedAttackPullSpeed;
	m_fAttackEdgeHoldSecondsPerUnit = pDesc->fAttackEdgeHoldSecondsPerUnit;
	m_fMinimumAttackPullDuration = pDesc->fMinimumAttackPullDuration;
	m_fMaximumAttackPullDuration = pDesc->fMaximumAttackPullDuration;
	m_fPullTimeout = pDesc->fPullTimeout;
	m_iMinimumAttackTargetScore = pDesc->iMinimumAttackTargetScore;
	SanitizeTuning();
	m_fPlannedPullDuration = m_fScorePullDuration;
	m_bDebugDraw = pDesc->bDebugDraw;

	GetTransform().SetPosition(pDesc->vInitialPosition);
	GetTransform().SetRotationEuler(pDesc->vInitialRotation);
	GetTransform().Update();
	return S_OK;
}

void CAccioActivity_Npc::OnRegisteredToManager()
{
	if (auto* pActivity = CGameInstance::Get().
		GetGameObjectByHandleT<CAccioActivity_Base>(m_hActivity))
	{
		// [LSY] Initialize 중에는 아직 자기 Handle이 유효하지 않으므로 등록 완료 후 참가시킨다.
		pActivity->SetParticipantHandle(
			CAccioActivity_Base::PARTICIPANT::NPC,
			GetHandle());
	}
}

void CAccioActivity_Npc::FixedUpdate(_float fTimeDelta)
{
	auto* pActivity = CGameInstance::Get().
		GetGameObjectByHandleT<CAccioActivity_Base>(m_hActivity);
	if (!pActivity || pActivity->GetPendingDestroy())
	{
		ResetTurnState();
		return;
	}

	const auto eMatchState = pActivity->GetMatchState();
	const _float fSafeDelta = std::max(fTimeDelta, 0.f);
	if (eMatchState == CAccioActivity_Base::MATCH_STATE::NPC_TURN)
	{
		UpdateNpcTurn(*pActivity, fSafeDelta);
		return;
	}

	if (m_eState == STATE::RETURNING)
	{
		UpdateReturnToRest(
			fSafeDelta,
			eMatchState ==
			CAccioActivity_Base::MATCH_STATE::WAIT_NPC_BALL_SETTLED);
		return;
	}

	if (eMatchState == CAccioActivity_Base::MATCH_STATE::WAIT_NPC_BALL_SETTLED)
	{
		if (m_eState != STATE::WAIT_BALL_SETTLED && !BeginReturnToRest())
			m_eState = STATE::WAIT_BALL_SETTLED;
		return;
	}

	ResetTurnState();
}

void CAccioActivity_Npc::UpdateNpcTurn(
	CAccioActivity_Base& activity,
	_float fTimeDelta)
{
	switch (m_eState)
	{
	case STATE::IDLE:
	{
		m_fStateElapsed += fTimeDelta;
		if (m_fStateElapsed < m_fTurnStartDelay)
			return;

		const auto hBall = activity.FindControllableBall(GetHandle());
		if (!hBall)
		{
			// [LSY] 리소스 누락이나 공 파괴로 선택지가 없어도 NPC 턴을 고착시키지 않는다.
			AbortNpcTurn(activity);
			return;
		}

		m_hActiveBall = *hBall;
		m_hDisruptionTargetBall = CHandle{};

		m_fStateElapsed = 0.f;
		const CAccioBall* pBall = GetSelectedBall();
		if (pBall && PrepareMoveTarget(activity, *pBall))
		{
			m_eState = STATE::MOVING;
		}
		else
		{
			// [LSY] 이동 영역이나 공 정보를 읽지 못한 상태로 제어를 시작하지 않는다.
			AbortNpcTurn(activity);
		}
		break;
	}

	case STATE::MOVING:
		m_fStateElapsed += fTimeDelta;
		if (!GetSelectedBall())
		{
			AbortNpcTurn(activity);
			break;
		}
		if (MoveToPreparedTarget(fTimeDelta))
		{
			m_fStateElapsed = 0.f;
			m_eState = STATE::AIMING;
		}
		break;

	case STATE::AIMING:
		if (const CAccioBall* pBall = GetSelectedBall())
			FaceTowards(pBall->GetTransform().GetPosition());
		else
		{
			AbortNpcTurn(activity);
			break;
		}

		m_fStateElapsed += fTimeDelta;
		if (m_fStateElapsed >= m_fAimDelay)
		{
			const CAccioBall* pBall = GetSelectedBall();
			if (!pBall || !PrepareMoveTarget(activity, *pBall))
			{
				AbortNpcTurn(activity);
				break;
			}

			// [LSY] 이동·조준 중 공 배치가 바뀌었으면 최신 계획 위치로 다시 이동한다.
			if (!IsAtPreparedMoveTarget())
			{
				m_fStateElapsed = 0.f;
				m_eState = STATE::MOVING;
				break;
			}

			if (AcquireSelectedBall())
			{
				m_fStateElapsed = 0.f;
				m_eState = STATE::PULLING;
			}
			else
			{
				AbortNpcTurn(activity);
			}
		}
		break;

	case STATE::PULLING:
	{
		m_fStateElapsed += fTimeDelta;
		CAccioBall* pBall = GetSelectedBall();
		if (!pBall || !pBall->IsControlledBy(GetHandle()))
		{
			AbortNpcTurn(activity);
			break;
		}

		// [LSY] 턴 시작 시 전략에 맞춰 계산한 유지시간만큼만 제어한다.
		// 득점은 고정시간, 공격은 충돌점과 보드 끝 거리로 계산된 시간이다.
		const _bool bReachedPlannedDuration =
			m_fStateElapsed >= m_fPlannedPullDuration;
		const _bool bTimedOut = m_fStateElapsed >= m_fPullTimeout;
		if (bReachedPlannedDuration || bTimedOut)
		{
			if (ReleaseSelectedBall())
			{
				m_fStateElapsed = 0.f;
				if (!BeginReturnToRest())
					m_eState = STATE::WAIT_BALL_SETTLED;
			}
			else
			{
				AbortNpcTurn(activity);
			}
		}
		break;
	}

	case STATE::WAIT_BALL_SETTLED:
		break;

	case STATE::RETURNING:
		UpdateReturnToRest(fTimeDelta, false);
		break;
	}
}

void CAccioActivity_Npc::AbortNpcTurn(CAccioActivity_Base& activity)
{
	// [LSY] Release가 성공했다면 Base가 이미 WAIT 상태로 전환됐으므로 Skip하지 않는다.
	ResetTurnState();
	if (activity.GetMatchState() ==
		CAccioActivity_Base::MATCH_STATE::NPC_TURN)
	{
		activity.SkipNpcTurn(GetHandle());
	}
	BeginReturnToRest();
}

_bool CAccioActivity_Npc::BeginReturnToRest()
{
	const auto* pPlatform = CGameInstance::Get().
		GetGameObjectByHandleT<CAccioActivity_Platform>(m_hPlatform);
	if (!pPlatform || pPlatform->GetPendingDestroy())
		return false;

	BoundingOrientedBox moveArea{};
	if (!pPlatform->GetNpcMoveAreaWorldOBB(moveArea))
		return false;

	const _matrix moveAreaWorld =
		XMMatrixRotationQuaternion(XMLoadFloat4(&moveArea.Orientation)) *
		XMMatrixTranslation(
			moveArea.Center.x,
			moveArea.Center.y,
			moveArea.Center.z);
	// [LSY] 이동영역 로컬 -Z가 경기장을 바라보는 NPC의 뒤쪽이다.
	const _float fRestLocalZ = -std::max(
		moveArea.Extents.z - m_fMoveAreaMargin,
		0.f);

	XMStoreFloat3(
		&m_vMoveTarget,
		XMVector3TransformCoord(
			XMVectorSet(0.f, 0.f, fRestLocalZ, 1.f),
			moveAreaWorld));
	m_vMoveTarget.y = GetTransform().GetPosition().y;
	XMStoreFloat3(
		&m_vRestFacingTarget,
		XMVector3TransformCoord(
			XMVectorSet(0.f, 0.f, 0.f, 1.f),
			moveAreaWorld));

	m_fStateElapsed = 0.f;
	m_fCurrentMoveSpeed = 0.f;
	m_eState = STATE::RETURNING;
	return true;
}

void CAccioActivity_Npc::UpdateReturnToRest(
	_float fTimeDelta,
	_bool bWaitForBall)
{
	if (!MoveToPreparedTarget(fTimeDelta))
		return;

	FaceTowards(m_vRestFacingTarget);
	m_fStateElapsed = 0.f;
	m_eState = bWaitForBall ? STATE::WAIT_BALL_SETTLED : STATE::IDLE;
}

void CAccioActivity_Npc::ResetTurnState()
{
	if (CAccioBall* pBall = GetSelectedBall();
		pBall && pBall->IsControlledBy(GetHandle()))
	{
		pBall->ReleaseControl(GetHandle());
	}

	m_hActiveBall = CHandle{};
	m_hDisruptionTargetBall = CHandle{};
	m_vMoveTarget = GetTransform().GetPosition();
	m_fStateElapsed = 0.f;
	m_fCurrentMoveSpeed = 0.f;
	m_fPlannedPullDuration = m_fScorePullDuration;
	m_fPlannedCollisionDistance = 0.f;
	m_fPlannedTargetEdgeDistance = 0.f;
	m_eTactic = TACTIC::SCORE;
	m_eState = STATE::IDLE;
}

void CAccioActivity_Npc::SanitizeTuning()
{
	m_fTurnStartDelay = std::max(m_fTurnStartDelay, 0.f);
	m_fMoveSpeed = std::max(m_fMoveSpeed, 0.1f);
	m_fMoveAcceleration = std::max(m_fMoveAcceleration, 0.1f);
	m_fMoveDeceleration = std::max(m_fMoveDeceleration, 0.1f);
	m_fMoveArrivalDistance = std::max(m_fMoveArrivalDistance, 0.01f);
	m_fMoveAreaMargin = std::max(m_fMoveAreaMargin, 0.f);
	m_fAimDelay = std::max(m_fAimDelay, 0.f);
	m_fScorePullDuration = std::max(m_fScorePullDuration, 0.1f);
	m_fMaximumAttackEdgeDistance = std::max(
		m_fMaximumAttackEdgeDistance, 0.f);
	m_fEstimatedAttackPullSpeed = std::max(
		m_fEstimatedAttackPullSpeed, 0.1f);
	m_fAttackEdgeHoldSecondsPerUnit = std::max(
		m_fAttackEdgeHoldSecondsPerUnit, 0.f);
	m_fMinimumAttackPullDuration = std::max(
		m_fMinimumAttackPullDuration, 0.1f);
	m_fMaximumAttackPullDuration = std::max(
		m_fMaximumAttackPullDuration,
		m_fMinimumAttackPullDuration);
	m_fPullTimeout = std::max(m_fPullTimeout, 0.1f);
	m_iMinimumAttackTargetScore = std::clamp(
		m_iMinimumAttackTargetScore, 30, 50);
}

_bool CAccioActivity_Npc::PrepareMoveTarget(
	const CAccioActivity_Base& activity,
	const CAccioBall& ball)
{
	const auto* pPlatform = CGameInstance::Get().
		GetGameObjectByHandleT<CAccioActivity_Platform>(m_hPlatform);
	if (!pPlatform)
		return false;

	BoundingOrientedBox moveArea{};
	if (!pPlatform->GetNpcMoveAreaWorldOBB(moveArea))
		return false;

	const _matrix moveAreaWorld =
		XMMatrixRotationQuaternion(XMLoadFloat4(&moveArea.Orientation)) *
		XMMatrixTranslation(
			moveArea.Center.x,
			moveArea.Center.y,
			moveArea.Center.z);
	_vector determinant{};
	const _matrix inverseMoveArea = XMMatrixInverse(
		&determinant,
		moveAreaWorld);
	if (fabsf(XMVectorGetX(determinant)) <= FLT_EPSILON)
		return false;

	_float3 vBallPosition = ball.GetTransform().GetPosition();
	if (ball.GetRigidBody())
		vBallPosition = ball.GetRigidBody()->GetPosition();

	_float3 localBall{};
	XMStoreFloat3(
		&localBall,
		XMVector3TransformCoord(
			XMLoadFloat3(&vBallPosition),
			inverseMoveArea));

	_float3 localNpc{};
	XMStoreFloat3(
		&localNpc,
		XMVector3TransformCoord(
			GetTransform().GetLoadedPostion(),
			inverseMoveArea));

	const _float fUsableHalfX = std::max(
		moveArea.Extents.x - m_fMoveAreaMargin, 0.f);

	// [LSY] 득점 전략은 공과 NPC의 X를 맞춘 직선 최단 경로를 사용한다.
	localNpc.x = localBall.x;
	m_hDisruptionTargetBall = CHandle{};
	m_eTactic = TACTIC::SCORE;
	m_fPlannedPullDuration = m_fScorePullDuration;
	m_fPlannedCollisionDistance = 0.f;
	m_fPlannedTargetEdgeDistance = 0.f;
	const auto attackPlan = BuildAttackPlan(
		activity,
		ball,
		inverseMoveArea,
		fUsableHalfX);
	if (attackPlan)
	{
		// [LSY] 후보 검사 중에는 상태를 건드리지 않고, 완성된 계획만 한 번에 반영한다.
		localNpc.x = attackPlan->fLocalNpcX;
		m_hDisruptionTargetBall = attackPlan->hTargetBall;
		m_eTactic = TACTIC::ATTACK;
		m_fPlannedCollisionDistance = attackPlan->fCollisionDistance;
		m_fPlannedTargetEdgeDistance = attackPlan->fTargetDistanceToEdge;
		m_fPlannedPullDuration = attackPlan->fPullDuration;
	}

	localNpc.x = std::clamp(localNpc.x, -fUsableHalfX, fUsableHalfX);
	localNpc.z = 0.f;
	m_fCurrentMoveSpeed = 0.f;

	XMStoreFloat3(
		&m_vMoveTarget,
		XMVector3TransformCoord(
			XMLoadFloat3(&localNpc),
			moveAreaWorld));
	return true;
}

std::optional<CAccioActivity_Npc::ATTACK_PLAN>
CAccioActivity_Npc::BuildAttackPlan(
	const CAccioActivity_Base& activity,
	const CAccioBall& ball,
	const _matrix& inverseMoveArea,
	_float fUsableHalfX) const
{
	_float3 vBallPosition = ball.GetTransform().GetPosition();
	if (ball.GetRigidBody())
		vBallPosition = ball.GetRigidBody()->GetPosition();

	_float3 localBall{};
	XMStoreFloat3(
		&localBall,
		XMVector3TransformCoord(
			XMLoadFloat3(&vBallPosition),
			inverseMoveArea));

	// [LSY] 50점, 30점 순으로만 검사한다. 10점과 20점 공은 공격 가치가
	// 낮으므로 후보에서 제외하고 NPC 자신의 득점을 우선한다.
	const auto targetHandles = activity.FindScoringBalls(
		CAccioActivity_Base::PARTICIPANT::PLAYER,
		m_iMinimumAttackTargetScore);
	for (const CHandle& hTarget : targetHandles)
	{
		const auto* pTargetBall = CGameInstance::Get().
			GetGameObjectByHandleT<CAccioBall>(hTarget);
		if (!pTargetBall || pTargetBall->GetPendingDestroy())
			continue;

		_float3 vTargetPosition = pTargetBall->GetTransform().GetPosition();
		if (pTargetBall->GetRigidBody())
			vTargetPosition = pTargetBall->GetRigidBody()->GetPosition();

		_float3 localTarget{};
		XMStoreFloat3(
			&localTarget,
			XMVector3TransformCoord(
				XMLoadFloat3(&vTargetPosition),
				inverseMoveArea));

		const _float fDeltaZ = localTarget.z - localBall.z;
		if (fabsf(fDeltaZ) <= FLT_EPSILON)
			continue;

		// 선택 공 -> 플레이어 공 직선을 NPC 이동선까지 연장했을 때의 위치다.
		const _float fIntersectionRatio = -localBall.z / fDeltaZ;
		if (fIntersectionRatio <= 1.f)
			continue;

		const _float fAttackPositionX = localBall.x +
			(localTarget.x - localBall.x) * fIntersectionRatio;
		if (fabsf(fAttackPositionX) > fUsableHalfX)
			continue;

		const _float3 vPushDirection{
			vTargetPosition.x - vBallPosition.x,
			0.f,
			vTargetPosition.z - vBallPosition.z
		};
		const auto activeBallDistanceToEdge =
			activity.GetDistanceToPlayAreaEdge(ball, vPushDirection);
		if (!activeBallDistanceToEdge)
			continue;

		const auto firstPlayerHit = activity.FindFirstBallOnPath(
			CAccioActivity_Base::PARTICIPANT::PLAYER,
			ball.GetHandle(),
			vBallPosition,
			vPushDirection,
			ball.GetSphereRadius(),
			*activeBallDistanceToEdge);
		if (!firstPlayerHit || firstPlayerHit->hBall != hTarget)
		{
			// 낮은 점수 공이라도 먼저 부딪히면 목표 공을 직접 공격할 수 없다.
			continue;
		}

		const auto npcPathBlocker = activity.FindFirstBallOnPath(
			CAccioActivity_Base::PARTICIPANT::NPC,
			ball.GetHandle(),
			vBallPosition,
			vPushDirection,
			ball.GetSphereRadius(),
			*activeBallDistanceToEdge);
		if (npcPathBlocker)
		{
			// 이미 득점한 NPC 공이 제거 경로에 있으면 자기 공을 보호한다.
			continue;
		}

		const auto distanceToEdge = activity.GetDistanceToPlayAreaEdge(
			*pTargetBall,
			vPushDirection);
		if (!distanceToEdge ||
			*distanceToEdge > m_fMaximumAttackEdgeDistance)
		{
			continue;
		}

		const _float fCollisionDistance =
			firstPlayerHit->fDistanceUntilCollision;
		return ATTACK_PLAN{
			.hTargetBall = hTarget,
			.fLocalNpcX = fAttackPositionX,
			.fCollisionDistance = fCollisionDistance,
			.fTargetDistanceToEdge = *distanceToEdge,
			.fPullDuration = CalculateAttackPullDuration(
				fCollisionDistance,
				*distanceToEdge)
		};
	}

	return std::nullopt;
}

_float CAccioActivity_Npc::CalculateAttackPullDuration(
	_float fCollisionDistance,
	_float fTargetDistanceToEdge) const
{
	// [LSY] 충돌점까지 도달할 예상시간에, 목표 공이 보드 끝까지 가야 하는
	// 거리만큼 유지시간을 더한다. 먼 공일수록 더 강한 충돌을 준비한다.
	const _float fCollisionTravelTime = std::max(
		fCollisionDistance,
		0.f) / m_fEstimatedAttackPullSpeed;
	const _float fEdgeHoldTime = std::max(
		fTargetDistanceToEdge,
		0.f) * m_fAttackEdgeHoldSecondsPerUnit;
	return std::clamp(
		fCollisionTravelTime + fEdgeHoldTime,
		m_fMinimumAttackPullDuration,
		m_fMaximumAttackPullDuration);
}

_bool CAccioActivity_Npc::IsAtPreparedMoveTarget() const
{
	const _float3 vPosition = GetTransform().GetPosition();
	const _float fDeltaX = m_vMoveTarget.x - vPosition.x;
	const _float fDeltaZ = m_vMoveTarget.z - vPosition.z;
	return fDeltaX * fDeltaX + fDeltaZ * fDeltaZ <=
		m_fMoveArrivalDistance * m_fMoveArrivalDistance;
}

_bool CAccioActivity_Npc::MoveToPreparedTarget(_float fTimeDelta)
{
	const _float3 vPosition = GetTransform().GetPosition();
	const _vector toTarget = XMVectorSet(
		m_vMoveTarget.x - vPosition.x,
		0.f,
		m_vMoveTarget.z - vPosition.z,
		0.f);
	const _float fDistance = XMVectorGetX(XMVector3Length(toTarget));
	if (fDistance <= m_fMoveArrivalDistance)
	{
		GetTransform().SetPosition(_float3{
			m_vMoveTarget.x,
			vPosition.y,
			m_vMoveTarget.z
		});
		m_fCurrentMoveSpeed = 0.f;
		return true;
	}

	const _vector direction = toTarget / std::max(fDistance, FLT_EPSILON);
	const _float fBrakingDistance = std::max(
		fDistance - m_fMoveArrivalDistance, 0.f);
	const _float fBrakingSpeed = sqrtf(
		2.f * m_fMoveDeceleration * fBrakingDistance);
	const _float fDesiredSpeed = std::min(m_fMoveSpeed, fBrakingSpeed);
	const _float fSafeDelta = std::max(fTimeDelta, 0.f);
	if (m_fCurrentMoveSpeed < fDesiredSpeed)
	{
		m_fCurrentMoveSpeed = std::min(
			m_fCurrentMoveSpeed + m_fMoveAcceleration * fSafeDelta,
			fDesiredSpeed);
	}
	else
	{
		m_fCurrentMoveSpeed = std::max(
			m_fCurrentMoveSpeed - m_fMoveDeceleration * fSafeDelta,
			fDesiredSpeed);
	}

	const _float fStep = std::min(
		m_fCurrentMoveSpeed * fSafeDelta,
		fDistance);
	GetTransform().SetPosition(
		GetTransform().GetLoadedPostion() + direction * fStep);
	FaceTowards(m_vMoveTarget);
	return false;
}

void CAccioActivity_Npc::FaceTowards(const _float3& vWorldPosition)
{
	const _float3 vPosition = GetTransform().GetPosition();
	const _float fDirectionX = vWorldPosition.x - vPosition.x;
	const _float fDirectionZ = vWorldPosition.z - vPosition.z;
	if (fDirectionX * fDirectionX + fDirectionZ * fDirectionZ <= FLT_EPSILON)
		return;

	const _float fYaw = XMConvertToDegrees(atan2f(
		fDirectionX,
		fDirectionZ));
	GetTransform().SetRotationEuler({ 0.f, fYaw, 0.f });
}

_bool CAccioActivity_Npc::AcquireSelectedBall()
{
	CAccioBall* pBall = GetSelectedBall();
	return pBall && pBall->TryAcquireControl(GetHandle());
}

_bool CAccioActivity_Npc::ReleaseSelectedBall()
{
	CAccioBall* pBall = GetSelectedBall();
	return pBall && pBall->ReleaseControl(GetHandle());
}

CAccioBall* CAccioActivity_Npc::GetSelectedBall() const
{
	return CGameInstance::Get().GetGameObjectByHandleT<CAccioBall>(m_hActiveBall);
}

CAccioBall* CAccioActivity_Npc::GetDisruptionTargetBall() const
{
	return CGameInstance::Get().
		GetGameObjectByHandleT<CAccioBall>(m_hDisruptionTargetBall);
}

void CAccioActivity_Npc::LateUpdate(_float)
{
	GetTransform().Update();
	if (m_bDebugDraw)
		DrawDebugState();
}

void CAccioActivity_Npc::UpdateGUI()
{
	CGameObject::UpdateGUI();

	ImGui::Text("State: %s", GetStateName(m_eState));
	ImGui::Text("State Time: %.2f", m_fStateElapsed);
	if (m_eTactic == TACTIC::ATTACK)
		ImGui::Text("Tactic: Attack");
	else
		ImGui::Text("Tactic: Score");
	ImGui::Text("Planned Pull Time: %.2f", m_fPlannedPullDuration);
	const _float fEffectivePullTime = std::min(
		m_fPlannedPullDuration,
		m_fPullTimeout);
	ImGui::Text("Effective Pull Time: %.2f", fEffectivePullTime);
	if (m_fPullTimeout < m_fPlannedPullDuration)
	{
		ImGui::TextColored(
			{ 1.f, 0.75f, 0.2f, 1.f },
			"Safety timeout shortens this plan");
	}
	if (m_eTactic == TACTIC::ATTACK)
	{
		ImGui::Text(
			"Collision Distance: %.2f",
			m_fPlannedCollisionDistance);
		ImGui::Text(
			"Target To Edge: %.2f",
			m_fPlannedTargetEdgeDistance);
	}
	ImGui::Checkbox("Debug Draw", &m_bDebugDraw);
	if (GetDisruptionTargetBall())
		ImGui::Text("Disruption Target: Player Ball");
	else
		ImGui::Text("Disruption Target: None");
	ImGui::DragFloat(
		"Turn Start Delay", &m_fTurnStartDelay,
		0.05f, 0.f, 5.f, "%.2f");
	ImGui::DragFloat("Move Speed", &m_fMoveSpeed, 0.1f, 0.1f, 20.f, "%.1f");
	ImGui::DragFloat(
		"Move Acceleration", &m_fMoveAcceleration,
		0.1f, 0.1f, 30.f, "%.1f");
	ImGui::DragFloat(
		"Move Deceleration", &m_fMoveDeceleration,
		0.1f, 0.1f, 40.f, "%.1f");
	ImGui::DragFloat(
		"Move Arrival Distance", &m_fMoveArrivalDistance,
		0.01f, 0.01f, 2.f, "%.2f");
	ImGui::DragFloat(
		"Move Area Margin", &m_fMoveAreaMargin, 0.05f, 0.f, 5.f, "%.2f");
	ImGui::DragFloat("Aim Delay", &m_fAimDelay, 0.05f, 0.f, 5.f, "%.2f");
	ImGui::DragFloat(
		"Score Pull Duration", &m_fScorePullDuration,
		0.05f, 0.1f, 3.f, "%.2f");
	ImGui::DragFloat(
		"Maximum Attack Edge Distance", &m_fMaximumAttackEdgeDistance,
		0.1f, 0.f, 30.f, "%.1f");
	ImGui::DragFloat(
		"Estimated Attack Pull Speed", &m_fEstimatedAttackPullSpeed,
		0.1f, 0.1f, 40.f, "%.1f");
	ImGui::DragFloat(
		"Attack Edge Hold Seconds Per Unit",
		&m_fAttackEdgeHoldSecondsPerUnit,
		0.005f, 0.f, 0.2f, "%.3f");
	ImGui::DragFloat(
		"Minimum Attack Pull Duration",
		&m_fMinimumAttackPullDuration,
		0.05f, 0.1f, m_fMaximumAttackPullDuration, "%.2f");
	m_fMaximumAttackPullDuration = std::max(
		m_fMaximumAttackPullDuration,
		m_fMinimumAttackPullDuration);
	ImGui::DragFloat(
		"Maximum Attack Pull Duration",
		&m_fMaximumAttackPullDuration,
		0.05f, m_fMinimumAttackPullDuration, 5.f, "%.2f");
	ImGui::DragFloat(
		"Pull Safety Timeout", &m_fPullTimeout, 0.1f, 0.1f, 20.f, "%.1f");
	ImGui::SliderInt(
		"Minimum Attack Target Score",
		&m_iMinimumAttackTargetScore,
		30,
		50);
	SanitizeTuning();
}

void CAccioActivity_Npc::DrawDebugState() const
{
	auto* pDebug = CGameInstance::Get().GetDbgLineRender();
	if (!pDebug)
		return;

	const _float4 vPreviousColor = pDebug->GetColor();
	const auto ePreviousDepthMode = pDebug->GetDepthMode();
	_float4 vStateColor{ 1.f, 0.2f, 0.2f, 1.f };
	if (m_eState == STATE::AIMING)
		vStateColor = { 1.f, 0.85f, 0.15f, 1.f };
	else if (m_eState == STATE::MOVING)
		vStateColor = { 0.25f, 1.f, 0.25f, 1.f };
	else if (m_eState == STATE::PULLING)
		vStateColor = { 1.f, 0.1f, 0.8f, 1.f };
	else if (m_eState == STATE::RETURNING)
		vStateColor = { 0.2f, 1.f, 0.75f, 1.f };
	else if (m_eState == STATE::WAIT_BALL_SETTLED)
		vStateColor = { 0.3f, 0.8f, 1.f, 1.f };

	pDebug->SetDepthTest(false);
	pDebug->SetColor(vStateColor);
	const _float3 vPosition = GetTransform().GetPosition();
	pDebug->AddSphere(0.75f, XMMatrixTranslation(
		vPosition.x, vPosition.y + 1.f, vPosition.z));
	pDebug->AddCross(vPosition, 0.5f);
	if (m_eState == STATE::MOVING || m_eState == STATE::RETURNING)
	{
		pDebug->AddCross(m_vMoveTarget, 0.65f);
		pDebug->AddLine(vPosition, m_vMoveTarget);
	}

	if (const CAccioBall* pBall = GetSelectedBall())
	{
		const _float3 vBallPosition = pBall->GetTransform().GetPosition();
		pDebug->AddLine(vPosition, vBallPosition);
	}

	if (const CAccioBall* pTargetBall = GetDisruptionTargetBall())
	{
		_float3 vTargetPosition = pTargetBall->GetTransform().GetPosition();
		if (pTargetBall->GetRigidBody())
			vTargetPosition = pTargetBall->GetRigidBody()->GetPosition();
		pDebug->SetColor({ 0.2f, 0.85f, 1.f, 1.f });
		pDebug->AddCross(vTargetPosition, 0.9f);
		pDebug->AddLine(m_vMoveTarget, vTargetPosition);
	}

	pDebug->SetColor(vPreviousColor);
	pDebug->SetDepthMode(ePreviousDepthMode);
}

const _char* CAccioActivity_Npc::GetStateName(STATE eState)
{
	switch (eState)
	{
	case STATE::IDLE:
		return "Idle";
	case STATE::MOVING:
		return "Moving";
	case STATE::AIMING:
		return "Aiming";
	case STATE::PULLING:
		return "Pulling";
	case STATE::RETURNING:
		return "Returning";
	case STATE::WAIT_BALL_SETTLED:
		return "Wait Ball Settled";
	default:
		return "Unknown";
	}
}

UPtr<CAccioActivity_Npc> CAccioActivity_Npc::Create()
{
	auto pInstance = ToUPtr(new CAccioActivity_Npc{});
	if (FAILED(pInstance->InitializePrototype()))
		return nullptr;
	return pInstance;
}

UPtr<CPrototype> CAccioActivity_Npc::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CAccioActivity_Npc{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
		return nullptr;
	return pInstance;
}
