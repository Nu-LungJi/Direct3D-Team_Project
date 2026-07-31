#include "pch.h"
#include "Player_Magic_Bullet.h"
#include "Client_Resources.h"
#include "Trail_CPU.h"
#include "ComPxSphereCollider.h"
#include "ComPxRigidBody.h"
#include "DbgLineRender.h"

#include "TmbGurdian.h"
#include "BossTMB.h"
NS_USING(Client)

CPlayer_Magic_Bullet::CPlayer_Magic_Bullet()
	: CGameObject{}
{
}

CPlayer_Magic_Bullet::~CPlayer_Magic_Bullet()
{
}

void CPlayer_Magic_Bullet::UpdateGUI()
{
	CGameObject::UpdateGUI();


}

HRESULT CPlayer_Magic_Bullet::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CPlayer_Magic_Bullet::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	const auto pDesc = static_cast<const MAGIC_BULLET_DESC*>(pArg);
	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	m_vStartPosition = pDesc->vStartPosition;
	m_vEndPosition = pDesc->vEndPosition;
	m_fSpeed = pDesc->fSpeed;
	m_fRadius = pDesc->fRadius;
	BuildSpline(pDesc->fCurveHeight, pDesc->iSampleCount);
	if (m_Splines.empty())
		return E_FAIL;

	{
		CComPxRigidBody::DESC Desc{};
		Desc.eType = CComPxRigidBody::TYPE::KINEMATIC;

		Desc.vPosition = pDesc->vStartPosition;

		if (FAILED(AddComponentFromProto("PHYSX", "Prototype_Component_ComPxRigidBody", "ComPxRigidBody", &Desc, &m_pComPxRigidBody)))
		{
			return E_FAIL;
		};
	}
	//m_pComPxRigidBody->SetKinematicTarget()

	{
		CComPxSphereCollider::DESC Desc{};
		Desc.pComPxRigidBody = m_pComPxRigidBody;
		//이거 나중에 하나 미리 만들어놓고 가져오는 걸로 (캐싱해서)
		Desc.pResSphereGeo = CResPhysXSphereGeometry::CreateAndLoad({ .fRadius = pDesc->fRadius });
		Desc.pResMaterial = CGameInstance::Get().GetResourceFirst<CResPhysXMaterial>("CLIENT_PX", "TMP_MATERIAL");
		Desc.bIsTrigger = true;
		Desc.tFilter = pDesc->tFilter;
		if (FAILED(AddComponentFromProto("PHYSX", "Prototype_Component_ComPxSphereCollider", "ComPxShpereCollider", &Desc, &m_pComPxShpereCollider)))
		{
			return E_FAIL;
		};
	}

	//if (!m_pComPxRigidBody->SetLinearVelocity(_float3(pDesc->fSpeed, pDesc->fSpeed, pDesc->fSpeed)))
		//return E_FAIL;
	//if (!m_pComPxRigidBody->SetGravityEnabled(false))
	//	return E_FAIL;


	//GetTransform().SetPosition(m_Splines.front());
	m_pComPxRigidBody->SetKinematicTarget(m_Splines.front(), GetTransform().GetQuaternion());

	GetTransform().SetPosition(m_pComPxRigidBody->GetPosition());
	return S_OK;
}

void CPlayer_Magic_Bullet::PriorityUpdate(E::_float fTimeDelta)
{
}

void CPlayer_Magic_Bullet::Update(E::_float fTimeDelta)
{
	if (m_Splines.size() < 2)
		return;

	_float fRemainDistance = m_fSpeed * fTimeDelta;


	

	while (fRemainDistance > 0.f && m_iSplineIndex < m_Splines.size() - 1)
	{
		const _vector vCurrent = XMLoadFloat3(&m_Splines[m_iSplineIndex]);
		const _vector vNext = XMLoadFloat3(&m_Splines[m_iSplineIndex + 1]);
		const _float fSegmentLength = XMVectorGetX(XMVector3Length(vNext - vCurrent));
		const _float fSegmentRemain = fSegmentLength - m_fDistanceOnSegment;

		if (fSegmentLength <= 0.0001f)
		{
			++m_iSplineIndex;
			m_fDistanceOnSegment = 0.f;
			continue;
		}

		if (fRemainDistance >= fSegmentRemain)
		{
			fRemainDistance -= fSegmentRemain;
			++m_iSplineIndex;
			m_fDistanceOnSegment = 0.f;
			//GetTransform().SetPosition(m_Splines[m_iSplineIndex]);
			m_pComPxRigidBody->SetKinematicTarget(m_Splines[m_iSplineIndex], GetTransform().GetQuaternion());
		}
		else
		{
			m_fDistanceOnSegment += fRemainDistance;
			const _float fRatio = m_fDistanceOnSegment / fSegmentLength;
			//GetTransform().SetPosition(XMVectorLerp(vCurrent, vNext, fRatio));

			_float3 tmpPos{};
			XMStoreFloat3(&tmpPos, XMVectorLerp(vCurrent, vNext, fRatio));
			m_pComPxRigidBody->SetKinematicTarget(tmpPos, GetTransform().GetQuaternion());
			fRemainDistance = 0.f;
		}
	}
	{
		_float3 vstart, vend;

		vstart = _float3(m_pComTransform->GetPosition().x, m_pComTransform->GetPosition().y + 0.4f, m_pComTransform->GetPosition().z);
		vend = _float3(m_pComTransform->GetPosition().x, m_pComTransform->GetPosition().y - 0.4f, m_pComTransform->GetPosition().z);
		CGameInstance::Get().AddTrailPoint("PlayerAttackTrail_CPU", "PlayerAttackTrail_CPU", vstart, vend);
	}

	const _float3 vCurrentPosition = m_pComPxRigidBody->GetPosition();
	const _vector vCurrent = XMLoadFloat3(&vCurrentPosition);
	const _vector vEnd = XMLoadFloat3(&m_vEndPosition);

	const _float fDistanceToEnd = XMVectorGetX(XMVector3Length(vEnd - vCurrent));

	if (m_iSplineIndex >= m_Splines.size() - 1 &&
		fDistanceToEnd <= 0.001f)
	{
		SetPendingDestroy();
	}
}

