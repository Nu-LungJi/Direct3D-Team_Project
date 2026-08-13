#include "pch.h"
#include "Player_Confringo_Bullet.h"

#include "ComSound.h"
#include "DbgLineRender.h"
#include "Monster.h"
#include "PhysXManager.h"
#include "SoundManager.h"

NS_USING(Client)

CPlayer_Confringo_Bullet::CPlayer_Confringo_Bullet() = default;

CPlayer_Confringo_Bullet::CPlayer_Confringo_Bullet(
	const CPlayer_Confringo_Bullet& Prototype)
	: CGameObject{ Prototype }
{
}

CPlayer_Confringo_Bullet::~CPlayer_Confringo_Bullet()
{
	StopFlightSound();
	StopProjectileEffect();
}

HRESULT CPlayer_Confringo_Bullet::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CPlayer_Confringo_Bullet::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<const DESC*>(pArg);
	if (!pDesc || FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	if (pDesc->fSpeed <= 0.f ||
		pDesc->fLifeTime <= 0.f ||
		pDesc->fRadius <= 0.f)
	{
		return E_INVALIDARG;
	}

	const _vector vToEnd =
		XMLoadFloat3(&pDesc->vEndPosition) -
		XMLoadFloat3(&pDesc->vStartPosition);
	if (XMVectorGetX(XMVector3LengthSq(vToEnd)) <= FLT_EPSILON)
		return E_INVALIDARG;

	m_vEndPosition = pDesc->vEndPosition;
	m_hOwner = pDesc->hOwner;
	m_fSpeed = pDesc->fSpeed;
	m_fLifeTime = pDesc->fLifeTime;
	m_fRadius = pDesc->fRadius;
	m_bDebugDraw = pDesc->bDebugDraw;
	m_eSkillType = pDesc->eSkillType;
	m_sProjectileEffectName = pDesc->sProjectileEffectName;
	m_sTrailParticleQueue = pDesc->sTrailParticleQueue;
	m_sImpactEffectName = pDesc->sImpactEffectName;
	m_fTrailSpacing = std::max(pDesc->fTrailSpacing, 0.01f);
	m_fTrailDistanceAccumulator = 0.f;
	m_tQueryFilter = pDesc->tQueryFilter;
	m_tQueryFilter.hIgnoreGameObject = m_hOwner;
	m_fElapsedTime = 0.f;
	m_bFinished = false;
	m_iPathSegmentIndex = 0;
	m_fDistanceOnPathSegment = 0.f;

	BuildDynamicPath(
		pDesc->vStartPosition,
		pDesc->vEndPosition,
		std::max(pDesc->fCurveAmplitude, 0.f),
		std::max(pDesc->fCurveFrequency, 0.f),
		pDesc->iPathSampleCount);
	if (m_PathPoints.size() < 2)
		return E_FAIL;

	m_fRemainingDistance = 0.f;
	for (size_t i = 1; i < m_PathPoints.size(); ++i)
	{
		m_fRemainingDistance += XMVectorGetX(XMVector3Length(
			XMLoadFloat3(&m_PathPoints[i]) -
			XMLoadFloat3(&m_PathPoints[i - 1])));
	}

	_float3 vInitialDirection{};
	XMStoreFloat3(
		&vInitialDirection,
		XMVector3Normalize(
			XMLoadFloat3(&m_PathPoints[1]) -
			XMLoadFloat3(&m_PathPoints[0])));
	UpdateFlightTransform(m_PathPoints.front(), vInitialDirection);

	CComSound::DESC SoundDesc{};
	if (FAILED(AddComponentFromProto(
		ES_EngineProtoMajorType::PERMANENT,
		ES_EngineProtoComponent::Prototype_Component_ComSound,
		"Com_Sound",
		&SoundDesc,
		&m_pComSound)))
	{
		return E_FAIL;
	}

	return S_OK;
}

void CPlayer_Confringo_Bullet::OnRegisteredToManager()
{
	StartProjectileEffect();
	StartFlightSound();
}

