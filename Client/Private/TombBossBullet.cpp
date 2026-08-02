#include "pch.h"
#include "TombBossBullet.h"

#include "ComPathPlayback.h"
#include "DbgLineRender.h"
#include "PhysXManager.h"
#include "ResPathPlayback.h"

NS_USING(Client)

static PATH_PLAYBACK_KEYFRAME TombBossBulletMakePathKeyframe(
	_float fTime,
	const _float3& vPosition,
	const StringID& sEventTag = {},
	PATH_PLAYBACK_EASING eEasing = PATH_PLAYBACK_EASING::LINEAR)
{
	PATH_PLAYBACK_KEYFRAME Keyframe{};
	Keyframe.fTime = fTime;
	Keyframe.vPosition = vPosition;
	Keyframe.vRotation = { 0.f, 0.f, 0.f, 1.f };
	Keyframe.ePositionInterpolation =
		PATH_PLAYBACK_INTERPOLATION::LINEAR;
	Keyframe.eEasing = eEasing;
	Keyframe.sEventTag = sEventTag;
	return Keyframe;
}

CTombBossBullet::CTombBossBullet() = default;

CTombBossBullet::CTombBossBullet(
	const CTombBossBullet& Prototype)
	: CGameObject{ Prototype }
	, m_pPathResource{ Prototype.m_pPathResource }
{
}

HRESULT CTombBossBullet::InitializePrototype(void* pArg)
{
	return BuildTestPathResource();
}

HRESULT CTombBossBullet::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<const DESC*>(pArg);
	if (!pDesc || !m_pPathResource ||
		FAILED(CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	}

	if (pDesc->fArcMoveSpeed <= 0.f ||
		pDesc->fArcHeight < 0.f ||
		pDesc->fArcLifeTime <= 0.f ||
		pDesc->fRadius <= 0.f)
		return E_INVALIDARG;

	m_hTarget = pDesc->hTarget;
	m_vTargetOffset = pDesc->vTargetOffset;
	m_fArcMoveSpeed = pDesc->fArcMoveSpeed;
	m_fArcHeight = pDesc->fArcHeight;
	m_fArcLifeTime = pDesc->fArcLifeTime;
	m_fRadius = pDesc->fRadius;
	m_tQueryFilter = pDesc->tQueryFilter;
	m_eMoveState = MOVE_STATE::PATH_PLAYBACK;

	GetTransform().SetPosition(pDesc->vInitialPosition);
	GetTransform().SetQuaternion(pDesc->vInitialRotation);
	GetTransform().Update();

	CComPathPlayback::DESC PathDesc{};
	PathDesc.pPathResource = m_pPathResource;
	PathDesc.fPlaybackRate = pDesc->fPlaybackRate;
	if (FAILED(AddComponentFromProto(
		ES_EngineProtoMajorType::PERMANENT,
		ES_EngineProtoComponent::Prototype_Component_ComPathPlayback,
		"ComPathPlayback",
		&PathDesc,
		&m_pComPathPlayback)) ||
		!m_pComPathPlayback ||
		!m_pComPathPlayback->Play("BossBulletPrepare"))
	{
		return E_FAIL;
	}

	const PATH_PLAYBACK_POSE& StartPose =
		m_pComPathPlayback->GetCurrentPose();
	GetTransform().SetPosition(StartPose.vPosition);
	GetTransform().SetQuaternion(StartPose.vRotation);
	GetTransform().Update();

	if (false && !pDesc->sEffectName.empty())
	{
		m_iEffectID = CGameInstance::Get().PlayEffect(
			pDesc->sEffectName,
			*GetTransform().GetWorldMatrix(),
			XMVectorZero(),
			[hOwner = GetHandle()](
				EFFECT_INSTANCE_ID,
				EFFECT_FINISH_REASON)
			{
				if (auto* pBullet = CGameInstance::Get().
					GetGameObjectByHandleT<CTombBossBullet>(hOwner))
				{
					pBullet->m_iEffectID =
						INVALID_EFFECT_INSTANCE_ID;
				}
			});
	}

	return S_OK;
}

