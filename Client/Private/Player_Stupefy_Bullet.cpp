#include "pch.h"
#include "Player_Stupefy_Bullet.h"
#include "ClientEvents.h"
#include "DbgLineRender.h"
#include "Monster.h"
#include "PhysXManager.h"

NS_USING(Client)

CPlayer_Stupefy_Bullet::~CPlayer_Stupefy_Bullet()
{
	StopCoreEffect();
}

HRESULT CPlayer_Stupefy_Bullet::Initialize(void* pArg)
{
	const auto* desc = static_cast<const DESC*>(pArg);
	if (!desc || FAILED(CGameObject::Initialize(pArg)) || desc->fSpeed <= 0.f ||
		desc->fLifeTime <= 0.f || desc->fRadius <= 0.f)
		return E_INVALIDARG;
	m_fSpeed = desc->fSpeed;
	m_fLifeTime = desc->fLifeTime;
	m_fRadius = desc->fRadius;
	m_fTrailSpacing = std::max(0.01f, desc->fTrailSpacing);
	m_hOwner = desc->hOwner;
	m_eSkillType = desc->eSkillType;
	m_sCoreEffect = desc->sProjectileEffectName;
	m_sTrailQueue = desc->sTrailParticleQueue;
	m_sImpactEffect = desc->sImpactEffectName;
	m_bDebugSphere = desc->bDebugSphere;
	m_bDebugPath = desc->bDebugPath;
	m_QueryFilter = desc->tQueryFilter;
	m_QueryFilter.hIgnoreGameObject = m_hOwner;
	BuildPath(*desc);
	if (m_Path.size() < 2) return E_INVALIDARG;
	for (size_t i = 1; i < m_Path.size(); ++i)
		m_fRemainingDistance += XMVectorGetX(XMVector3Length(XMLoadFloat3(&m_Path[i]) - XMLoadFloat3(&m_Path[i - 1])));
	XMStoreFloat3(&m_vDirection, XMVector3Normalize(XMLoadFloat3(&m_Path[1]) - XMLoadFloat3(&m_Path[0])));
	SetFlightTransform(m_Path[0], m_vDirection);
	return S_OK;
}

void CPlayer_Stupefy_Bullet::OnRegisteredToManager()
{
	if (m_sCoreEffect.empty()) return;
	m_iCoreEffect = CGameInstance::Get().PlayEffect(m_sCoreEffect,
		*GetTransform().GetWorldMatrix(), XMVectorZero());
}

void CPlayer_Stupefy_Bullet::FixedUpdate(_float dt)
{
	if (m_bFinished || dt <= 0.f) return;
	m_fElapsed += dt;
	if (m_fElapsed >= m_fLifeTime) { StopCoreEffect(); SetPendingDestroy(); m_bFinished = true; return; }
	_float budget = std::min(m_fSpeed * dt, m_fRemainingDistance);
	while (budget > FLT_EPSILON && m_iSegment + 1 < m_Path.size())
	{
		const _vector a = XMLoadFloat3(&m_Path[m_iSegment]);
		const _vector b = XMLoadFloat3(&m_Path[m_iSegment + 1]);
		const _vector segment = b - a;
		const _float length = XMVectorGetX(XMVector3Length(segment));
		if (length <= FLT_EPSILON) { ++m_iSegment; m_fSegmentDistance = 0.f; continue; }
		const _float move = std::min(budget, length - m_fSegmentDistance);
		_float3 start{}, end{};
		XMStoreFloat3(&start, XMVectorLerp(a, b, m_fSegmentDistance / length));
		XMStoreFloat3(&end, XMVectorLerp(a, b, (m_fSegmentDistance + move) / length));
		XMStoreFloat3(&m_vDirection, XMVector3Normalize(segment));
		PX_SWEEP_RESULT hit{};
		if (SweepTo(start, end, hit)) { Finish(hit.vHitpos, hit.vHitNormal, hit.pGameObject); return; }
		SetFlightTransform(end, m_vDirection);
		EmitTrail(start, end);
		if (m_iCoreEffect != INVALID_EFFECT_INSTANCE_ID)
			CGameInstance::Get().SetEffectWorldMatrix(m_iCoreEffect, *GetTransform().GetWorldMatrix());
		m_fSegmentDistance += move;
		m_fRemainingDistance -= move;
		budget -= move;
		if (m_fSegmentDistance >= length - FLT_EPSILON) { ++m_iSegment; m_fSegmentDistance = 0.f; }
	}
	if (m_fRemainingDistance <= FLT_EPSILON || m_iSegment + 1 >= m_Path.size())
		Finish(m_Path.back(), { -m_vDirection.x, -m_vDirection.y, -m_vDirection.z }, nullptr);
}