void CPlayer_Confringo_Bullet::FixedUpdate(_float fTimeDelta)
{
	if (m_bFinished || fTimeDelta <= 0.f)
		return;

	m_fElapsedTime += fTimeDelta;
	if (m_fElapsedTime >= m_fLifeTime)
	{
		FinishWithoutHit();
		return;
	}

	_float fMoveBudget = std::min(
		m_fSpeed * fTimeDelta,
		m_fRemainingDistance);
	if (fMoveBudget <= FLT_EPSILON)
	{
		FinishAtEndPosition();
		return;
	}

	while (fMoveBudget > FLT_EPSILON &&
		m_iPathSegmentIndex + 1 < m_PathPoints.size())
	{
		const _vector vSegmentStart =
			XMLoadFloat3(&m_PathPoints[m_iPathSegmentIndex]);
		const _vector vSegmentEnd =
			XMLoadFloat3(&m_PathPoints[m_iPathSegmentIndex + 1]);
		const _vector vSegment = vSegmentEnd - vSegmentStart;
		const _float fSegmentLength =
			XMVectorGetX(XMVector3Length(vSegment));

		if (fSegmentLength <= FLT_EPSILON)
		{
			++m_iPathSegmentIndex;
			m_fDistanceOnPathSegment = 0.f;
			continue;
		}

		const _float fSegmentRemaining =
			fSegmentLength - m_fDistanceOnPathSegment;
		const _float fMoveDistance =
			std::min(fMoveBudget, fSegmentRemaining);
		const _float fStartRatio =
			m_fDistanceOnPathSegment / fSegmentLength;
		const _float fEndRatio =
			(m_fDistanceOnPathSegment + fMoveDistance) /
			fSegmentLength;

		_float3 vMoveStart{};
		_float3 vMoveEnd{};
		XMStoreFloat3(
			&vMoveStart,
			XMVectorLerp(vSegmentStart, vSegmentEnd, fStartRatio));
		XMStoreFloat3(
			&vMoveEnd,
			XMVectorLerp(vSegmentStart, vSegmentEnd, fEndRatio));
		XMStoreFloat3(&m_vDirection, XMVector3Normalize(vSegment));

		PX_SWEEP_RESULT Hit{};
		if (SweepTo(vMoveStart, vMoveEnd, Hit))
		{
			UpdateFlightTransform(vMoveStart, m_vDirection);
			HandleHit(Hit);
			return;
		}

		UpdateFlightTransform(vMoveEnd, m_vDirection);
		EmitTrailBetween(vMoveStart, vMoveEnd);
		UpdateProjectileEffect();

		m_fDistanceOnPathSegment += fMoveDistance;
		m_fRemainingDistance = std::max(
			0.f,
			m_fRemainingDistance - fMoveDistance);
		fMoveBudget -= fMoveDistance;

		if (m_fDistanceOnPathSegment >= fSegmentLength - FLT_EPSILON)
		{
			++m_iPathSegmentIndex;
			m_fDistanceOnPathSegment = 0.f;
		}
	}

	if (m_fRemainingDistance <= FLT_EPSILON ||
		m_iPathSegmentIndex + 1 >= m_PathPoints.size())
		FinishAtEndPosition();
}

void CPlayer_Confringo_Bullet::Update(_float)
{
	UpdateFlightSound();
	if (m_pComSound)
		m_pComSound->Update();
}

void CPlayer_Confringo_Bullet::LateUpdate(_float fTimeDelta)
{
	GetTransform().Update();

	if (!m_bDebugDraw)
		return;

	auto* pDebug = CGameInstance::Get().GetDbgLineRender();
	if (!pDebug)
		return;

	const _float4 vPreviousColor = pDebug->GetColor();
	const DBG_LINE_DEPTH_MODE ePreviousDepth = pDebug->GetDepthMode();
	pDebug->SetDepthTest(false);
	pDebug->SetColor({ 1.f, 0.2f, 0.02f, 1.f });
	pDebug->AddSphere(
		m_fRadius,
		GetTransform().GetLoadedWorldMatrix());
	for (size_t i = 1; i < m_PathPoints.size(); ++i)
	{
		pDebug->AddLine(
			m_PathPoints[i - 1],
			m_PathPoints[i],
			{ 1.f, 0.25f, 0.02f, 0.65f });
	}
	pDebug->SetColor(vPreviousColor);
	pDebug->SetDepthMode(ePreviousDepth);
}