void CTombBossBullet::FixedUpdate(_float fTimeDelta)
{
	if (fTimeDelta <= 0.f)
		return;

	switch (m_eMoveState)
	{
	case MOVE_STATE::PATH_PLAYBACK:
		FixedUpdatePathPlayback(fTimeDelta);
		break;

	case MOVE_STATE::TARGET_ARC:
		FixedUpdateTargetArc(fTimeDelta);
		break;

	case MOVE_STATE::FINISHED:
		break;
	}
}

void CTombBossBullet::LateUpdate(_float fTimeDelta)
{
	GetTransform().Update();

	auto* pDebug = CGameInstance::Get().GetDbgLineRender();
	if (!pDebug)
		return;

	const _float4 PreviousColor = pDebug->GetColor();
	const DBG_LINE_DEPTH_MODE PreviousDepth = pDebug->GetDepthMode();
	pDebug->SetDepthTest(false);
	pDebug->SetColor({ 1.f, 0.15f, 0.1f, 1.f });
	pDebug->AddSphere(
		m_fRadius,
		GetTransform().GetLoadedWorldMatrix());
	pDebug->SetColor(PreviousColor);
	pDebug->SetDepthMode(PreviousDepth);
}

void CTombBossBullet::UpdateGUI()
{
	CGameObject::UpdateGUI();
	ImGui::Separator();
	ImGui::Text(
		"Boss Bullet State: %s",
		magic_enum::enum_name(m_eMoveState).data());
	ImGui::Text("Arc Move Speed: %.2f", m_fArcMoveSpeed);
	ImGui::Text("Arc Height: %.2f", m_fArcHeight);
	ImGui::Text("Arc Life Time: %.2f", m_fArcLifeTime);
	ImGui::Text("Radius: %.2f", m_fRadius);
}

HRESULT CTombBossBullet::BuildTestPathResource()
{
	PATH_PLAYBACK_CLIP Clip{};
	Clip.sClipID = "BossBulletPrepare";
	Clip.eCoordinateSpace =
		PATH_PLAYBACK_COORDINATE_SPACE::START_LOCAL;
	Clip.eRotationMode =
		PATH_PLAYBACK_ROTATION_MODE::FACE_DIRECTION;
	Clip.ePlayMode = PATH_PLAYBACK_MODE::ONCE;
	Clip.eFinishBehavior =
		PATH_PLAYBACK_FINISH_BEHAVIOR::HOLD_LAST;
	Clip.Keyframes = {
		TombBossBulletMakePathKeyframe(
			0.f,
			{ 0.f, 0.f, 0.f },
			{},
			PATH_PLAYBACK_EASING::EASE_IN),
		TombBossBulletMakePathKeyframe(
			1.5f,
			{ 0.f, 3.f, 4.f },
			"HoldStart"),
		// 같은 위치를 다른 시간에 한 번 더 기록해 2초 동안 정지한다.
		TombBossBulletMakePathKeyframe(
			3.5f,
			{ 0.f, 3.f, 4.f },
			"HoldEnd",
			PATH_PLAYBACK_EASING::EASE_IN),
		TombBossBulletMakePathKeyframe(
			4.25f,
			{ 0.f, 2.f, 7.f },
			"StartTracking")
	};

	PATH_PLAYBACK_DATA Data{};
	Data.Clips.push_back(std::move(Clip));
	m_pPathResource =
		CResPathPlayback::CreateFromData(std::move(Data));
	return m_pPathResource ? S_OK : E_FAIL;
}

