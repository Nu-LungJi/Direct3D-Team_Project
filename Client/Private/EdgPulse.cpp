#include "pch.h"
#include "EdgPulse.h"
#include "PhysXManager.h"
#include "Player.h"
NS_USING(Client)
CEdgPulse::CEdgPulse()
{
}

CEdgPulse::CEdgPulse(const CEdgPulse& rhs) : CDragonSkill(rhs)
{
}

CEdgPulse::~CEdgPulse()
{
}

HRESULT CEdgPulse::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CEdgPulse::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
	{
		MSG_BOX("Create Failed EDG CEdgPulse");
		return E_FAIL;
	}
	m_fDamage = 70.f;
	m_fSpeed = 20.f;
	m_fRadius = 1.5f;
	m_fMaxLife = 3.f;


	return S_OK;
}

void CEdgPulse::PriorityUpdate(E::_float fTimeDelta)
{
	__super::PriorityUpdate(fTimeDelta);

	if (!m_bActive) return;

	Life_Check(fTimeDelta);
}

void CEdgPulse::FixedUpdate(E::_float fTimeDelta)
{
	if (!m_bActive) return;

	__super::FixedUpdate(fTimeDelta);
	Pulse(fTimeDelta);
}

void CEdgPulse::Update(E::_float fTimeDelta)
{
	if (!m_bActive) return;

	__super::Update(fTimeDelta);
}

void CEdgPulse::LateUpdate(E::_float fTimeDelta)
{
	if (!m_bActive) return;

	__super::LateUpdate(fTimeDelta);
}

void CEdgPulse::Active(EDG_ACSKT_DESC& SkillTable, _vector vOffsetPos)
{
	_float4x4 BoneMatrix = Get_BoneMatrix(m_iBoneIndex);

	_matrix matBone = XMLoadFloat4x4(&BoneMatrix);
	_vector vQuat = XMQuaternionRotationMatrix(matBone);

	GetTransform().SetPosition(matBone.r[3]);
	GetTransform().SetQuaternion(vQuat);
	GetTransform().Update();
	m_bActive = true;
	m_bHit = false;
	m_bPlayerHit = false;
	m_bWorldStaticHit = false;
	m_vClosestPointToPlayer = {};
	m_fLife = 0.f;
	m_fRadius = 1.5f;
	Spawn_Skill_Effect(SkillTable.SkillName);
}

void CEdgPulse::Cancle()
{
	ResetValue();

	CGameInstance::Get().StopEffect(m_iSkillEffID);
}

void CEdgPulse::ResetValue()
{
	if (m_iBurstParticleOwnerId != INVALID_PARTICLE_OWNER_ID)
	{
		CGameInstance::Get().ClearParticleOwner(m_iBurstParticleOwnerId);
		m_iBurstParticleOwnerId = INVALID_PARTICLE_OWNER_ID;
	}

	__super::ResetValue();
	m_bPlayerHit = false;
	m_bWorldStaticHit = false;
	m_vClosestPointToPlayer = {};
}

void CEdgPulse::Pulse(_float fTimeDelta)
{
	_float4x4 BoneMatrix = Get_BoneMatrix(m_iBoneIndex);
	_matrix matBone = XMLoadFloat4x4(&BoneMatrix);

	m_fRadius += m_fSpeed * fTimeDelta;

	GetTransform().SetPosition(matBone.r[3]);
	GetTransform().Update();

	PulseOverlap(matBone.r[3]);

	if (m_iSkillEffID != INVALID_EFFECT_INSTANCE_ID)
		CGameInstance::Get().SetEffectWorldMatrix(m_iSkillEffID, *GetTransform().GetWorldMatrix());
}

