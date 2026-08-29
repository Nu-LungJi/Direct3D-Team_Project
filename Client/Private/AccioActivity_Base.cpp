#include "pch.h"
#include "AccioActivity_Base.h"

#include "AccioBall.h"
#include "ComPxBoxCollider.h"
#include "ComPxRigidBody.h"
#include "GameInstance.h"
#include "ResPhysXBoxGeometry.h"
#include "ResPhysXMaterial.h"
#include "UIManager.h"

NS_USING(Client)

CAccioActivity_Base::CAccioActivity_Base() = default;

CAccioActivity_Base::CAccioActivity_Base(const CAccioActivity_Base& prototype)
	: CAccioActivityPartBase{ prototype }
{
}

StringID CAccioActivity_Base::GetModelResourceTag() const
{
	return "Static_AccioActivity_Resource";
}

const _char* CAccioActivity_Base::GetMatchStateText() const
{
	return GetMatchStateName(m_eMatchState);
}

void CAccioActivity_Base::FixedUpdate(_float)
{
	if (m_eMatchState != MATCH_STATE::READY &&
		m_eMatchState != MATCH_STATE::MATCH_END)
	{
		const auto* pPlayer = CGameInstance::Get().
			GetGameObjectByHandle(m_hPlayer);
		const auto* pNpc = CGameInstance::Get().
			GetGameObjectByHandle(m_hNpc);
		if (!pPlayer || pPlayer->GetPendingDestroy() ||
			!pNpc || pNpc->GetPendingDestroy() ||
			!ValidateRegisteredBalls())
		{
			// [LSY] 참가자나 등록 공이 사라진 경기는 진행 상태에 고착되지 않게 즉시 종료한다.
			m_eMatchState = MATCH_STATE::MATCH_END;
			ReleaseActiveBallControl();
			m_bScoreUiSubmitted = false;
			GET_SINGLE(UIManager)->AssioMiniGameFinish();
		}
	}

	UpdateSettledScores();
	RefreshScores();
	UpdateTurnState();
}

void CAccioActivity_Base::UpdateGUI()
{
	CAccioActivityPartBase::UpdateGUI();

	if (!ImGui::CollapsingHeader(
		"Accio Activity Score",
		ImGuiTreeNodeFlags_DefaultOpen))
	{
		return;
	}

	ImGui::TextColored(
		ImVec4{ 0.25f, 0.55f, 1.f, 1.f },
		"Blue Score: %d",
		m_iBlueScore);
	ImGui::TextColored(
		ImVec4{ 1.f, 0.25f, 0.2f, 1.f },
		"Red Score: %d",
		m_iRedScore);
	ImGui::Separator();
	ImGui::Text("Scoring Balls: %zu", m_BallScoreStates.size());
	ImGui::TextDisabled(
		"Score is determined by the settled ball center.");
	ImGui::Separator();
	ImGui::Text("Match State: %s", GetMatchStateName(m_eMatchState));
	ImGui::Text(
		"Round: %u / %u",
		std::min(m_iCurrentRound + 1u, m_iMaxRounds),
		m_iMaxRounds);
	if (m_eMatchState == MATCH_STATE::READY)
	{
		ImGui::Checkbox("NPC Starts First", &m_bNpcStartsFirst);
		if (ImGui::Button("Start Accio Match") && !StartMatch())
			DEBUG_LOG("[AccioActivity] Participants or balls are invalid.\n");
	}
	else if (ImGui::Button("Reset Accio Match"))
	{
		if (!ResetMatch(true))
			DEBUG_LOG("[AccioActivity] Failed to reset one or more balls.\n");
	}
}

_bool CAccioActivity_Base::RegisterBall(const CHandle& hBall)
{
	auto* pBall = CGameInstance::Get().GetGameObjectByHandleT<CAccioBall>(hBall);
	if (!pBall)
		return false;

	if (std::ranges::find(m_BallHandles, hBall) == m_BallHandles.end())
		m_BallHandles.push_back(hBall);

	pBall->SetActivityHandle(GetHandle());
	return true;
}

std::optional<CHandle> CAccioActivity_Base::FindControllableBall(
	const CHandle& hController) const
{
	for (const CHandle& hBall : m_BallHandles)
	{
		if (CanControlBall(hController, hBall))
			return hBall;
	}

	return std::nullopt;
}