void CPlayer_Confringo_Bullet::UpdateGUI()
{
	CGameObject::UpdateGUI();
	ImGui::Separator();
	ImGui::Text("Confringo Projectile");
	ImGui::Text("Speed: %.2f", m_fSpeed);
	ImGui::Text("Life: %.2f / %.2f", m_fElapsedTime, m_fLifeTime);
	ImGui::Text("Sweep Radius: %.3f", m_fRadius);
	ImGui::Text("Trail Spacing: %.3f", m_fTrailSpacing);
	ImGui::Text("Remaining Distance: %.2f", m_fRemainingDistance);
	ImGui::Text("Effect ID: %u", m_iProjectileEffectID);
	ImGui::Checkbox("Debug Draw", &m_bDebugDraw);
}

_bool CPlayer_Confringo_Bullet::SweepTo(
	const _float3& vStartPosition,
	const _float3& vTargetPosition,
	PX_SWEEP_RESULT& OutHit) const
{
	const _vector vDisplacement =
		XMLoadFloat3(&vTargetPosition) -
		XMLoadFloat3(&vStartPosition);
	const _float fDistance =
		XMVectorGetX(XMVector3Length(vDisplacement));
	if (fDistance <= FLT_EPSILON)
		return false;

	PX_SWEEP_DESC Desc{};
	Desc.tGeometry.eType = PX_QUERY_GEOMETRY_TYPE::SPHERE;
	Desc.tGeometry.fRadius = m_fRadius;
	Desc.tPose.vPosition = vStartPosition;
	XMStoreFloat3(
		&Desc.vDirection,
		XMVector3Normalize(vDisplacement));
	Desc.fMaxDistance = fDistance;
	Desc.tFilter = m_tQueryFilter;

	auto* pPhysXManager = CGameInstance::Get().GetPhysXManager();
	return pPhysXManager &&
		pPhysXManager->Sweep(Desc, OutHit) &&
		OutHit.bHit;
}

void CPlayer_Confringo_Bullet::BuildDynamicPath(
	const _float3& vStartPosition,
	const _float3& vEndPosition,
	_float fCurveAmplitude,
	_float fCurveFrequency,
	uint32_t iSampleCount)
{
	m_PathPoints.clear();
	iSampleCount = std::max(iSampleCount, 8u);

	const _vector vStart = XMLoadFloat3(&vStartPosition);
	const _vector vEnd = XMLoadFloat3(&vEndPosition);
	_vector vForward = vEnd - vStart;
	if (XMVectorGetX(XMVector3LengthSq(vForward)) <= FLT_EPSILON)
		return;

	vForward = XMVector3Normalize(vForward);
	_vector vRight = XMVector3Cross(
		XMVectorSet(0.f, 1.f, 0.f, 0.f),
		vForward);
	if (XMVectorGetX(XMVector3LengthSq(vRight)) <= FLT_EPSILON)
		vRight = XMVectorSet(1.f, 0.f, 0.f, 0.f);
	else
		vRight = XMVector3Normalize(vRight);

	const _vector vUp = XMVector3Normalize(
		XMVector3Cross(vForward, vRight));
	const _float fPhase = Randf(0.f, XM_2PI);
	const _float fSecondPhase = Randf(0.f, XM_2PI);

	m_PathPoints.reserve(iSampleCount + 1);
	for (uint32_t i = 0; i <= iSampleCount; ++i)
	{
		const _float fRatio =
			static_cast<_float>(i) / iSampleCount;
		const _float fEnvelope = std::sin(XM_PI * fRatio);
		const _float fHorizontalWave = std::sin(
			XM_2PI * fCurveFrequency * fRatio + fPhase);
		const _float fVerticalWave = std::sin(
			XM_2PI * fCurveFrequency * 0.73f * fRatio +
			fSecondPhase);

		const _vector vPosition =
			XMVectorLerp(vStart, vEnd, fRatio) +
			vRight * fHorizontalWave * fCurveAmplitude * fEnvelope +
			vUp * fVerticalWave * fCurveAmplitude * 0.45f * fEnvelope;

		_float3 vPathPoint{};
		XMStoreFloat3(&vPathPoint, vPosition);
		m_PathPoints.push_back(vPathPoint);
	}

	// 시작점과 목표점은 전달받은 위치에 정확히 고정한다.
	m_PathPoints.front() = vStartPosition;
	m_PathPoints.back() = vEndPosition;
}

