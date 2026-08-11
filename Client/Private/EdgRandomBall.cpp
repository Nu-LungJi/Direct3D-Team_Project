#include "pch.h"
#include "EdgRandomBall.h"
#include "PhysXManager.h"
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
		MSG_BOX("Create Failed EDG FireBall");
		return E_FAIL;
	}
	m_fDamage = 30.f;
	m_fSpeed = 100.f;
	m_fRadius = 0.5f;
	m_fMaxLife = 3.f;
	return S_OK;
}

void CEdgRandomBall::PriorityUpdate(E::_float fTimeDelta)
{
	__super::PriorityUpdate(fTimeDelta);

	if (!m_bActive) return;

	if (Life_Check(fTimeDelta))
		SetPendingDestroy();
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

void CEdgRandomBall::Active(const _string& SkillName)
{
	_float4x4 matB = Get_BoneMatrix(m_iBoneIndex);
	_float4x4 matOffB = Get_BoneMatrix(m_iOffsetBoneIdex);
	_matrix matBone = XMLoadFloat4x4(&matB);
	_matrix matOffset = XMLoadFloat4x4(&matOffB);

	_vector vQuat = XMQuaternionRotationMatrix(matBone);

	GetTransform().SetPosition(matBone.r[3]);
	GetTransform().SetQuaternion(vQuat);
	GetTransform().Update();
	//Set_TargetDir(matBone.r[3]);
	XMStoreFloat3(&m_vTargetDir, XMVector3Normalize(matOffset.r[3] - matBone.r[3]));
	m_bActive = true;
	m_bHit = false;
	m_fLife = 0.f;
	Spawn_Skill_Effect(SkillName);
}

void CEdgRandomBall::Cancle()
{
	ResetValue();
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