std::vector<CHandle> CAccioActivity_Base::FindScoringBalls(
	PARTICIPANT eParticipant,
	int32_t iMinimumScore,
	_bool bSettledOnly) const
{
	CAccioBall::COLOR eTargetColor = CAccioBall::COLOR::NONE;
	if (eParticipant == PARTICIPANT::PLAYER)
		eTargetColor = CAccioBall::COLOR::BLUE;
	else if (eParticipant == PARTICIPANT::NPC)
		eTargetColor = CAccioBall::COLOR::RED;
	else
		return {};

	struct SCORED_BALL
	{
		CHandle hBall{};
		int32_t iScore{};
	};

	std::vector<SCORED_BALL> scoredBalls{};
	for (const CHandle& hBall : m_BallHandles)
	{
		const auto* pBall = CGameInstance::Get().
			GetGameObjectByHandleT<CAccioBall>(hBall);
		if (!pBall || pBall->GetPendingDestroy() ||
			pBall->GetBallColor() != eTargetColor ||
			!m_UsedBalls.contains(hBall))
		{
			continue;
		}

		if (bSettledOnly && !pBall->IsSettled())
			continue;

		_float3 vPosition = pBall->GetTransform().GetPosition();
		if (pBall->GetRigidBody())
			vPosition = pBall->GetRigidBody()->GetPosition();

		const int32_t iScore = ResolveScoreAtPosition(vPosition);
		if (iScore < std::max(iMinimumScore, 1))
			continue;

		scoredBalls.push_back({ hBall, iScore });
	}

	// [LSY] 높은 점수 구역에 들어간 공이 보드의 더 먼 공이다.
	// 동일 점수에서는 등록 순서를 유지해 판단 결과가 매 프레임 바뀌지 않게 한다.
	std::stable_sort(
		scoredBalls.begin(),
		scoredBalls.end(),
		[](const SCORED_BALL& lhs, const SCORED_BALL& rhs)
		{
			return lhs.iScore > rhs.iScore;
		});

	std::vector<CHandle> result{};
	result.reserve(scoredBalls.size());
	for (const SCORED_BALL& scoredBall : scoredBalls)
		result.push_back(scoredBall.hBall);
	return result;
}

std::optional<CAccioActivity_Base::BALL_PATH_HIT>
CAccioActivity_Base::FindFirstBallOnPath(
	PARTICIPANT eParticipant,
	const CHandle& hIgnoredBall,
	const _float3& vPathStart,
	const _float3& vPathDirection,
	_float fMovingBallRadius,
	_float fMaximumPathDistance) const
{
	if (fMaximumPathDistance < 0.f)
		return std::nullopt;

	CAccioBall::COLOR eTargetColor = CAccioBall::COLOR::NONE;
	if (eParticipant == PARTICIPANT::PLAYER)
		eTargetColor = CAccioBall::COLOR::BLUE;
	else if (eParticipant == PARTICIPANT::NPC)
		eTargetColor = CAccioBall::COLOR::RED;
	else
		return std::nullopt;

	const _vector pathDirection = XMVectorSet(
		vPathDirection.x,
		0.f,
		vPathDirection.z,
		0.f);
	if (XMVectorGetX(XMVector3LengthSq(pathDirection)) <= FLT_EPSILON)
		return std::nullopt;

	const _vector normalizedDirection = XMVector3Normalize(pathDirection);
	std::optional<BALL_PATH_HIT> nearestHit{};
	for (const CHandle& hBall : m_BallHandles)
	{
		if (hBall == hIgnoredBall)
			continue;

		const auto* pBall = CGameInstance::Get().
			GetGameObjectByHandleT<CAccioBall>(hBall);
		if (!pBall || pBall->GetPendingDestroy() ||
			pBall->GetBallColor() != eTargetColor)
		{
			continue;
		}

		_float3 vBallPosition = pBall->GetTransform().GetPosition();
		if (pBall->GetRigidBody())
			vBallPosition = pBall->GetRigidBody()->GetPosition();

		const _vector toBall = XMVectorSet(
			vBallPosition.x - vPathStart.x,
			0.f,
			vBallPosition.z - vPathStart.z,
			0.f);
		const _float fCenterDistanceAlongPath = XMVectorGetX(
			XMVector3Dot(toBall, normalizedDirection));
		if (fCenterDistanceAlongPath <= 0.f)
			continue;

		const _float fToBallLengthSq = XMVectorGetX(
			XMVector3LengthSq(toBall));
		const _float fLateralDistanceSq = std::max(
			fToBallLengthSq -
			fCenterDistanceAlongPath * fCenterDistanceAlongPath,
			0.f);
		const _float fContactRadius = std::max(fMovingBallRadius, 0.f) +
			pBall->GetSphereRadius();
		const _float fContactRadiusSq = fContactRadius * fContactRadius;
		if (fLateralDistanceSq > fContactRadiusSq)
			continue;

		const _float fForwardContactOffset = sqrtf(std::max(
			fContactRadiusSq - fLateralDistanceSq,
			0.f));
		const _float fDistanceUntilCollision = std::max(
			fCenterDistanceAlongPath - fForwardContactOffset,
			0.f);
		if (fDistanceUntilCollision > fMaximumPathDistance)
			continue;

		if (nearestHit &&
			nearestHit->fDistanceUntilCollision <= fDistanceUntilCollision)
		{
			continue;
		}

		nearestHit = BALL_PATH_HIT{
			.hBall = hBall,
			.fDistanceUntilCollision = fDistanceUntilCollision
		};
	}

	return nearestHit;
}