void CPlayer_Confringo_Bullet::UpdateFlightTransform(
	const _float3& vPosition,
	const _float3& vDirection)
{
	GetTransform().SetPosition(vPosition);
	const _float4x4 tWorld = MakeOrientedWorld(vPosition, vDirection);
	GetTransform().SetQuaternion(
		XMQuaternionRotationMatrix(XMLoadFloat4x4(&tWorld)));
	GetTransform().Update();
}

void CPlayer_Confringo_Bullet::HandleHit(
	const PX_SWEEP_RESULT& Hit)
{
	if (m_bFinished)
		return;

	m_bFinished = true;

	const _float3 vCurrentPosition = GetTransform().GetPosition();
	_float3 vFinalPosition{};
	XMStoreFloat3(
		&vFinalPosition,
		XMLoadFloat3(&vCurrentPosition) +
		XMLoadFloat3(&m_vDirection) * Hit.fDistance);

	GetTransform().SetPosition(vFinalPosition);
	GetTransform().Update();
	EmitTrailBetween(vCurrentPosition, vFinalPosition);
	UpdateProjectileEffect();
	StopFlightSound();
	StopProjectileEffect();

	ProcessHitGameplay(Hit);
	PlayImpactEffect(Hit.vHitpos, Hit.vHitNormal);

	DEBUG_LOG_STR(
		std::string{ "[PX][CPlayer_Confringo_Bullet] Sweep Hit: " } +
		(Hit.pGameObject
			? std::string{ Hit.pGameObject->GetObjectTag() }
			: std::string{ "null" }) + "\n");

	SetPendingDestroy();
}

void CPlayer_Confringo_Bullet::ProcessHitGameplay(
	const PX_SWEEP_RESULT& Hit)
{
	// [LSY] 충돌 판정이 확정된 뒤 추가 게임 로직을 넣는 확장 지점이다.
	// 피해, 상태 이상, 넉백, 이벤트 통지는 이 함수에서 처리한다.
	// Hit.pGameObject는 이 함수 안에서만 사용하고, 보관이 필요하면
	// 수명 검증이 가능한 Hit.hGameObject를 저장한다.
	if (auto* pMonster = Cast<CMonster>(Hit.pGameObject))
		pMonster->Check_Table(m_eSkillType);
}

void CPlayer_Confringo_Bullet::FinishWithoutHit()
{
	if (m_bFinished)
		return;

	m_bFinished = true;
	StopFlightSound();
	StopProjectileEffect();
	SetPendingDestroy();
}

void CPlayer_Confringo_Bullet::FinishAtEndPosition()
{
	if (m_bFinished)
		return;

	m_bFinished = true;
	GetTransform().SetPosition(m_vEndPosition);
	GetTransform().Update();
	UpdateProjectileEffect();
	StopFlightSound();
	StopProjectileEffect();

	_float3 vImpactNormal{};
	XMStoreFloat3(
		&vImpactNormal,
		-XMVector3Normalize(XMLoadFloat3(&m_vDirection)));
	PlayImpactEffect(m_vEndPosition, vImpactNormal);
	SetPendingDestroy();
}

void CPlayer_Confringo_Bullet::StartProjectileEffect()
{
	if (m_iProjectileEffectID != INVALID_EFFECT_INSTANCE_ID ||
		m_sProjectileEffectName.empty())
	{
		return;
	}

	m_iProjectileEffectID = CGameInstance::Get().PlayEffect(
		m_sProjectileEffectName,
		*GetTransform().GetWorldMatrix(),
		XMVectorZero(),
		[hOwner = GetHandle()](
			EFFECT_INSTANCE_ID iEffectID,
			EFFECT_FINISH_REASON)
		{
			if (auto* pBullet = CGameInstance::Get().
				GetGameObjectByHandleT<CPlayer_Confringo_Bullet>(hOwner))
			{
				if (pBullet->m_iProjectileEffectID == iEffectID)
				{
					pBullet->m_iProjectileEffectID =
						INVALID_EFFECT_INSTANCE_ID;
				}
			}
		});
}