void CPlayer_Stupefy_Bullet::LateUpdate(_float)
{
	GetTransform().Update();
	if (!m_bDebugSphere && !m_bDebugPath) return;
	auto* debug = CGameInstance::Get().GetDbgLineRender();
	if (!debug) return;

	const _float4 previousColor = debug->GetColor();
	const DBG_LINE_DEPTH_MODE previousDepth = debug->GetDepthMode();
	debug->SetDepthTest(false);

	if (m_bDebugSphere)
	{
		// 실제 이펙트와 무관한 충돌 위치 확인용 디버그 구다.
		debug->SetColor({ 0.82f, 0.92f, 1.f, 1.f });
		debug->AddSphere(std::max(m_fRadius * 2.4f, 0.32f),
			GetTransform().GetLoadedWorldMatrix());
		debug->SetColor({ 1.f, 1.f, 1.f, 1.f });
		debug->AddSphere(std::max(m_fRadius * 1.25f, 0.18f),
			GetTransform().GetLoadedWorldMatrix());
	}

	if (m_bDebugPath)
	{
		const _float3 position = GetTransform().GetPosition();
		const _float3 tailEnd{
			position.x - m_vDirection.x * 1.5f,
			position.y - m_vDirection.y * 1.5f,
			position.z - m_vDirection.z * 1.5f
		};
		debug->AddLine(tailEnd, position, { 1.f, 1.f, 1.f, 1.f });
		for (size_t i = 1; i < m_Path.size(); ++i)
			debug->AddLine(m_Path[i - 1], m_Path[i], { 1.f, 1.f, 1.f, 0.45f });
	}

	debug->SetColor(previousColor);
	debug->SetDepthMode(previousDepth);
}

void CPlayer_Stupefy_Bullet::UpdateGUI()
{
	CGameObject::UpdateGUI();
	ImGui::Text("Stupefy Projectile");
	ImGui::Text("Speed %.1f | Radius %.3f", m_fSpeed, m_fRadius);
	ImGui::Text("Life %.2f / %.2f", m_fElapsed, m_fLifeTime);
	ImGui::Text("Remaining %.2f", m_fRemainingDistance);
	ImGui::Checkbox("Debug Sphere", &m_bDebugSphere);
	ImGui::Checkbox("Debug Path", &m_bDebugPath);
}

_bool CPlayer_Stupefy_Bullet::SweepTo(const _float3& start, const _float3& end, PX_SWEEP_RESULT& hit) const
{
	const _vector delta = XMLoadFloat3(&end) - XMLoadFloat3(&start);
	const _float distance = XMVectorGetX(XMVector3Length(delta));
	if (distance <= FLT_EPSILON) return false;
	PX_SWEEP_DESC desc{};
	desc.tGeometry.eType = PX_QUERY_GEOMETRY_TYPE::SPHERE;
	desc.tGeometry.fRadius = m_fRadius;
	desc.tPose.vPosition = start;
	XMStoreFloat3(&desc.vDirection, XMVector3Normalize(delta));
	desc.fMaxDistance = distance;
	desc.tFilter = m_QueryFilter;
	auto* physx = CGameInstance::Get().GetPhysXManager();
	return physx && physx->Sweep(desc, hit) && hit.bHit;
}

void CPlayer_Stupefy_Bullet::BuildPath(const DESC& desc)
{
	const uint32_t count = std::max(8u, desc.iPathSampleCount);
	const _vector start = XMLoadFloat3(&desc.vStartPosition);
	const _vector end = XMLoadFloat3(&desc.vEndPosition);
	_vector forward = XMVector3Normalize(end - start);
	_vector right = XMVector3Cross(XMVectorSet(0.f, 1.f, 0.f, 0.f), forward);
	if (XMVectorGetX(XMVector3LengthSq(right)) <= FLT_EPSILON) right = XMVectorSet(1.f, 0.f, 0.f, 0.f);
	else right = XMVector3Normalize(right);
	const _vector up = XMVector3Normalize(XMVector3Cross(forward, right));
	m_Path.reserve(count + 1);
	for (uint32_t i = 0; i <= count; ++i)
	{
		const _float t = static_cast<_float>(i) / count;
		const _float envelope = std::sin(XM_PI * t);
		const _float wave = std::sin(XM_2PI * desc.fCurveFrequency * t);
		_float3 point{};
		XMStoreFloat3(&point, XMVectorLerp(start, end, t) + right * wave * desc.fCurveAmplitude * envelope + up * wave * desc.fCurveAmplitude * 0.35f * envelope);
		m_Path.push_back(point);
	}
	m_Path.front() = desc.vStartPosition;
	m_Path.back() = desc.vEndPosition;
}