std::optional<_float> CAccioActivity_Base::GetDistanceToPlayAreaEdge(
	const CAccioBall& ball,
	const _float3& vPushDirection) const
{
	if (!m_pComPxRigidBody)
		return std::nullopt;

	_float3 vBallPosition = ball.GetTransform().GetPosition();
	if (ball.GetRigidBody())
		vBallPosition = ball.GetRigidBody()->GetPosition();

	const _float3 vActorPosition = m_pComPxRigidBody->GetPosition();
	const _float4 vActorRotation = m_pComPxRigidBody->GetRotation();
	const _matrix playAreaLocal =
		XMMatrixRotationRollPitchYaw(
			m_PlayArea.vLocalRotation.x,
			m_PlayArea.vLocalRotation.y,
			m_PlayArea.vLocalRotation.z) *
		XMMatrixTranslation(
			m_PlayArea.vLocalOffset.x,
			m_PlayArea.vLocalOffset.y,
			m_PlayArea.vLocalOffset.z);
	const _matrix actorWorld =
		XMMatrixRotationQuaternion(XMLoadFloat4(&vActorRotation)) *
		XMMatrixTranslation(
			vActorPosition.x,
			vActorPosition.y,
			vActorPosition.z);

	_vector determinant{};
	const _matrix inversePlayArea = XMMatrixInverse(
		&determinant,
		playAreaLocal * actorWorld);
	if (fabsf(XMVectorGetX(determinant)) <= FLT_EPSILON)
		return std::nullopt;

	_float3 vLocalPosition{};
	XMStoreFloat3(
		&vLocalPosition,
		XMVector3TransformCoord(
			XMLoadFloat3(&vBallPosition),
			inversePlayArea));
	if (fabsf(vLocalPosition.x) > m_PlayArea.vHalfExtents.x ||
		fabsf(vLocalPosition.z) > m_PlayArea.vHalfExtents.z)
	{
		return std::nullopt;
	}

	_vector localDirection = XMVector3TransformNormal(
		XMVectorSet(vPushDirection.x, 0.f, vPushDirection.z, 0.f),
		inversePlayArea);
	localDirection = XMVectorSetY(localDirection, 0.f);
	if (XMVectorGetX(XMVector3LengthSq(localDirection)) <= FLT_EPSILON)
		return std::nullopt;

	_float3 vLocalDirection{};
	XMStoreFloat3(
		&vLocalDirection,
		XMVector3Normalize(localDirection));

	_float fDistanceToEdge = FLT_MAX;
	if (vLocalDirection.x > FLT_EPSILON)
	{
		fDistanceToEdge = std::min(
			fDistanceToEdge,
			(m_PlayArea.vHalfExtents.x - vLocalPosition.x) /
			vLocalDirection.x);
	}
	else if (vLocalDirection.x < -FLT_EPSILON)
	{
		fDistanceToEdge = std::min(
			fDistanceToEdge,
			(-m_PlayArea.vHalfExtents.x - vLocalPosition.x) /
			vLocalDirection.x);
	}

	if (vLocalDirection.z > FLT_EPSILON)
	{
		fDistanceToEdge = std::min(
			fDistanceToEdge,
			(m_PlayArea.vHalfExtents.z - vLocalPosition.z) /
			vLocalDirection.z);
	}
	else if (vLocalDirection.z < -FLT_EPSILON)
	{
		fDistanceToEdge = std::min(
			fDistanceToEdge,
			(-m_PlayArea.vHalfExtents.z - vLocalPosition.z) /
			vLocalDirection.z);
	}

	fDistanceToEdge = std::max(
		fDistanceToEdge - ball.GetSphereRadius(),
		0.f);
	if (!std::isfinite(fDistanceToEdge))
		return std::nullopt;

	return fDistanceToEdge;
}

void CAccioActivity_Base::SetParticipantHandle(
	PARTICIPANT eParticipant,
	const CHandle& hObject)
{
	if (eParticipant == PARTICIPANT::PLAYER)
		m_hPlayer = hObject;
	else if (eParticipant == PARTICIPANT::NPC)
		m_hNpc = hObject;
}

_bool CAccioActivity_Base::StartMatch()
{
	if (m_eMatchState != MATCH_STATE::READY)
		return false;

	const auto* pPlayer = CGameInstance::Get().
		GetGameObjectByHandle(m_hPlayer);
	const auto* pNpc = CGameInstance::Get().
		GetGameObjectByHandle(m_hNpc);
	if (!pPlayer || pPlayer->GetPendingDestroy() ||
		!pNpc || pNpc->GetPendingDestroy() ||
		!ValidateRegisteredBalls())
	{
		return false;
	}

	if (!ResetMatch(true))
		return false;

	m_eMatchState = m_bNpcStartsFirst ?
		MATCH_STATE::NPC_TURN : MATCH_STATE::PLAYER_TURN;
	GET_SINGLE(UIManager)->AssioMiniGameStart(!m_bNpcStartsFirst);
	return true;
}

_bool CAccioActivity_Base::ResetMatch(_bool bResetBalls)
{
	m_BallScoreStates.clear();
	m_UsedBalls.clear();
	m_eMatchState = MATCH_STATE::READY;
	ReleaseActiveBallControl();
	m_iCurrentRound = 0u;
	m_iBlueScore = 0;
	m_iRedScore = 0;
	m_bScoreUiSubmitted = false;
	if (GET_SINGLE(UIManager)->IsAssioMiniGameActive())
		GET_SINGLE(UIManager)->AssioMiniGameFinish();

	if (!bResetBalls)
		return true;

	_bool bResetSucceeded = true;
	for (const CHandle& hBall : m_BallHandles)
	{
		auto* pBall = CGameInstance::Get().
			GetGameObjectByHandleT<CAccioBall>(hBall);
		if (!pBall || pBall->GetPendingDestroy() ||
			!pBall->ResetToInitialPose())
		{
			bResetSucceeded = false;
		}
	}

	return bResetSucceeded;
}

