#include "pch.h"
#include "EdgRandomBall.h"
#include "PhysXManager.h"
#include "ComPxRigidBody.h"
#include "ComPxSphereCollider.h"
NS_USING(Client)
CEdgRandomBall::CEdgRandomBall()
{
}

CEdgRandomBall::CEdgRandomBall(const CEdgRandomBall& rhs) : CDragonSkill(rhs)
{
}

CEdgRandomBall::~CEdgRandomBall()
{
}

HRESULT CEdgRandomBall::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CEdgRandomBall::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
	{
		MSG_BOX("Create Failed EDG RandomBall");
		return E_FAIL;
	}

	m_fDamage = 30.f;
	m_fSpeed = 100.f;
	m_fRadius = 0.5f;
	m_fMaxLife = 6.f;

	{
		CComPxRigidBody::DESC Desc{};
		Desc.eType = CComPxRigidBody::TYPE::KINEMATIC;
		Desc.vPosition = GetTransform().GetPosition();
		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxRigidBody,
			"ComPxRigidBody", &Desc, &m_pComRigidBody)))
			return E_FAIL;
	}

	{
		CComPxSphereCollider::DESC Desc{};
		Desc.pComPxRigidBody = m_pComRigidBody;
		Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});
		Desc.pResSphereGeo = CResPhysXSphereGeometry::CreateAndLoad({ .fRadius = m_fRadius });
		Desc.bIsTrigger = false;
		Desc.tFilter = PX_FILTER_DESC{ .iLayer = ETOUI(COLLISION_LAYER::ENEMY_BODY),.iSimulationMask = 0,
		.iQueryMask = 0 };

		if (!Desc.pResMaterial || !Desc.pResSphereGeo || FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxSphereCollider,
			"ComPxSphereCollider", &Desc, &m_pComSphereCollider)))
			return E_FAIL;

		//물리 충돌 사용안함
		m_pComSphereCollider->SetSimulationEnabled(false);
		m_pComSphereCollider->SetQueryEnabled(false);
	}
	return S_OK;
}

void CEdgRandomBall::PriorityUpdate(E::_float fTimeDelta)
{
	__super::PriorityUpdate(fTimeDelta);

	if (!m_bActive) return;

	if (Life_Check(fTimeDelta))
	{
		auto pSrc = Get_Owner();
		if (nullptr == pSrc) return;

		pSrc->Heal(20.f);
	}
}

void CEdgRandomBall::FixedUpdate(E::_float fTimeDelta)
{
	if (!m_bActive) return;

	__super::FixedUpdate(fTimeDelta);
	Ball(fTimeDelta);
}

void CEdgRandomBall::Update(E::_float fTimeDelta)
{
	if (!m_bActive) return;

	__super::Update(fTimeDelta);
}

void CEdgRandomBall::LateUpdate(E::_float fTimeDelta)
{
	if (!m_bActive) return;

	__super::LateUpdate(fTimeDelta);
}

void CEdgRandomBall::Active(EDG_ACSKT_DESC& SkillTable, _vector vOffsetPos)
{
	if (nullptr == m_pComRigidBody || nullptr == m_pComSphereCollider) return;

	m_eType = SkillTable.eType;
	auto pOwner = Get_Owner();
	if (nullptr == pOwner) return;

	auto pTarget = pOwner->Get_Target();
	if (nullptr == pTarget) return;

	_vector vTarget = XMLoadFloat3(&pTarget->GetTransform().GetPosition())+ vOffsetPos;

	_float4x4 matB = Get_BoneMatrix(m_iBoneIndex);
	_matrix matBone = XMLoadFloat4x4(&matB);
	_vector vDir = XMVector3Normalize(vTarget - matBone.r[3]);
	_float4 vQuat{};
	_float3 vPos{};
	XMStoreFloat4(&vQuat, XMQuaternionRotationMatrix(matBone));
	XMStoreFloat3(&vPos, matBone.r[3] + vDir * SkillTable.fDist);
	GetTransform().SetPosition(XMLoadFloat3(&vPos));
	GetTransform().SetQuaternion(vQuat);
	GetTransform().Update();

	m_pComRigidBody->SetPose(vPos, vQuat);
	m_pComSphereCollider->SetSimulationEnabled(false);
	m_pComSphereCollider->SetQueryEnabled(true);

	m_bActive = true;
	m_bHit = false;
	m_fLife = 0.f;

	uint32_t randomInt = RandInt(0, 2);

	switch (randomInt) {
	case 0:
		m_eColor = COLOR::YELLOW;
		m_iEffectID = CGameInstance::Get().PlayEffect("YellowSphere", *m_pComTransform->GetWorldMatrix());
		break;
	case 1:
		m_eColor = COLOR::PURPLE;
		m_iEffectID = CGameInstance::Get().PlayEffect("PurpleSphere", *m_pComTransform->GetWorldMatrix());
		break;
	case 2:
		m_eColor = COLOR::RED;
		m_iEffectID = CGameInstance::Get().PlayEffect("RedSphere", *m_pComTransform->GetWorldMatrix());
		break;
	}

	Spawn_Skill_Effect(SkillTable.SkillName);
}