void CPlayer_Confringo_Bullet::StopProjectileEffect()
{
	if (m_iProjectileEffectID == INVALID_EFFECT_INSTANCE_ID)
		return;

	const EFFECT_INSTANCE_ID iEffectID = m_iProjectileEffectID;
	m_iProjectileEffectID = INVALID_EFFECT_INSTANCE_ID;
	CGameInstance::Get().StopEffect(iEffectID);
}

void CPlayer_Confringo_Bullet::UpdateProjectileEffect()
{
	if (m_iProjectileEffectID == INVALID_EFFECT_INSTANCE_ID)
		return;

	CGameInstance::Get().SetEffectWorldMatrix(
		m_iProjectileEffectID,
		*GetTransform().GetWorldMatrix());
}

void CPlayer_Confringo_Bullet::StartFlightSound()
{
	if (!m_pComSound)
		return;

	m_pComSound->PlaySlot3D(
		E::StringID{ "PLAYER_CONFRINGO_PROJECTILE" },
		"./Resources/SampleClient/Sound/Player/SkillEffect/Confringo/Confringo_Projectile.wav",
		SOUND_3D_DESC{
			.vPosition = GetTransform().GetPosition(),
			.fMinDistance = 1.f,
			.fMaxDistance = 60.f,
			.eRolloff = SOUND_3D_ROLLOFF::LINEAR
		},
		SOUND_PLAY_DESC{
			.sBusID = SOUND_BUS::SFX,
			.fVolume = 0.25f,
			.fPitch = 1.f,
			.iPriority = 76,
			.bLoop = false
		});
}

void CPlayer_Confringo_Bullet::StopFlightSound()
{
	if (m_pComSound)
		m_pComSound->StopSlot(E::StringID{ "PLAYER_CONFRINGO_PROJECTILE" });
}

void CPlayer_Confringo_Bullet::UpdateFlightSound()
{
	if (!m_pComSound || m_bFinished)
		return;

	m_pComSound->SetSlot3DAttributes(
		E::StringID{ "PLAYER_CONFRINGO_PROJECTILE" },
		GetTransform().GetPosition());
}

void CPlayer_Confringo_Bullet::PlayImpactSounds(
	const _float3& vPosition) const
{
	auto* pSoundManager = CGameInstance::Get().GetSoundManager();
	if (!pSoundManager)
		return;

	const SOUND_3D_DESC Sound3DDesc{
		.vPosition = vPosition,
		.fMinDistance = 2.f,
		.fMaxDistance = 80.f,
		.eRolloff = SOUND_3D_ROLLOFF::LINEAR
	};

	pSoundManager->Play3D(
		"./Resources/SampleClient/Sound/Player/SkillEffect/Confringo/Confringo_Impact_Main.wav",
		Sound3DDesc,
		SOUND_PLAY_DESC{
			.sBusID = SOUND_BUS::SFX,
			.fVolume = 0.55f,
			.fPitch = 1.f,
			.iPriority = 84,
			.bLoop = false
		});
	pSoundManager->Play3D(
		"./Resources/SampleClient/Sound/Player/SkillEffect/Confringo/Confringo_Impact_Tail.wav",
		Sound3DDesc,
		SOUND_PLAY_DESC{
			.sBusID = SOUND_BUS::SFX,
			.fVolume = 0.22f,
			.fPitch = 1.f,
			.iPriority = 78,
			.bLoop = false
		});
}