_bool CAccioActivity_Base::CanControlBall(
	const CHandle& hController,
	const CHandle& hBall) const
{
	if (m_eMatchState == MATCH_STATE::READY)
		return true;
	if (m_eMatchState == MATCH_STATE::MATCH_END ||
		m_UsedBalls.contains(hBall) ||
		(m_hActiveBall != CHandle{} && m_hActiveBall != hBall))
	{
		return false;
	}

	const auto* pBall = CGameInstance::Get().
		GetGameObjectByHandleT<CAccioBall>(hBall);
	if (!pBall)
		return false;

	const PARTICIPANT eParticipant = ResolveParticipant(hController);
	if (m_eMatchState == MATCH_STATE::PLAYER_TURN)
	{
		return eParticipant == PARTICIPANT::PLAYER &&
			pBall->GetBallColor() == CAccioBall::COLOR::BLUE;
	}
	if (m_eMatchState == MATCH_STATE::NPC_TURN)
	{
		return eParticipant == PARTICIPANT::NPC &&
			pBall->GetBallColor() == CAccioBall::COLOR::RED;
	}

	return false;
}

void CAccioActivity_Base::NotifyBallControlAcquired(
	const CHandle& hController,
	const CHandle& hBall)
{
	if (m_eMatchState == MATCH_STATE::READY ||
		!CanControlBall(hController, hBall))
	{
		return;
	}

	m_hActiveBall = hBall;
}

void CAccioActivity_Base::NotifyBallControlReleased(
	const CHandle& hController,
	const CHandle& hBall)
{
	if (m_eMatchState == MATCH_STATE::READY || m_hActiveBall != hBall)
		return;

	const PARTICIPANT eParticipant = ResolveParticipant(hController);
	if (m_eMatchState == MATCH_STATE::PLAYER_TURN &&
		eParticipant == PARTICIPANT::PLAYER)
	{
		m_UsedBalls.insert(hBall);
		m_eMatchState = MATCH_STATE::WAIT_PLAYER_BALL_SETTLED;
	}
	else if (m_eMatchState == MATCH_STATE::NPC_TURN &&
		eParticipant == PARTICIPANT::NPC)
	{
		m_UsedBalls.insert(hBall);
		m_eMatchState = MATCH_STATE::WAIT_NPC_BALL_SETTLED;
	}
}

_bool CAccioActivity_Base::SkipNpcTurn(const CHandle& hController)
{
	if (m_eMatchState != MATCH_STATE::NPC_TURN ||
		ResolveParticipant(hController) != PARTICIPANT::NPC)
	{
		return false;
	}

	// [LSY] 사용할 NPC 공이 사라진 경우에도 0점 UI 연출을 거쳐 다음 턴으로 진행한다.
	m_hActiveBall = CHandle{};
	m_bScoreUiSubmitted = false;
	m_eMatchState = MATCH_STATE::WAIT_NPC_BALL_SETTLED;
	return true;
}

void CAccioActivity_Base::OnTriggerEnter(
	CGameObject* pObj,
	const PX_ON_TRIGGER_DATA& info)
{
	UpdateBallScoreOverlap(pObj, info, true);
}

void CAccioActivity_Base::OnTriggerExit(
	CGameObject* pObj,
	const PX_ON_TRIGGER_DATA& info)
{
	UpdateBallScoreOverlap(pObj, info, false);
}

void CAccioActivity_Base::UpdateBallScoreOverlap(
	CGameObject* pObj,
	const PX_ON_TRIGGER_DATA& info,
	_bool bEntered)
{
	if (!pObj || !info.bSelfIsTrigger)
		return;

	const uint8_t iZoneBit = GetScoreZoneBit(info.iSelfShapeSubIndex);
	if (iZoneBit == 0u || !pObj->IsA(CAccioBall::StaticType))
		return;

	auto* pBall = static_cast<CAccioBall*>(pObj);
	const CHandle ballHandle = pBall->GetHandle();
	if (std::ranges::find(m_BallHandles, ballHandle) == m_BallHandles.end())
		return;

	if (bEntered)
	{
		auto& state = m_BallScoreStates[ballHandle];
		state.iZoneMask |= iZoneBit;
		switch (pBall->GetBallColor())
		{
		case CAccioBall::COLOR::BLUE:
			state.eTeam = SCORE_TEAM::BLUE;
			break;
		case CAccioBall::COLOR::RED:
			state.eTeam = SCORE_TEAM::RED;
			break;
		default:
			state.eTeam = SCORE_TEAM::NONE;
			break;
		}
	}
	else
	{
		const auto iter = m_BallScoreStates.find(ballHandle);
		if (iter == m_BallScoreStates.end())
			return;

		iter->second.iZoneMask &= static_cast<uint8_t>(~iZoneBit);
		if (iter->second.iZoneMask == 0u)
			m_BallScoreStates.erase(iter);
	}

	UpdateSettledScores();
	RefreshScores();
}