void CTombBossBullet::FixedUpdatePathPlayback(_float fTimeDelta)
{
	if (!m_pComPathPlayback ||
		!m_pComPathPlayback->IsPlaying())
	{
		return;
	}

	const PATH_PLAYBACK_STEP_RESULT Step =
		m_pComPathPlayback->EvaluateNext(fTimeDelta);
	if (!Step.bValid ||
		!m_pComPathPlayback->CommitEvaluatedStep())
	{
		return;
	}

	const PATH_PLAYBACK_POSE& Pose =
		m_pComPathPlayback->GetCurrentPose();
	GetTransform().SetPosition(Pose.vPosition);
	GetTransform().SetQuaternion(Pose.vRotation);
	GetTransform().Update();
	UpdateEffectTransform();
	HandleCommittedPathEvents();

	// 마지막 키프레임에 이벤트가 빠져도 자연 완료되면 추적으로 전환한다.
	if (m_pComPathPlayback->IsCompleted() &&
		m_eMoveState == MOVE_STATE::PATH_PLAYBACK)
	{
		BeginTargetArc();
	}
}

void CTombBossBullet::BeginTargetArc()
{
	CGameObject* pTarget =
		CGameInstance::Get().GetGameObjectByHandle(m_hTarget);
	if (!pTarget)
	{
		m_eMoveState = MOVE_STATE::FINISHED;
		SetPendingDestroy();
		return;
	}

	m_vArcStartPosition = GetTransform().GetPosition();

	const _vector Target =
		pTarget->GetTransform().GetLoadedPostion() +
		XMLoadFloat3(&m_vTargetOffset);
	const _vector Direction =
		Target - XMLoadFloat3(&m_vArcStartPosition);
	if (XMVectorGetX(XMVector3LengthSq(Direction)) <= FLT_EPSILON)
	{
		m_eMoveState = MOVE_STATE::FINISHED;
		SetPendingDestroy();
		return;
	}

	XMStoreFloat3(
		&m_vArcDirection,
		XMVector3Normalize(Direction));
	m_fArcElapsedTime = 0.f;
	m_eMoveState = MOVE_STATE::TARGET_ARC;
}

void CTombBossBullet::FixedUpdateTargetArc(_float fTimeDelta)
{
	m_fArcElapsedTime = std::min(
		m_fArcElapsedTime + fTimeDelta,
		m_fArcLifeTime);
	const _float fRatio = std::clamp(
		m_fArcElapsedTime / m_fArcLifeTime,
		0.f,
		1.f);

	const _vector Start = XMLoadFloat3(&m_vArcStartPosition);
	const _vector Direction = XMLoadFloat3(&m_vArcDirection);
	_vector Next = Start +
		Direction * m_fArcMoveSpeed * m_fArcElapsedTime;
	Next += XMVectorSet(
		0.f,
		4.f * m_fArcHeight * fRatio * (1.f - fRatio),
		0.f,
		0.f);

	_float3 NextPosition{};
	XMStoreFloat3(&NextPosition, Next);

	PX_SWEEP_RESULT Hit{};
	if (SweepTo(NextPosition, Hit))
	{
		FinishByHit(Hit);
		return;
	}

	GetTransform().SetPosition(NextPosition);
	GetTransform().Update();
	UpdateEffectTransform();

	if (fRatio >= 1.f)
	{
		if (m_iEffectID != INVALID_EFFECT_INSTANCE_ID)
		{
			CGameInstance::Get().StopEffect(m_iEffectID);
			m_iEffectID = INVALID_EFFECT_INSTANCE_ID;
		}

		m_eMoveState = MOVE_STATE::FINISHED;
		SetPendingDestroy();
	}
}