void CEdgRandomBall::Cancle()
{
	if (m_pComSphereCollider)
	{
		m_pComSphereCollider->SetQueryEnabled(false);
		m_pComSphereCollider->SetSimulationEnabled(false);
	}
	///여기에 하면됨 터지는거
	if (m_iEffectID != INVALID_EFFECT_INSTANCE_ID)
	{
		CGameInstance::Get().StopEffect(m_iEffectID);
		m_iEffectID = INVALID_EFFECT_INSTANCE_ID;
	}	
	SetPendingDestroy();
	ResetValue();
}

_bool CEdgRandomBall::Check_Table(PLAYER_SKILL_TYPE eType)
{
	if (eType == PLAYER_SKILL_TYPE::DEFAULT || eType == PLAYER_SKILL_TYPE::END
		|| eType == PLAYER_SKILL_TYPE::ATTACK)
		return false;

	m_bHit = true;
	
	auto pDragon = Get_Owner();
	if (nullptr == pDragon) return false;

	pDragon->Check_Table(PLAYER_SKILL_TYPE::DESTORY);
	return true;
}

void CEdgRandomBall::Ball(_float fTimeDelta)
{
	_vector vPos = XMLoadFloat3(&GetTransform().GetPosition());

	if (Sweep(vPos))
	{
		if (m_iSkillEffID != INVALID_EFFECT_INSTANCE_ID)
			CGameInstance::Get().SetEffectWorldMatrix(m_iSkillEffID, *GetTransform().GetWorldMatrix());
		GetTransform().SetPosition(vPos);
		GetTransform().Update();
	}

}

_bool CEdgRandomBall::Sweep(_vector vNextPos)
{
	_float3 vPos = GetTransform().GetPosition();

	PX_SWEEP_DESC SweepDesc{};
	SweepDesc.tGeometry.eType = PX_QUERY_GEOMETRY_TYPE::SPHERE;
	SweepDesc.tGeometry.fRadius = m_fRadius;
	SweepDesc.tPose.vPosition = vPos;
	SweepDesc.tFilter = m_pxQueryFilter;

	////////////////////////////////////
	DebugLine(SweepDesc.tPose.vPosition);
	/////////////////////////////////////

	PX_SWEEP_RESULT SweepResult{};
	auto pPhysX = CGameInstance::Get().GetPhysXManager();
	if (nullptr == pPhysX) return false;

	if (pPhysX->Sweep(SweepDesc, SweepResult) && SweepResult.bHit)
	{
		m_bHit = true;
		//펑
		return false;
	}
	return true;
}

E::UPtr<CEdgRandomBall> CEdgRandomBall::Create()
{
	auto pInstance = E::ToUPtr(new CEdgRandomBall{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CEdgRandomBall");
		return nullptr;
	}
	return pInstance;
}

E::UPtr<E::CPrototype> CEdgRandomBall::Clone(void* pArg)
{
	auto pInstance = E::ToUPtr(new CEdgRandomBall{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CEdgRandomBall");
		return nullptr;
	}

	return pInstance;
}