void CAccioActivity_Base::UpdateSettledScores()
{
	std::erase_if(
		m_BallScoreStates,
		[this](const auto& entry)
		{
			const auto* pBall = CGameInstance::Get().
				GetGameObjectByHandleT<CAccioBall>(entry.first);
			return !pBall || pBall->GetPendingDestroy() ||
				entry.second.iZoneMask == 0u ||
				std::ranges::find(m_BallHandles, entry.first) ==
				m_BallHandles.end();
		});

	for (auto& [handle, state] : m_BallScoreStates)
	{
		const auto* pBall = CGameInstance::Get().
			GetGameObjectByHandleT<CAccioBall>(handle);
		if (!pBall)
			continue;
		if (!m_UsedBalls.contains(handle))
		{
			// [LSY] 아직 턴에 사용하지 않은 공은 Trigger 안에 있어도 점수로 확정하지 않는다.
			state.iCommittedScore = 0;
			state.bScoreCommitted = false;
			continue;
		}

		if (!pBall->IsSettled())
		{
			state.iCommittedScore = 0;
			state.bScoreCommitted = false;
			continue;
		}

		if (!state.bScoreCommitted)
		{
			const CComPxRigidBody* pRigidBody = pBall->GetRigidBody();
			state.iCommittedScore = pRigidBody ?
				ResolveScoreAtPosition(pRigidBody->GetPosition()) : 0;
			state.bScoreCommitted = true;
		}
	}
}

void CAccioActivity_Base::RefreshScores()
{
	m_iBlueScore = 0;
	m_iRedScore = 0;

	for (const auto& [_, state] : m_BallScoreStates)
	{
		if (!state.bScoreCommitted)
			continue;

		const int32_t iScore = state.iCommittedScore;
		if (state.eTeam == SCORE_TEAM::BLUE)
			m_iBlueScore += iScore;
		else if (state.eTeam == SCORE_TEAM::RED)
			m_iRedScore += iScore;
	}
}

void CAccioActivity_Base::UpdateTurnState()
{
	const _bool bWaitingForPlayer =
		m_eMatchState == MATCH_STATE::WAIT_PLAYER_BALL_SETTLED;
	const _bool bWaitingForNpc =
		m_eMatchState == MATCH_STATE::WAIT_NPC_BALL_SETTLED;
	if (!bWaitingForPlayer && !bWaitingForNpc)
		return;

	const auto* pBall = CGameInstance::Get().
		GetGameObjectByHandleT<CAccioBall>(m_hActiveBall);
	if (pBall && !IsBallOnPlayArea(*pBall))
	{
		// [LSY] 베이스를 벗어난 공은 점수와 Sleep 추적 대상에서 제외한다.
		m_BallScoreStates.erase(m_hActiveBall);
		RefreshScores();
	}
	if (!AreInPlayBallsSettled())
		return;

	const _bool bRoundFinished = m_bNpcStartsFirst ?
		bWaitingForPlayer : bWaitingForNpc;
	const _bool bFinalScore = bRoundFinished &&
		m_iCurrentRound + 1u >= m_iMaxRounds;
	auto* pUIManager = GET_SINGLE(UIManager);
	if (!m_bScoreUiSubmitted && pUIManager->IsAssioMiniGameActive())
	{
		int32_t iTurnScore = 0;
		if (const auto iter = m_BallScoreStates.find(m_hActiveBall);
			iter != m_BallScoreStates.end() && iter->second.bScoreCommitted)
		{
			iTurnScore = iter->second.iCommittedScore;
		}

		if (!pUIManager->AddScore(
			iTurnScore,
			m_iBlueScore,
			m_iRedScore,
			bWaitingForPlayer,
			bFinalScore))
		{
			return;
		}

		m_bScoreUiSubmitted = true;
		return;
	}
	if (m_bScoreUiSubmitted && !bFinalScore &&
		pUIManager->IsAssioMiniGameActive() &&
		!pUIManager->CanAddAssioScore())
	{
		return;
	}

	m_bScoreUiSubmitted = false;
	m_hActiveBall = CHandle{};
	if (!bRoundFinished)
	{
		m_eMatchState = bWaitingForPlayer ?
			MATCH_STATE::NPC_TURN : MATCH_STATE::PLAYER_TURN;
		return;
	}

	++m_iCurrentRound;
	if (m_iCurrentRound >= m_iMaxRounds)
		m_eMatchState = MATCH_STATE::MATCH_END;
	else
		m_eMatchState = m_bNpcStartsFirst ?
			MATCH_STATE::NPC_TURN : MATCH_STATE::PLAYER_TURN;
}

_bool CAccioActivity_Base::ValidateRegisteredBalls() const
{
	uint32_t iBlueBallCount = 0u;
	uint32_t iRedBallCount = 0u;
	for (const CHandle& hBall : m_BallHandles)
	{
		const auto* pBall = CGameInstance::Get().
			GetGameObjectByHandleT<CAccioBall>(hBall);
		if (!pBall || pBall->GetPendingDestroy())
			return false;

		if (pBall->GetBallColor() == CAccioBall::COLOR::BLUE)
			++iBlueBallCount;
		else if (pBall->GetBallColor() == CAccioBall::COLOR::RED)
			++iRedBallCount;
		else
			return false;
	}

	// [LSY] 한 라운드에 양 팀이 공 하나씩 사용하므로 팀별 라운드 수만큼 필요하다.
	return iBlueBallCount >= m_iMaxRounds &&
		iRedBallCount >= m_iMaxRounds;
}