void CPlayer_Magic_Bullet::LateUpdate(E::_float fTimeDelta)
{
	GetTransform().SetPosition(m_pComPxRigidBody->GetPosition());
	GetTransform().Update();

}

HRESULT CPlayer_Magic_Bullet::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	return S_OK;
}

void CPlayer_Magic_Bullet::OnCollisionEnter(CGameObject* pObj, const PX_ON_COLLISION_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CPlayer_Magic_Bullet] Collision Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

void CPlayer_Magic_Bullet::OnCollisionExit(CGameObject* pObj, const PX_ON_COLLISION_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CPlayer_Magic_Bullet] Collision Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

void CPlayer_Magic_Bullet::OnTriggerEnter(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CPlayer_Magic_Bullet] Trigger Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");


	CGameInstance::Get().PlayEffect("PlayerAttackSpread", *m_pComTransform->GetWorldMatrix());

	if (auto pGuridan = Cast<CTmbGurdian>(pObj))
	{
		static constexpr const char* HIT_SOUND_PATHS[] =
		{
			"./Resources/SampleClient/Sound/boss_hit_01.wav",
			"./Resources/SampleClient/Sound/boss_hit_02.wav",
			"./Resources/SampleClient/Sound/boss_hit_03.wav",
			"./Resources/SampleClient/Sound/boss_hit_04.wav"
		};
		constexpr int HIT_SOUND_COUNT = static_cast<int>(sizeof(HIT_SOUND_PATHS) / sizeof(HIT_SOUND_PATHS[0]));
		const int iSoundIndex = Engine::RandInt(0, HIT_SOUND_COUNT - 1);

		auto id = CGameInstance::Get().GetSoundManager()->Play3D(
			HIT_SOUND_PATHS[iSoundIndex],
			SOUND_3D_DESC{
				.vPosition = GetTransform().GetPosition(),
				.fMinDistance = 10.f,
				.fMaxDistance = 30.f,
				.eRolloff = SOUND_3D_ROLLOFF::LINEAR
			},
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::SFX,
				.fVolume = 1.f,
				.fPitch = 1.f,
				.iPriority = 64,
				.bLoop = false
			}
		);
		if (id == INVALID_SOUND_ID)
		{
			MSG_BOX("INVALID_SOUND_ID");
		}
		//auto id = m_pComSound->PlaySlot3D(
		//	TEST_SLOT,
		//	"./Resources/SampleClient/Sound/avada.wav",
		//	SOUND_3D_DESC{
		//		.vPosition = GetTransform().GetPosition(),
		//		.fMinDistance = SOUND_MIN_DISTANCE,
		//		.fMaxDistance = 30.f,
		//		.eRolloff = SOUND_3D_ROLLOFF::LINEAR
		//	},
		//	SOUND_PLAY_DESC{
		//		.sBusID = SOUND_BUS::VOICE,
		//		.fVolume = 1.f,
		//		.fPitch = 1.f,
		//		.iPriority = 64,
		//		.bLoop = false
		//	});

		//if (id == INVALID_SOUND_ID)
		//{
		//	MSG_BOX("INVALID_SOUND_ID");
		//}

	}

	if (auto pBoss = Cast<CBossTMB>(pObj))
	{
		static constexpr const char* HIT_SOUND_PATHS[] =
		{
			"./Resources/SampleClient/Sound/boss_hit_01.wav",
			"./Resources/SampleClient/Sound/boss_hit_02.wav",
			"./Resources/SampleClient/Sound/boss_hit_03.wav",
			"./Resources/SampleClient/Sound/boss_hit_04.wav"
		};
		constexpr int HIT_SOUND_COUNT = static_cast<int>(sizeof(HIT_SOUND_PATHS) / sizeof(HIT_SOUND_PATHS[0]));
		const int iSoundIndex = Engine::RandInt(0, HIT_SOUND_COUNT - 1);

		auto id = CGameInstance::Get().GetSoundManager()->Play3D(
			HIT_SOUND_PATHS[iSoundIndex],
			SOUND_3D_DESC{
				.vPosition = GetTransform().GetPosition(),
				.fMinDistance = 10.f,
				.fMaxDistance = 30.f,
				.eRolloff = SOUND_3D_ROLLOFF::LINEAR
			},
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::SFX,
				.fVolume = 1.f,
				.fPitch = 1.f,
				.iPriority = 64,
				.bLoop = false
			}
		);
		if (id == INVALID_SOUND_ID)
		{
			MSG_BOX("INVALID_SOUND_ID");
		}
		//auto id = m_pComSound->PlaySlot3D(
		//	TEST_SLOT,
		//	"./Resources/SampleClient/Sound/avada.wav",
		//	SOUND_3D_DESC{
		//		.vPosition = GetTransform().GetPosition(),
		//		.fMinDistance = SOUND_MIN_DISTANCE,
		//		.fMaxDistance = 30.f,
		//		.eRolloff = SOUND_3D_ROLLOFF::LINEAR
		//	},
		//	SOUND_PLAY_DESC{
		//		.sBusID = SOUND_BUS::VOICE,
		//		.fVolume = 1.f,
		//		.fPitch = 1.f,
		//		.iPriority = 64,
		//		.bLoop = false
		//	});

		//if (id == INVALID_SOUND_ID)
		//{
		//	MSG_BOX("INVALID_SOUND_ID");
		//}

	}

}