void CTombBossBullet::HandleCommittedPathEvents()
{
	if (!m_pComPathPlayback)
		return;

	for (const size_t iKeyframe :
		m_pComPathPlayback->GetReachedKeyframeIndicesThisCommit())
	{
		const PATH_PLAYBACK_KEYFRAME* pKeyframe =
			m_pComPathPlayback->GetKeyframe(iKeyframe);
		if (!pKeyframe || pKeyframe->sEventTag.hash == 0)
			continue;

		DEBUG_LOG_STR(
			std::string{ "[TombBossBullet] Path Event: " } +
			pKeyframe->sEventTag.GetDbgStr() + "\n");

		if (pKeyframe->sEventTag == StringID{ "StartTracking" })
		{
			// 마지막 키프레임이므로 Path는 COMPLETED로 남기고
			// 실제 추적 이동은 다음 Fixed Tick부터 시작한다.
			BeginTargetArc();
			break;
		}
	}
}

_bool CTombBossBullet::SweepTo(
	const _float3& vTargetPosition,
	PX_SWEEP_RESULT& OutHit) const
{
	const _float3 vCurrentPosition = GetTransform().GetPosition();
	const _vector Displacement =
		XMLoadFloat3(&vTargetPosition) -
		XMLoadFloat3(&vCurrentPosition);
	const _float fDistance =
		XMVectorGetX(XMVector3Length(Displacement));
	if (fDistance <= FLT_EPSILON)
		return false;

	PX_SWEEP_DESC Desc{};
	Desc.tGeometry.eType = PX_QUERY_GEOMETRY_TYPE::SPHERE;
	Desc.tGeometry.fRadius = m_fRadius;
	Desc.tPose.vPosition = vCurrentPosition;
	XMStoreFloat3(
		&Desc.vDirection,
		XMVector3Normalize(Displacement));
	Desc.fMaxDistance = fDistance;
	Desc.tFilter = m_tQueryFilter;

	auto* pPhysXManager =
		CGameInstance::Get().GetPhysXManager();
	return pPhysXManager &&
		pPhysXManager->Sweep(Desc, OutHit) &&
		OutHit.bHit;
}

void CTombBossBullet::FinishByHit(const PX_SWEEP_RESULT& Hit)
{
	const _float3 vCurrentPosition = GetTransform().GetPosition();
	const _vector Direction =
		XMLoadFloat3(&Hit.vHitpos) -
		XMLoadFloat3(&vCurrentPosition);
	const _float fDirectionLengthSq =
		XMVectorGetX(XMVector3LengthSq(Direction));

	_float3 vFinalPosition = Hit.vHitpos;
	if (fDirectionLengthSq > FLT_EPSILON)
	{
		XMStoreFloat3(
			&vFinalPosition,
			XMLoadFloat3(&vCurrentPosition) +
			XMVector3Normalize(Direction) * Hit.fDistance);
	}

	GetTransform().SetPosition(vFinalPosition);
	GetTransform().Update();
	UpdateEffectTransform();

	DEBUG_LOG_STR(
		std::string{ "[TombBossBullet] Sweep Hit: " } +
		(Hit.pGameObject
			? std::string{ Hit.pGameObject->GetObjectTag() }
			: std::string{ "null" }) + "\n");

	if (m_iEffectID != INVALID_EFFECT_INSTANCE_ID)
	{
		CGameInstance::Get().StopEffect(m_iEffectID);
		m_iEffectID = INVALID_EFFECT_INSTANCE_ID;
	}

	m_eMoveState = MOVE_STATE::FINISHED;
	SetPendingDestroy();
}

void CTombBossBullet::UpdateEffectTransform()
{
	if (m_iEffectID == INVALID_EFFECT_INSTANCE_ID)
		return;

	CGameInstance::Get().SetEffectWorldMatrix(
		m_iEffectID,
		*GetTransform().GetWorldMatrix());
}

UPtr<CTombBossBullet> CTombBossBullet::Create()
{
	auto pInstance = ToUPtr(new CTombBossBullet{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Create: CTombBossBullet");
		return nullptr;
	}
	return pInstance;
}

UPtr<CPrototype> CTombBossBullet::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CTombBossBullet{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Clone: CTombBossBullet");
		return nullptr;
	}
	return pInstance;
}