void CAccioActivity_Base::ReleaseActiveBallControl()
{
	if (auto* pBall = CGameInstance::Get().
		GetGameObjectByHandleT<CAccioBall>(m_hActiveBall))
	{
		const CHandle hController = pBall->GetControllerHandle();
		if (hController != CHandle{})
			pBall->ReleaseControl(hController);
	}

	m_hActiveBall = CHandle{};
}

_bool CAccioActivity_Base::AreInPlayBallsSettled() const
{
	for (const CHandle& hBall : m_BallHandles)
	{
		const auto* pBall = CGameInstance::Get().
			GetGameObjectByHandleT<CAccioBall>(hBall);
		if (!pBall || pBall->GetPendingDestroy() ||
			!IsBallOnPlayArea(*pBall))
		{
			continue;
		}

		if (!pBall->IsSettled())
			return false;
	}

	return true;
}

_bool CAccioActivity_Base::IsBallOnPlayArea(const CAccioBall& ball) const
{
	if (!m_pComPxRigidBody)
		return false;

	_float3 vBallPosition = ball.GetTransform().GetPosition();
	if (ball.GetRigidBody())
		vBallPosition = ball.GetRigidBody()->GetPosition();

	const _float3 vActorPosition = m_pComPxRigidBody->GetPosition();
	const _float4 vActorRotation = m_pComPxRigidBody->GetRotation();
	const _matrix playAreaLocal =
		XMMatrixRotationRollPitchYaw(
			m_PlayArea.vLocalRotation.x,
			m_PlayArea.vLocalRotation.y,
			m_PlayArea.vLocalRotation.z) *
		XMMatrixTranslation(
			m_PlayArea.vLocalOffset.x,
			m_PlayArea.vLocalOffset.y,
			m_PlayArea.vLocalOffset.z);
	const _matrix actorWorld =
		XMMatrixRotationQuaternion(XMLoadFloat4(&vActorRotation)) *
		XMMatrixTranslation(
			vActorPosition.x,
			vActorPosition.y,
			vActorPosition.z);

	_vector determinant{};
	const _matrix inversePlayArea = XMMatrixInverse(
		&determinant,
		playAreaLocal * actorWorld);
	if (fabsf(XMVectorGetX(determinant)) <= FLT_EPSILON)
		return false;

	_float3 vLocalPosition{};
	XMStoreFloat3(
		&vLocalPosition,
		XMVector3TransformCoord(
			XMLoadFloat3(&vBallPosition),
			inversePlayArea));

	const _bool bInsideHorizontalBounds =
		fabsf(vLocalPosition.x) <= m_PlayArea.vHalfExtents.x &&
		fabsf(vLocalPosition.z) <= m_PlayArea.vHalfExtents.z;
	const _bool bAbovePlatformBottom =
		vLocalPosition.y >= -m_PlayArea.vHalfExtents.y;
	return bInsideHorizontalBounds && bAbovePlatformBottom;
}

CAccioActivity_Base::PARTICIPANT CAccioActivity_Base::ResolveParticipant(
	const CHandle& hController) const
{
	if (hController != CHandle{} && hController == m_hPlayer)
		return PARTICIPANT::PLAYER;
	if (hController != CHandle{} && hController == m_hNpc)
		return PARTICIPANT::NPC;
	return PARTICIPANT::NONE;
}

const _char* CAccioActivity_Base::GetMatchStateName(MATCH_STATE eState)
{
	switch (eState)
	{
	case MATCH_STATE::READY:
		return "Ready";
	case MATCH_STATE::PLAYER_TURN:
		return "Player Turn";
	case MATCH_STATE::WAIT_PLAYER_BALL_SETTLED:
		return "Wait Player Ball";
	case MATCH_STATE::NPC_TURN:
		return "NPC Turn";
	case MATCH_STATE::WAIT_NPC_BALL_SETTLED:
		return "Wait NPC Ball";
	case MATCH_STATE::MATCH_END:
		return "Match End";
	default:
		return "Unknown";
	}
}

uint8_t CAccioActivity_Base::GetScoreZoneBit(uint32_t iShapeSubIndex)
{
	switch (iShapeSubIndex)
	{
	case 10u:
		return 1u << 0;
	case 20u:
		return 1u << 1;
	case 30u:
		return 1u << 2;
	case 50u:
		return 1u << 3;
	default:
		return 0u;
	}
}

int32_t CAccioActivity_Base::ResolveScoreAtPosition(
	const _float3& vWorldPosition) const
{
	// 공유 경계선에서는 높은 점수 영역을 우선한다.
	if (IsPointInsideScoreZone(vWorldPosition, m_ScoreZones[3]))
		return 50;
	if (IsPointInsideScoreZone(vWorldPosition, m_ScoreZones[2]))
		return 30;
	if (IsPointInsideScoreZone(vWorldPosition, m_ScoreZones[1]))
		return 20;
	if (IsPointInsideScoreZone(vWorldPosition, m_ScoreZones[0]))
		return 10;
	return 0;
}

