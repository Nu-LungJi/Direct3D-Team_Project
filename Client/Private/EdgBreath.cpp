#include "pch.h"
#include "EdgBreath.h"
#include "PhysXManager.h"
NS_USING(Client)
CEdgBreath::CEdgBreath()
{
}

CEdgBreath::CEdgBreath(const CEdgBreath& rhs) : CDragonSkill(rhs)
{
}

CEdgBreath::~CEdgBreath()
{
}

HRESULT CEdgBreath::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CEdgBreath::Initialize(void* pArg)
{
	auto pDesc = static_cast<EDG_SKILL_DESC*>(pArg);
	pDesc->tQueryFilter.bQueryStatic = false;
	if (FAILED(__super::Initialize(pArg)))
	{
		MSG_BOX("Create Failed EDG Breath");
		return E_FAIL;
	}
	m_fDamage = 3.f;
	m_fRadius = 1.2f;
	m_fMaxLife = 2.f;
	m_fMaxBreath = 60.f;
	return S_OK;
}

void CEdgBreath::PriorityUpdate(E::_float fTimeDelta)
{
	__super::PriorityUpdate(fTimeDelta);

	if (!m_bActive) return;

	Life_Check(fTimeDelta);
}

void CEdgBreath::FixedUpdate(E::_float fTimeDelta)
{
	if (!m_bActive) return;

	__super::FixedUpdate(fTimeDelta);
	MoveBreath(fTimeDelta);
}

void CEdgBreath::Update(E::_float fTimeDelta)
{
	if (!m_bActive) return;

	__super::Update(fTimeDelta);
}

void CEdgBreath::LateUpdate(E::_float fTimeDelta)
{
	if (!m_bActive) return;

	__super::LateUpdate(fTimeDelta);
}

void CEdgBreath::Active(EDG_ACSKT_DESC& SkillTable, _vector vOffsetPos)
{
	m_eType = SkillTable.eType;
	auto pSrc = Get_Owner();
	if (nullptr == pSrc) return;
	auto pDest = pSrc->Get_Target();
	if (nullptr == pDest) return;

	 _float4x4 BoneMatrix = Get_BoneMatrix(m_iBoneIndex);
	 _matrix matBone = XMLoadFloat4x4(&BoneMatrix);
	_vector vDestPos = XMLoadFloat3(&pDest->GetTransform().GetPosition());


	_vector vLength = vDestPos - matBone.r[3];
	_vector vDir = XMVector3Normalize(vLength);
	_float fHalfDis = XMVectorGetX(XMVector3Length(vLength)) * 0.5f;

	m_fBreathDis = 0.f;
	if (m_eType == DRAGON_SKILL::BREATH || m_eType == DRAGON_SKILL::GASIBREATH)
	{
		XMStoreFloat3(&m_vTargetDir, vDir);
		XMStoreFloat3(&m_vDir, XMVector3Normalize((matBone.r[3] + vDir * fHalfDis) - matBone.r[3]));


	}
	else if (m_eType == DRAGON_SKILL::TURNBREATH)
	{

	}
	else if (m_eType == DRAGON_SKILL::LONGBREATH)
	{
		_float4x4 matOffB = Get_BoneMatrix(m_iOffsetBoneIdex);
		_matrix matOffset = XMLoadFloat4x4(&matOffB);

		XMStoreFloat3(&m_vDir, XMVector3Normalize(matOffset.r[3] - matBone.r[3]));
	}
	
	GetTransform().SetPosition(matBone.r[3]);
	GetTransform().SetQuaternion(XMQuaternionRotationMatrix(matBone));
	
	m_bActive = true;
	m_bGround = m_bHit = false;
	m_fBreathTick = m_fLife = 0.f;
	m_fMaxLife = SkillTable.fLifeTime;
	m_fBreathParticleTick = 0.f;
	m_fGroundParticleTick = 0.f;
	Spawn_Skill_Effect(SkillTable.SkillName);

	m_iBreathSoundID  = E::CGameInstance::Get().GetSoundManager()->Play2D("./Resources/SampleClient/Sound/LastBossRanrok/Ambient/Breath.wav", SOUND_PLAY_DESC{
	.sBusID = SOUND_BUS::SFX,
	.fVolume = 0.7f,
	.fPitch = 1.f,
	.iPriority = 64,
	.bLoop = false
		});

	CGameInstance::Get().Spawn("BreathReady.json", BoneMatrix);
}

void CEdgBreath::Cancle()
{
	ResetValue();

	auto pSoundManager = E::CGameInstance::Get().GetSoundManager();

	if (pSoundManager && m_iBreathSoundID != INVALID_SOUND_ID)
	{
		pSoundManager->Stop(m_iBreathSoundID);
		m_iBreathSoundID = INVALID_SOUND_ID;
	}

}