void CPlayer_Confringo_Bullet::EmitTrailBetween(
	const _float3& vPreviousPosition,
	const _float3& vCurrentPosition)
{
	if (m_sTrailParticleQueue.empty() || m_fTrailSpacing <= FLT_EPSILON)
		return;

	_vector vSegmentStart = XMLoadFloat3(&vPreviousPosition);
	const _vector vSegmentEnd = XMLoadFloat3(&vCurrentPosition);
	_vector vSegment = vSegmentEnd - vSegmentStart;
	_float fRemainingLength = XMVectorGetX(XMVector3Length(vSegment));
	if (fRemainingLength <= FLT_EPSILON)
		return;

	const _vector vSegmentDirection = XMVector3Normalize(vSegment);
	_float fDistanceToNext = m_fTrailSpacing - m_fTrailDistanceAccumulator;

	while (fDistanceToNext <= fRemainingLength)
	{
		vSegmentStart += vSegmentDirection * fDistanceToNext;

		_float3 vSpawnPosition{};
		XMStoreFloat3(&vSpawnPosition, vSegmentStart);
		SpawnTrailParticle(vSpawnPosition);

		fRemainingLength -= fDistanceToNext;
		m_fTrailDistanceAccumulator = 0.f;
		fDistanceToNext = m_fTrailSpacing;
	}

	m_fTrailDistanceAccumulator += fRemainingLength;
}

void CPlayer_Confringo_Bullet::SpawnTrailParticle(
	const _float3& vPosition) const
{
	const _float4x4 tTrailWorld =
		MakeOrientedWorld(vPosition, m_vDirection);

	CGameInstance::Get().Spawn(
		m_sTrailParticleQueue,
		tTrailWorld);
}

void CPlayer_Confringo_Bullet::PlayImpactEffect(
	const _float3& vPosition,
	const _float3& vNormal)
{
	// [LSY] 파편 오브젝트가 제거되어도 충돌음의 잔향은 끝까지 재생한다.
	PlayImpactSounds(vPosition);

	if (m_sImpactEffectName.empty())
		return;

	_float3 vForward = vNormal;
	if (XMVectorGetX(XMVector3LengthSq(XMLoadFloat3(&vForward))) <=
		FLT_EPSILON)
	{
		XMStoreFloat3(
			&vForward,
			-XMVector3Normalize(XMLoadFloat3(&m_vDirection)));
	}

	const _float4x4 tImpactWorld =
		MakeOrientedWorld(vPosition, vForward);
	CGameInstance::Get().PlayEffect(
		m_sImpactEffectName,
		tImpactWorld);
}

_float4x4 CPlayer_Confringo_Bullet::MakeOrientedWorld(
	const _float3& vPosition,
	const _float3& vForward)
{
	_vector vLook = XMLoadFloat3(&vForward);
	if (XMVectorGetX(XMVector3LengthSq(vLook)) <= FLT_EPSILON)
		vLook = XMVectorSet(0.f, 0.f, 1.f, 0.f);
	else
		vLook = XMVector3Normalize(vLook);

	_vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	_vector vRight = XMVector3Cross(vUp, vLook);
	if (XMVectorGetX(XMVector3LengthSq(vRight)) <= FLT_EPSILON)
	{
		vUp = XMVectorSet(0.f, 0.f, 1.f, 0.f);
		vRight = XMVector3Cross(vUp, vLook);
	}

	vRight = XMVector3Normalize(vRight);
	vUp = XMVector3Normalize(XMVector3Cross(vLook, vRight));

	_matrix matWorld = XMMatrixIdentity();
	matWorld.r[0] = XMVectorSetW(vRight, 0.f);
	matWorld.r[1] = XMVectorSetW(vUp, 0.f);
	matWorld.r[2] = XMVectorSetW(vLook, 0.f);
	matWorld.r[3] = XMVectorSetW(XMLoadFloat3(&vPosition), 1.f);

	_float4x4 tWorld{};
	XMStoreFloat4x4(&tWorld, matWorld);
	return tWorld;
}

UPtr<CPlayer_Confringo_Bullet> CPlayer_Confringo_Bullet::Create()
{
	auto pInstance = ToUPtr(new CPlayer_Confringo_Bullet{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Create: CPlayer_Confringo_Bullet");
		return nullptr;
	}

	return pInstance;
}

UPtr<CPrototype> CPlayer_Confringo_Bullet::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CPlayer_Confringo_Bullet{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Clone: CPlayer_Confringo_Bullet");
		return nullptr;
	}

	return pInstance;
}