_bool CAccioActivity_Base::IsPointInsideScoreZone(
	const _float3& vWorldPosition,
	const ACCIO_ACTIVITY_BOX_COLLIDER_DESC& zone) const
{
	if (!m_pComPxRigidBody)
		return false;

	const _float3 vActorPosition = m_pComPxRigidBody->GetPosition();
	const _float4 vActorRotation = m_pComPxRigidBody->GetRotation();
	const _matrix localMatrix =
		XMMatrixRotationRollPitchYaw(
			zone.vLocalRotation.x,
			zone.vLocalRotation.y,
			zone.vLocalRotation.z) *
		XMMatrixTranslation(
			zone.vLocalOffset.x,
			zone.vLocalOffset.y,
			zone.vLocalOffset.z);
	const _matrix actorMatrix =
		XMMatrixRotationQuaternion(XMLoadFloat4(&vActorRotation)) *
		XMMatrixTranslation(
			vActorPosition.x,
			vActorPosition.y,
			vActorPosition.z);
	const _matrix zoneWorldMatrix = localMatrix * actorMatrix;

	_vector vDeterminant{};
	const _matrix inverseZoneWorld =
		XMMatrixInverse(&vDeterminant, zoneWorldMatrix);
	if (fabsf(XMVectorGetX(vDeterminant)) <= FLT_EPSILON)
		return false;

	const _vector vLocalPoint = XMVector3TransformCoord(
		XMLoadFloat3(&vWorldPosition), inverseZoneWorld);
	_float3 vLocalPosition{};
	XMStoreFloat3(&vLocalPosition, vLocalPoint);

	constexpr _float fBoundaryEpsilon = 0.0001f;
	return fabsf(vLocalPosition.x) <= zone.vHalfExtents.x + fBoundaryEpsilon &&
		fabsf(vLocalPosition.y) <= zone.vHalfExtents.y + fBoundaryEpsilon &&
		fabsf(vLocalPosition.z) <= zone.vHalfExtents.z + fBoundaryEpsilon;
}