void CEdgBreath::SpawnGasi(_vector vPos, _vector vDirection)
{		
	auto pSrc = Get_Owner();
	if (nullptr == pSrc)
		return;

	const _string SkillName = pSrc->Get_SkillNmae(DRAGON_SKILL::GASI);
	auto Table = pSrc->Get_SkillInfo(DRAGON_SKILL::GASI);

	if (SkillName.empty())
		return;

	auto pSkill = CGameInstance::Get().GetGameObjectByHandleT<CDragonSkill>(Table.handle);
	if (nullptr == pSkill)
		return;

	vDirection = XMVectorSetY(vDirection, 0.f);

	if (XMVectorGetX(XMVector3LengthSq(vDirection)) <= FLT_EPSILON)
		vDirection = XMVectorSet(0.f, 0.f, 1.f, 0.f);
	else
		vDirection = XMVector3Normalize(vDirection);

	const _float fYaw = atan2f(XMVectorGetX(vDirection), XMVectorGetZ(vDirection));
	const _vector vQuaternion = XMQuaternionRotationRollPitchYaw(0.f, fYaw, 0.f);

	pSkill->GetTransform().SetQuaternion(vQuaternion);

	EDG_ACSKT_DESC ACTable{};
	ACTable.SkillName = SkillName;
	ACTable.fLifeTime = 3.f;
	ACTable.eType = DRAGON_SKILL::GASI;

	pSkill->Active(ACTable, vPos);
}

void CEdgBreath::MoveBreath(_float fTimeDelta)
{
	 _float4x4 BoneMatrix = Get_BoneMatrix(m_iBoneIndex);
	_matrix matBone = XMLoadFloat4x4(&BoneMatrix);
	_vector vQuat = XMQuaternionRotationMatrix(matBone);

	m_fBreathTick += fTimeDelta;
	_vector vForward{};
	_float t = std::min(m_fBreathTick / 3.f, 1.f);
	_float tBreath = std::min(m_fBreathTick / 1.f, 1.f);

	_float tLBreath = std::min(m_fBreathTick / 3.f, 1.f);
	if (m_eType == DRAGON_SKILL::BREATH)
	{
		vForward = XMVector3Normalize(XMVectorLerp(XMLoadFloat3(&m_vDir), XMLoadFloat3(&m_vTargetDir), t));
	}else if (m_eType == DRAGON_SKILL::GASIBREATH)
	{
		_float4x4 matOffB = Get_BoneMatrix(m_iOffsetBoneIdex);
		_matrix matOffset = XMLoadFloat4x4(&matOffB);
		vForward = XMVector3Normalize(matOffset.r[3] - matBone.r[3]);
		vForward = XMVector3Normalize(XMVectorSetY(vForward, -0.7f));
	}
	else if (m_eType == DRAGON_SKILL::TURNBREATH)
	{ 
		_float4x4 matOffB = Get_BoneMatrix(m_iOffsetBoneIdex);
		_matrix matOffset = XMLoadFloat4x4(&matOffB);
		vForward = XMVector3Normalize(matOffset.r[3]  - matBone.r[3]);
		vForward = XMVector3Normalize(XMVectorSetY(vForward, -0.158f));
	}
	else if (m_eType == DRAGON_SKILL::LONGBREATH)
	{
		auto pSrc = Get_Owner();
		if (nullptr == pSrc) return;
		auto pTarget = pSrc->Get_Target();
		if (nullptr == pTarget) return;

		_float4x4 matOffB = Get_BoneMatrix(m_iOffsetBoneIdex);
		_matrix matOffset = XMLoadFloat4x4(&matOffB);
		
		XMStoreFloat3(&m_vTargetDir, XMVector3Normalize(XMLoadFloat3(&pTarget->GetTransform().GetPosition()) - matOffset.r[3]));

		vForward = XMVector3Normalize(XMVectorLerp(XMLoadFloat3(&m_vDir), XMLoadFloat3(&m_vTargetDir), tLBreath));
	
	}

	_vector vUp = XMVector3Normalize(matBone.r[1]);

	if (fabsf(XMVectorGetX(XMVector3Dot(vForward, vUp))) > 0.98f)
		vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);

	_vector vRight = XMVector3Normalize(XMVector3Cross(vUp, vForward));
	vUp = XMVector3Normalize(XMVector3Cross(vForward, vRight));

	_matrix breathWorld = XMMatrixIdentity();
	breathWorld.r[0] = XMVectorSetW(vRight, 0.f);
	breathWorld.r[1] = XMVectorSetW(vUp, 0.f);
	breathWorld.r[2] = XMVectorSetW(vForward, 0.f);
	breathWorld.r[3] = XMVectorSetW(matBone.r[3], 1.f);


	_float4x4 breathWorldData{};
	XMStoreFloat4x4(&breathWorldData, breathWorld);

	m_fBreathParticleTick += fTimeDelta;
	constexpr _float fBreathSpawnInterval = 1.f / 60.f;
	if (m_fBreathParticleTick >= fBreathSpawnInterval)
	{
		m_fBreathParticleTick -= fBreathSpawnInterval;
		CGameInstance::Get().Spawn("FinalBreath.json", breathWorldData);
		//CGameInstance::Get().Spawn("DragonBreath.json", breathWorldData);
	}

	m_fBreathDis = m_fMaxBreath * tBreath;
	if (MoveSweep(matBone.r[3], vForward, fTimeDelta))
	{
		//if (m_iSkillEffID != INVALID_EFFECT_INSTANCE_ID)
		//	CGameInstance::Get().SetEffectWorldMatrix(m_iSkillEffID, breathWorldData);
		GetTransform().SetPosition(matBone.r[3]);
		GetTransform().SetQuaternion(vQuat);
		GetTransform().Update();
	
	}
	
}