void CPlayer_Stupefy_Bullet::SetFlightTransform(const _float3& position, const _float3& direction)
{
	GetTransform().SetPosition(position);
	_vector look = XMVector3Normalize(XMLoadFloat3(&direction));
	_vector right = XMVector3Cross(XMVectorSet(0.f, 1.f, 0.f, 0.f), look);
	if (XMVectorGetX(XMVector3LengthSq(right)) <= FLT_EPSILON) right = XMVectorSet(1.f, 0.f, 0.f, 0.f);
	right = XMVector3Normalize(right);
	const _vector up = XMVector3Normalize(XMVector3Cross(look, right));
	_matrix world{ XMVectorSetW(right, 0.f), XMVectorSetW(up, 0.f), XMVectorSetW(look, 0.f), XMVectorSetW(XMLoadFloat3(&position), 1.f) };
	GetTransform().SetQuaternion(XMQuaternionRotationMatrix(world));
	GetTransform().Update();
}

void CPlayer_Stupefy_Bullet::EmitTrail(const _float3& start, const _float3& end)
{
	if (m_sTrailQueue.empty()) return;
	const _vector center = XMLoadFloat3(&end);
	_vector look = XMVector3Normalize(XMLoadFloat3(&m_vDirection));
	_vector widthDir = XMVector3Cross(XMVectorSet(0.f, 1.f, 0.f, 0.f), look);
	if (XMVectorGetX(XMVector3LengthSq(widthDir)) <= FLT_EPSILON)
		widthDir = XMVectorSet(1.f, 0.f, 0.f, 0.f);
	else
		widthDir = XMVector3Normalize(widthDir);

	constexpr _float TRAIL_GROW_TIME = 0.09f;
	const _float growRatio = std::clamp(m_fElapsed / TRAIL_GROW_TIME, 0.f, 1.f);
	const _float halfWidth = 0.59f * growRatio;
	_float3 ribbonStart{}, ribbonEnd{};
	XMStoreFloat3(&ribbonStart, center - widthDir * halfWidth);
	XMStoreFloat3(&ribbonEnd, center + widthDir * halfWidth);
	CGameInstance::Get().AddTrailPoint(m_sTrailQueue, m_sTrailQueue,
		GetHandle(), ribbonStart, ribbonEnd);
}

void CPlayer_Stupefy_Bullet::Finish(const _float3& position, const _float3& normal, CGameObject* hitObject)
{
	if (m_bFinished) return;
	m_bFinished = true;
	SetFlightTransform(position, m_vDirection);
	StopCoreEffect();
	if (!m_sImpactEffect.empty())
	{
		_vector vLook = XMLoadFloat3(&normal);
		if (XMVectorGetX(XMVector3LengthSq(vLook)) <= FLT_EPSILON)
			vLook = -XMVector3Normalize(XMLoadFloat3(&m_vDirection));
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

		_matrix impactWorld = XMMatrixIdentity();
		impactWorld.r[0] = XMVectorSetW(vRight, 0.f);
		impactWorld.r[1] = XMVectorSetW(vUp, 0.f);
		impactWorld.r[2] = XMVectorSetW(vLook, 0.f);
		impactWorld.r[3] = XMVectorSetW(XMLoadFloat3(&position) + vLook * 0.025f, 1.f);

		_float4x4 world{};
		XMStoreFloat4x4(&world, impactWorld);
		CGameInstance::Get().PlayEffect(m_sImpactEffect, world, vLook);
	}
	// Impact Sound
	if (auto* pSoundManager = CGameInstance::Get().GetSoundManager())
	{
		SOUND_3D_DESC Sound3DDesc{
			.vPosition = position,
			.fMinDistance = 2.f,
			.fMaxDistance = 80.f,
			.eRolloff = SOUND_3D_ROLLOFF::LINEAR
		};

		pSoundManager->Play3D(
			"./Resources/SampleClient/Sound/Player/SkillEffect/Stupefy/Stupefy_Impact.wav",
			Sound3DDesc,
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::SFX,
				.fVolume = 0.55f,
				.fPitch = 1.f,
				.iPriority = 84,
				.bLoop = false
			});
	}
	CGameInstance::Get().EventPublish(FRequestPlayerCameraShake{
		.fIntensity = 0.7f,
		.fDuration = 1.f,
		.fFrequency = 40.f });
	if (auto* monster = Cast<CMonster>(hitObject))
		monster->Check_Table(m_eSkillType);
	SetPendingDestroy();
}

void CPlayer_Stupefy_Bullet::StopCoreEffect()
{
	if (m_iCoreEffect == INVALID_EFFECT_INSTANCE_ID) return;
	const auto id = m_iCoreEffect;
	m_iCoreEffect = INVALID_EFFECT_INSTANCE_ID;
	CGameInstance::Get().StopEffect(id);
}

UPtr<CPlayer_Stupefy_Bullet> CPlayer_Stupefy_Bullet::Create()
{
	auto instance = ToUPtr(new CPlayer_Stupefy_Bullet{});
	if (FAILED(instance->InitializePrototype())) return nullptr;
	return instance;
}

UPtr<CPrototype> CPlayer_Stupefy_Bullet::Clone(void* pArg)
{
	auto instance = ToUPtr(new CPlayer_Stupefy_Bullet{ *this });
	if (FAILED(instance->Initialize(pArg))) return nullptr;
	return instance;
}