HRESULT CAccioActivity_Base::InitializeBasePhysics(const DESC& desc)
{
	m_bNpcStartsFirst = desc.bNpcStartsFirst;
	m_PlayArea = desc.BoxColliders[3];
	m_ScoreZones = {
		desc.Score10Trigger,
		desc.Score20Trigger,
		desc.Score30Trigger,
		desc.Score50Trigger
	};

	{
		CComPxRigidBody::DESC rigidBodyDesc{};
		rigidBodyDesc.eType = CComPxRigidBody::TYPE::STATIC;
		rigidBodyDesc.vPosition = GetTransform().GetPosition();
		rigidBodyDesc.vRotation = GetTransform().GetQuaternion();
		if (FAILED(AddComponentFromProto(
			"PHYSX", "Prototype_Component_ComPxRigidBody",
			"ComPxRigidBody", &rigidBodyDesc, &m_pComPxRigidBody)))
		{
			return E_FAIL;
		}
	}

	auto material = CResPhysXMaterial::CreateAndLoad({});
	if (!material)
		return E_FAIL;

	for (size_t i = 0; i < desc.BoxColliders.size(); ++i)
	{
		const auto& box = desc.BoxColliders[i];
		CComPxBoxCollider::DESC colliderDesc{};
		colliderDesc.pComPxRigidBody = m_pComPxRigidBody;
		colliderDesc.pResMaterial = material;
		colliderDesc.pResBoxGeo = CResPhysXBoxGeometry::CreateAndLoad({
			.vHalfExtents = box.vHalfExtents });
		colliderDesc.vLocalOffset = box.vLocalOffset;
		colliderDesc.tFilter = desc.tPhysicsFilter;

		const _string componentTag = "ComPxBoxCollider_" + std::to_string(i);
		if (!colliderDesc.pResBoxGeo || FAILED(AddComponentFromProto(
			"PHYSX", "Prototype_Component_ComPxBoxCollider",
			componentTag, &colliderDesc, &m_pComPxBoxColliders[i])))
		{
			return E_FAIL;
		}

		_float4 localRotation{};
		XMStoreFloat4(&localRotation, XMQuaternionRotationRollPitchYaw(
			box.vLocalRotation.x, box.vLocalRotation.y, box.vLocalRotation.z));
		if (!m_pComPxBoxColliders[i]->SetLocalRotation(localRotation))
			return E_FAIL;
	}

	{
		const auto& box = desc.Score10Trigger;
		CComPxBoxCollider::DESC colliderDesc{};
		colliderDesc.pComPxRigidBody = m_pComPxRigidBody;
		colliderDesc.pResMaterial = material;
		colliderDesc.pResBoxGeo = CResPhysXBoxGeometry::CreateAndLoad({
			.vHalfExtents = box.vHalfExtents });
		colliderDesc.vLocalOffset = box.vLocalOffset;
		colliderDesc.bIsTrigger = true;
		colliderDesc.iShapeSubIndex = 10u;
		colliderDesc.tFilter = desc.tScore10TriggerFilter;

		if (!colliderDesc.pResBoxGeo || FAILED(AddComponentFromProto(
			"PHYSX", "Prototype_Component_ComPxBoxCollider",
			"Score_10", &colliderDesc, &m_pComPxScore10Trigger)))
		{
			return E_FAIL;
		}

		_float4 localRotation{};
		XMStoreFloat4(&localRotation, XMQuaternionRotationRollPitchYaw(
			box.vLocalRotation.x, box.vLocalRotation.y, box.vLocalRotation.z));
		if (!m_pComPxScore10Trigger->SetLocalRotation(localRotation))
			return E_FAIL;
	}

	{
		const auto& box = desc.Score20Trigger;
		CComPxBoxCollider::DESC colliderDesc{};
		colliderDesc.pComPxRigidBody = m_pComPxRigidBody;
		colliderDesc.pResMaterial = material;
		colliderDesc.pResBoxGeo = CResPhysXBoxGeometry::CreateAndLoad({
			.vHalfExtents = box.vHalfExtents });
		colliderDesc.vLocalOffset = box.vLocalOffset;
		colliderDesc.bIsTrigger = true;
		colliderDesc.iShapeSubIndex = 20u;
		colliderDesc.tFilter = desc.tScore20TriggerFilter;

		if (!colliderDesc.pResBoxGeo || FAILED(AddComponentFromProto(
			"PHYSX", "Prototype_Component_ComPxBoxCollider",
			"Score_20", &colliderDesc, &m_pComPxScore20Trigger)))
		{
			return E_FAIL;
		}

		_float4 localRotation{};
		XMStoreFloat4(&localRotation, XMQuaternionRotationRollPitchYaw(
			box.vLocalRotation.x, box.vLocalRotation.y, box.vLocalRotation.z));
		if (!m_pComPxScore20Trigger->SetLocalRotation(localRotation))
			return E_FAIL;
	}

	{
		const auto& box = desc.Score30Trigger;
		CComPxBoxCollider::DESC colliderDesc{};
		colliderDesc.pComPxRigidBody = m_pComPxRigidBody;
		colliderDesc.pResMaterial = material;
		colliderDesc.pResBoxGeo = CResPhysXBoxGeometry::CreateAndLoad({
			.vHalfExtents = box.vHalfExtents });
		colliderDesc.vLocalOffset = box.vLocalOffset;
		colliderDesc.bIsTrigger = true;
		colliderDesc.iShapeSubIndex = 30u;
		colliderDesc.tFilter = desc.tScore30TriggerFilter;

		if (!colliderDesc.pResBoxGeo || FAILED(AddComponentFromProto(
			"PHYSX", "Prototype_Component_ComPxBoxCollider",
			"Score_30", &colliderDesc, &m_pComPxScore30Trigger)))
		{
			return E_FAIL;
		}

		_float4 localRotation{};
		XMStoreFloat4(&localRotation, XMQuaternionRotationRollPitchYaw(
			box.vLocalRotation.x, box.vLocalRotation.y, box.vLocalRotation.z));
		if (!m_pComPxScore30Trigger->SetLocalRotation(localRotation))
			return E_FAIL;
	}

	{
		const auto& box = desc.Score50Trigger;
		CComPxBoxCollider::DESC colliderDesc{};
		colliderDesc.pComPxRigidBody = m_pComPxRigidBody;
		colliderDesc.pResMaterial = material;
		colliderDesc.pResBoxGeo = CResPhysXBoxGeometry::CreateAndLoad({
			.vHalfExtents = box.vHalfExtents });
		colliderDesc.vLocalOffset = box.vLocalOffset;
		colliderDesc.bIsTrigger = true;
		colliderDesc.iShapeSubIndex = 50u;
		colliderDesc.tFilter = desc.tScore50TriggerFilter;

		if (!colliderDesc.pResBoxGeo || FAILED(AddComponentFromProto(
			"PHYSX", "Prototype_Component_ComPxBoxCollider",
			"Score_50", &colliderDesc, &m_pComPxScore50Trigger)))
		{
			return E_FAIL;
		}

		_float4 localRotation{};
		XMStoreFloat4(&localRotation, XMQuaternionRotationRollPitchYaw(
			box.vLocalRotation.x, box.vLocalRotation.y, box.vLocalRotation.z));
		if (!m_pComPxScore50Trigger->SetLocalRotation(localRotation))
			return E_FAIL;
	}

	return S_OK;
}

UPtr<CAccioActivity_Base> CAccioActivity_Base::Create()
{
	auto pInstance = ToUPtr(new CAccioActivity_Base{});
	if (FAILED(pInstance->InitializePrototype()))
		return nullptr;
	return pInstance;
}

UPtr<CPrototype> CAccioActivity_Base::Clone(void* pArg)
{
	if (!pArg)
		return nullptr;

	auto pInstance = ToUPtr(new CAccioActivity_Base{ *this });
	if (FAILED(pInstance->Initialize(pArg)) ||
		FAILED(pInstance->InitializeBasePhysics(
			*static_cast<const DESC*>(pArg))))
	{
		return nullptr;
	}
	return pInstance;
}