void CPlayer_Magic_Bullet::OnTriggerExit(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CPlayer_Magic_Bullet] Trigger Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

void CPlayer_Magic_Bullet::BuildSpline(_float fCurveHeight, uint32_t iSampleCount)
{
	m_Splines.clear();
	m_iSplineIndex = 0;
	m_fDistanceOnSegment = 0.f;

	iSampleCount = std::max(iSampleCount, 3u);

	const _vector vStart = XMLoadFloat3(&m_vStartPosition);
	const _vector vEnd = XMLoadFloat3(&m_vEndPosition);

	_vector vForward = vEnd - vStart;
	const _float fDistance = XMVectorGetX(XMVector3Length(vForward));

	if (fDistance <= 0.0001f)
		return;

	vForward = XMVector3Normalize(vForward);

	// 진행 방향과 수직인 축 생성
	_vector vRight = XMVector3Cross(XMVectorSet(0.f, 1.f, 0.f, 0.f), vForward);

	if (XMVectorGetX(XMVector3LengthSq(vRight)) <= 0.0001f)
		vRight = XMVectorSet(1.f, 0.f, 0.f, 0.f);
	else
		vRight = XMVector3Normalize(vRight);

	const _vector vUp = XMVector3Normalize(XMVector3Cross(vForward, vRight));

	// 발사할 때 한 번만 랜덤 결정
	const _float fPhase = XMConvertToRadians((_float)(rand() % 360));

	const _float fFrequency = 1.5f + (_float)(rand() % 150) / 100.f;

	const _float fRightWeight = (rand() % 2 == 0) ? 1.f : -1.f;

	m_Splines.reserve(iSampleCount + 1);

	for (uint32_t i = 0; i <= iSampleCount; ++i)
	{
		const _float t = static_cast<_float>(i) / iSampleCount;

		// 기본 직선 위치
		_vector vPosition = XMVectorLerp(vStart, vEnd, t);

		// 시작점과 도착점에서 흔들림이 반드시 0이 되게 함
		const _float fEnvelope = std::sin(XM_PI * t);

		const _float fWave = std::sin(XM_2PI * fFrequency * t + fPhase);

		const _float fSecondWave = std::sin(XM_2PI * (fFrequency * 0.7f) * t + fPhase * 0.5f);

		vPosition += vRight * fWave * fCurveHeight * fEnvelope * fRightWeight;

		vPosition += vUp * fSecondWave * fCurveHeight * 0.5f * fEnvelope;

		_float3 vStoredPosition{};
		XMStoreFloat3(&vStoredPosition, vPosition);
		m_Splines.push_back(vStoredPosition);
	}
}

E::UPtr<CPlayer_Magic_Bullet> CPlayer_Magic_Bullet::Create()
{
	auto pInstance = E::ToUPtr(new CPlayer_Magic_Bullet{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CPlayer_Magic_Bullet");
		return nullptr;
	}
	return pInstance;
}

E::UPtr<E::CPrototype> CPlayer_Magic_Bullet::Clone(void* pArg)
{
	auto pInstance = E::ToUPtr(new CPlayer_Magic_Bullet{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CPlayer_Magic_Bullet");
		return nullptr;
	}

	return pInstance;
}