void CEdgPulse::PulseOverlap(_vector vCenter)
{
	_float3 vPos = {};
	XMStoreFloat3(&vPos, vCenter);

	////////////////////////////////////
	DebugLine(vPos);
	/////////////////////////////////////

	auto pPhysX = CGameInstance::Get().GetPhysXManager();
	if (nullptr == pPhysX)
		return;

	PX_OVERLAP_DESC OverlapDesc{};
	OverlapDesc.tGeometry = { .eType = PX_QUERY_GEOMETRY_TYPE::SPHERE, .fRadius = m_fRadius };
	OverlapDesc.tPose = { .vPosition = vPos };

	if (!m_bWorldStaticHit)
	{
		OverlapDesc.tFilter = {
			.iQueryMask = ETOUI(COLLISION_LAYER::WORLD_STATIC),
			.bQueryStatic = true,
			.bQueryDynamic = false,
			.bIncludeTrigger = false
		};

		PX_OVERLAP_RESULT WorldResult{};
		if (pPhysX->Overlap(OverlapDesc, WorldResult) && WorldResult.bHit)
		{
			m_bWorldStaticHit = true;

			auto pOwner = Get_Owner();
			if (nullptr != pOwner)
			{
				auto pTarget = pOwner->Get_Target();
				if (nullptr != pTarget)
				{
					_vector vPlayerPosition = XMLoadFloat3(&pTarget->GetTransform().GetPosition());
					_float3 vEffectPosition = pOwner->GetTransform().GetPosition();
					vEffectPosition.y = XMVectorGetY(vPlayerPosition) - 5.f;
					m_vClosestPointToPlayer = vEffectPosition;

					_vector vEffectForward = vPlayerPosition - XMLoadFloat3(&vEffectPosition);
					if (XMVectorGetX(XMVector3LengthSq(vEffectForward)) <= FLT_EPSILON)
						vEffectForward = XMVectorSet(0.f, 0.f, 1.f, 0.f);
					else
						vEffectForward = XMVector3Normalize(vEffectForward);

					_vector vWorldUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
					if (fabsf(XMVectorGetX(XMVector3Dot(vEffectForward, vWorldUp))) > 0.999f)
						vWorldUp = XMVectorSet(0.f, 0.f, 1.f, 0.f);

					_vector vEffectRight = XMVector3Normalize(XMVector3Cross(vWorldUp, vEffectForward));
					_vector vEffectUp = XMVector3Normalize(XMVector3Cross(vEffectForward, vEffectRight));

					_matrix matEffectWorld = XMMatrixIdentity();
					matEffectWorld.r[0] = XMVectorSetW(vEffectRight, 0.f);
					matEffectWorld.r[1] = XMVectorSetW(vEffectUp, 0.f);
					matEffectWorld.r[2] = XMVectorSetW(vEffectForward, 0.f);
					matEffectWorld.r[3] = XMVectorSetW(XMLoadFloat3(&vEffectPosition), 1.f);

					_float4x4 effectWorld{};
					XMStoreFloat4x4(&effectWorld, matEffectWorld);

					m_iBurstParticleOwnerId = CGameInstance::Get().Spawn("Ranrok_BurstB.json", effectWorld);
				}
			}
		}
	}

	if (!m_bPlayerHit)
	{
		OverlapDesc.tFilter = {
			.iQueryMask = ETOUI(COLLISION_LAYER::PLAYER_HURTBOX),
			.bQueryStatic = false,
			.bQueryDynamic = true,
			.bIncludeTrigger = false
		};

		PX_OVERLAP_RESULT PlayerResult{};
		if (pPhysX->Overlap(OverlapDesc, PlayerResult) && PlayerResult.bHit)
		{
			auto pPlayer = CGameInstance::Get().GetGameObjectByHandleT<CPlayer>(PlayerResult.hGameObject);
			if (nullptr != pPlayer)
			{
				m_bPlayerHit = true;
				pPlayer->OnQueryHit(static_cast<int32_t>(m_fDamage), vPos);
			}
		}
	}
}

E::UPtr<CEdgPulse> CEdgPulse::Create()
{
	auto pInstance = E::ToUPtr(new CEdgPulse{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CEdgPulse");
		return nullptr;
	}
	return pInstance;
}

E::UPtr<E::CPrototype> CEdgPulse::Clone(void* pArg)
{
	auto pInstance = E::ToUPtr(new CEdgPulse{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CEdgPulse");
		return nullptr;
	}

	return pInstance;
}