_bool CEdgBreath::MoveSweep(_vector vNextPos, _vector vCurDir, _float fTimeDelta)
{
	_float3 vPos{}, vDir{};
	
	XMStoreFloat3(&vPos, vNextPos);
	XMStoreFloat3(&vDir, vCurDir);
	constexpr uint32_t iDebugCnt = 20;

	PX_SWEEP_DESC SweepDesc{};
	SweepDesc.tGeometry.eType = PX_QUERY_GEOMETRY_TYPE::SPHERE;
	SweepDesc.tGeometry.fRadius = m_fRadius;
	SweepDesc.tPose.vPosition = vPos;
	SweepDesc.vDirection = vDir;
	SweepDesc.tFilter = m_pxQueryFilter;
	SweepDesc.fMaxDistance = m_fBreathDis;

	for (uint32_t i = 0; i < iDebugCnt; ++i)
	{
		const _float t = static_cast<_float>(i) / static_cast<_float>(iDebugCnt - 1);
		_float3 vDebugPos{};
		XMStoreFloat3(&vDebugPos, XMLoadFloat3(&SweepDesc.tPose.vPosition) + XMLoadFloat3(&vDir) * SweepDesc.fMaxDistance * t);
		DebugLine(vDebugPos);
	}

	PX_SWEEP_RESULT SweepResult{};
	auto pPhysX = CGameInstance::Get().GetPhysXManager();
	if (nullptr == pPhysX) return false;

	//가시브래스로 교체예정
	//롱브래스나 회전 브래스는 땅에 닿으면 그 자리에 데칼 소환하게
	if (m_eType == DRAGON_SKILL::GASIBREATH && !m_bGround)
	{
		PX_SWEEP_DESC GroundDesc = SweepDesc;
		GroundDesc.tFilter = PX_QUERY_FILTER_DESC{ .iQueryMask = ETOUI(COLLISION_LAYER::WORLD_STATIC), .bQueryStatic = true, .bQueryDynamic = false, .bIncludeTrigger = false };

		PX_SWEEP_RESULT GroundSweep{};

		if (pPhysX->Sweep(GroundDesc, GroundSweep) && GroundSweep.bHit)
		{
			m_bGround = true;
			SpawnGasi(XMLoadFloat3(&GroundSweep.vHitpos), vCurDir);
		}
	}

	if (m_eType == DRAGON_SKILL::BREATH)
	{
		PX_SWEEP_DESC GroundDesc = SweepDesc;
		GroundDesc.tFilter = PX_QUERY_FILTER_DESC{ .iQueryMask = ETOUI(COLLISION_LAYER::WORLD_STATIC), .bQueryStatic = true, .bQueryDynamic = false, .bIncludeTrigger = false };

		PX_SWEEP_RESULT GroundSweep{};

		if (pPhysX->Sweep(GroundDesc, GroundSweep) && GroundSweep.bHit)
		{
			m_fGroundParticleTick += fTimeDelta;

			if (m_fGroundParticleTick >= 0.1f)
			{
				m_fGroundParticleTick -= 0.1f;

				_float4x4 groundWorld{};
				XMStoreFloat4x4(&groundWorld, XMMatrixTranslation(GroundSweep.vHitpos.x, GroundSweep.vHitpos.y, GroundSweep.vHitpos.z));
				CGameInstance::Get().Spawn("BreathAfter.json", groundWorld);
			}
		}
		else
		{
			m_fGroundParticleTick = 0.f;
		}
	}




	if (pPhysX->Sweep(SweepDesc, SweepResult) && SweepResult.bHit)
	{
		//auto pTarget = CGameInstance::Get().GetGameObjectByHandleT<CPlayer>(SweepResult.hGameObject);
		//pTarget->OnQueryHit(m_fDamage);
		return false;
	}

	return true;
}

E::UPtr<CEdgBreath> CEdgBreath::Create()
{
	auto pInstance = E::ToUPtr(new CEdgBreath{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CEdgBreath");
		return nullptr;
	}
	return pInstance;
}

E::UPtr<E::CPrototype> CEdgBreath::Clone(void* pArg)
{
	auto pInstance = E::ToUPtr(new CEdgBreath{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CEdgBreath");
		return nullptr;
	}

	return pInstance;
}
