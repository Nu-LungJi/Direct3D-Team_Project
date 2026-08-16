#include "pch.h"
#include "EdgGasi.h"
#include "PhysXManager.h"
NS_USING(Client)
CEdgGasi::CEdgGasi()
{
}

CEdgGasi::CEdgGasi(const CEdgGasi& rhs) : CDragonSkill(rhs)
{
}

CEdgGasi::~CEdgGasi()
{
}

HRESULT CEdgGasi::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CEdgGasi::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
	{
		MSG_BOX("Create Failed EDG Gasi");
		return E_FAIL;
	}
	m_fDamage = 30.f;
	m_fSpeed = 30.f;
	m_fRadius = 3.f;
	m_fMaxLife = 3.f;

	return S_OK;
}

void CEdgGasi::PriorityUpdate(E::_float fTimeDelta)
{
	__super::PriorityUpdate(fTimeDelta);

	if (!m_bActive) return;

	m_fLife += fTimeDelta;
	if (m_fLife >= m_fMaxLife || m_bHit)
		Cancle();

}

void CEdgGasi::FixedUpdate(E::_float fTimeDelta)
{
	if (!m_bActive) return;

	__super::FixedUpdate(fTimeDelta);
	MoveGasi(fTimeDelta);
}

void CEdgGasi::Update(E::_float fTimeDelta)
{
	if (!m_bActive) return;

	__super::Update(fTimeDelta);
}

void CEdgGasi::LateUpdate(E::_float fTimeDelta)
{
	if (!m_bActive) return;

	__super::LateUpdate(fTimeDelta);
}

void CEdgGasi::Active(EDG_ACSKT_DESC& SkillTable, _vector vOffsetPos)
{
	Set_TargetDir(vOffsetPos);
	m_vTargetDir.y = 0.f;
	_vector vDirection = XMLoadFloat3(&m_vTargetDir);


	XMStoreFloat3(&m_vTargetDir,XMVector3Normalize(vDirection));
	GetTransform().SetPosition(vOffsetPos);
	GetTransform().Update();

	m_bActive = true;
	m_bHit = false;
	m_fLife = 0.f;
	m_fMaxLife = SkillTable.fLifeTime;
	Spawn_Skill_Effect(SkillTable.SkillName);
}

void CEdgGasi::Cancle()
{
	ResetValue();
}

void CEdgGasi::MoveGasi(_float fTimeDelta)
{
	_vector vPos = GetTransform().GetLoadedPostion();
	_vector vDir = XMVector3Normalize(XMLoadFloat3(&m_vTargetDir));
	_vector vNextPos = vPos + vDir * m_fSpeed * fTimeDelta;
	
	if (MoveSweep(vNextPos))
	{
		GetTransform().SetPosition(vNextPos);
		GetTransform().Update();

		//f (m_iSkillEffID != INVALID_EFFECT_INSTANCE_ID)
		//	CGameInstance::Get().SetEffectWorldMatrix(m_iSkillEffID, *GetTransform().GetWorldMatrix());
	}

}

_bool CEdgGasi::MoveSweep(_vector vNextPos)
{
	_float3 vPos = GetTransform().GetPosition();

	_vector vDisPlacement = vNextPos - XMLoadFloat3(&vPos);

	_float fDist = XMVectorGetX(XMVector3Length(vDisPlacement));

	_float3 vDir = {};
	XMStoreFloat3(&vDir, XMVector3Normalize(vDisPlacement));

	PX_SWEEP_DESC SweepDesc{};
	SweepDesc.tGeometry.eType = PX_QUERY_GEOMETRY_TYPE::SPHERE;
	SweepDesc.tGeometry.fRadius = m_fRadius;
	SweepDesc.tPose.vPosition = vPos;
	SweepDesc.vDirection = vDir;
	SweepDesc.tFilter = m_pxQueryFilter;
	SweepDesc.fMaxDistance = fDist;

	////////////////////////////////////
	DebugLine(SweepDesc.tPose.vPosition);
	/////////////////////////////////////

	PX_SWEEP_RESULT SweepResult{};
	auto pPhysX = CGameInstance::Get().GetPhysXManager();
	if (nullptr == pPhysX) return false;

	if (pPhysX->Sweep(SweepDesc, SweepResult) && SweepResult.bHit)
	{
		//auto pTarget = CGameInstance::Get().GetGameObjectByHandleT<CPlayer>(SweepResult.hGameObject);
		//pTarget->OnQueryHit(m_fDamage);
		m_bHit = true;
		//펑
		return false;
	}

	return true;
}

E::UPtr<CEdgGasi> CEdgGasi::Create()
{
	auto pInstance = E::ToUPtr(new CEdgGasi{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CEdgGasi");
		return nullptr;
	}
	return pInstance;
}

E::UPtr<E::CPrototype> CEdgGasi::Clone(void* pArg)
{
	auto pInstance = E::ToUPtr(new CEdgGasi{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CEdgGasi");
		return nullptr;
	}

	return pInstance;
}
