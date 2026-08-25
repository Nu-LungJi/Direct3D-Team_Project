#include "pch.h"
#include "AccioActivity_Base.h"

#include "AccioBall.h"
#include "ComPxBoxCollider.h"
#include "ComPxRigidBody.h"
#include "GameInstance.h"
#include "ResPhysXBoxGeometry.h"
#include "ResPhysXMaterial.h"

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

void CAccioActivity_Base::FixedUpdate(_float)
{
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
		if (ImGui::Button("Start Accio Match") && !StartMatch())
			DEBUG_LOG("[AccioActivity] Player or NPC is not registered.\n");
	}
	else if (ImGui::Button("Reset Accio Match"))
	{
		ResetMatch(true);
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
	if (!CGameInstance::Get().GetGameObjectByHandle(m_hPlayer) ||
		!CGameInstance::Get().GetGameObjectByHandle(m_hNpc) ||
		m_BallHandles.empty())
	{
		return false;
	}

	ResetMatch(true);
	m_eMatchState = MATCH_STATE::PLAYER_TURN;
	return true;
}

void CAccioActivity_Base::ResetMatch(_bool bResetBalls)
{
	m_BallScoreStates.clear();
	m_UsedBalls.clear();
	m_hActiveBall = CHandle{};
	m_eMatchState = MATCH_STATE::READY;
	m_iCurrentRound = 0u;
	m_iBlueScore = 0;
	m_iRedScore = 0;

	if (!bResetBalls)
		return;

	for (const CHandle& hBall : m_BallHandles)
	{
		if (auto* pBall = CGameInstance::Get().
			GetGameObjectByHandleT<CAccioBall>(hBall))
		{
			pBall->ResetToInitialPose();
		}
	}
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
		[](const auto& entry)
		{
			const auto* pBall = CGameInstance::Get().
				GetGameObjectByHandleT<CAccioBall>(entry.first);
			return !pBall || pBall->GetPendingDestroy() ||
				entry.second.iZoneMask == 0u;
		});

	for (auto& [handle, state] : m_BallScoreStates)
	{
		const auto* pBall = CGameInstance::Get().
			GetGameObjectByHandleT<CAccioBall>(handle);
		if (!pBall)
			continue;

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
	if (pBall && !pBall->IsSettled())
		return;

	m_hActiveBall = CHandle{};
	if (bWaitingForPlayer)
	{
		m_eMatchState = MATCH_STATE::NPC_TURN;
		return;
	}

	++m_iCurrentRound;
	if (m_iCurrentRound >= m_iMaxRounds)
		m_eMatchState = MATCH_STATE::MATCH_END;
	else
		m_eMatchState = MATCH_STATE::PLAYER_TURN;
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
