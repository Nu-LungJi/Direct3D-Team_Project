#include "pch.h"
#include "MyMagicSquareStep.h"
#include "ComStaticModelInstance.h"
#include "ComPxRigidBody.h"
#include "ComPxBoxCollider.h"

NS_USING(Client)

CMyMagicSquareStep::CMyMagicSquareStep() = default;

CMyMagicSquareStep::CMyMagicSquareStep(const CMyMagicSquareStep& rhs)
	: CGameObject{ rhs }
	, m_pResBoxGeometry{ rhs.m_pResBoxGeometry }
	, m_pResPhysXMaterial{ rhs.m_pResPhysXMaterial }
{
}

HRESULT CMyMagicSquareStep::InitializePrototype(void* pArg)
{
	m_pResBoxGeometry = CResPhysXBoxGeometry::CreateAndLoad({
			.vHalfExtents = MAGIC_STEP_BOX_HALF_EXTENTS });
	m_pResPhysXMaterial = CResPhysXMaterial::CreateAndLoad({});
	if (!m_pResBoxGeometry || !m_pResPhysXMaterial)
		return E_FAIL;
	return S_OK;
}

HRESULT CMyMagicSquareStep::Initialize(void* pArg)
{
	auto* pDesc = static_cast<DESC*>(pArg);

	if (!pDesc || FAILED(CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	}

	GetTransform().SetPosition(pDesc->vInitialPosition);
	GetTransform().SetRotationEuler(pDesc->vInitialRotation);
	GetTransform().SetScale(pDesc->vInitialScale);

	m_vMoveTarget = pDesc->vInitialPosition;
	m_vFinalMoveTarget = pDesc->vInitialPosition;

	{
		CComStaticModelInstance::DESC Desc{};
		Desc.sGroupTag = pDesc->ResMajorTag;
		Desc.sResTag = pDesc->ResMinorTag;
		//Desc.sGroupTag = "LEVEL_CREATURE";
		//Desc.sResTag = "Static_SquareStep_A_Resource";
		if (FAILED(AddComponentFromProto(
			"PERMANENT",
			"Prototype_Component_StaticModelInstance",
			"ComModelInstance",
			&Desc,
			&m_pComModelInstance)))
			return E_FAIL;
	}


	{
		CComPxRigidBody::DESC Desc{};
		Desc.eType = CComPxRigidBody::TYPE::KINEMATIC;
		Desc.vPosition = pDesc->vInitialPosition;
		Desc.vRotation = GetTransform().GetQuaternion();
		if (FAILED(AddComponentFromProto(
			"PHYSX",
			"Prototype_Component_ComPxRigidBody",
			"ComPxRigidBody",
			&Desc,
			&m_pComPxRigidBody)))
			return E_FAIL;
	}

	{
		CComPxBoxCollider::DESC Desc{};
		Desc.pComPxRigidBody = m_pComPxRigidBody;
		Desc.pResBoxGeo = m_pResBoxGeometry;
		Desc.pResMaterial = m_pResPhysXMaterial;
		Desc.vLocalOffset = MAGIC_STEP_BOX_LOCAL_OFFSET;
		Desc.tFilter = pDesc->tFilter;
		if (FAILED(AddComponentFromProto(
			"PHYSX",
			"Prototype_Component_ComPxBoxCollider",
			"ComPxBoxCollider",
			&Desc,
			&m_pComPxBoxCollider)))
			return E_FAIL;
	}
	return S_OK;
}

void CMyMagicSquareStep::FixedUpdate(_float fTimeDelta)
{
	if (m_eState == STATE::IDLE)
	{
		return;
	}

	const _float3 vCurrent = GetTransform().GetPosition();
	const _vector vDelta =
		XMLoadFloat3(&m_vMoveTarget) - XMLoadFloat3(&vCurrent);
	const _float fDistance =
		XMVectorGetX(XMVector3Length(vDelta));

	_float3 vNext{};
	if (fDistance <= FLT_EPSILON)
	{
		vNext = m_vMoveTarget;
	}
	else
	{
		const _float fMoveDistance = std::min(m_fSpeed * fTimeDelta, fDistance);
		XMStoreFloat3(&vNext, XMLoadFloat3(&vCurrent) + XMVector3Normalize(vDelta) * fMoveDistance);
		if (fMoveDistance >= fDistance)
			vNext = m_vMoveTarget;
	}

	if (fDistance <= FLT_EPSILON ||
		XMVector3Equal(
			XMLoadFloat3(&vNext),
			XMLoadFloat3(&m_vMoveTarget)))
	{
		if (m_eState == STATE::BOUNCE_RISE)
		{
			m_vMoveTarget =
				m_vFinalMoveTarget;
			m_fSpeed =
				m_fBounceSettleSpeed;
			m_eState =
				STATE::BOUNCE_SETTLE;
		}
		else
		{
			m_eState = STATE::IDLE;
		}
	}

	GetTransform().SetPosition(vNext);
	if (m_pComPxRigidBody)
	{
		m_pComPxRigidBody->SetKinematicTarget(vNext, GetTransform().GetQuaternion());
	}
}

void CMyMagicSquareStep::LateUpdate(_float fTimeDelta)
{
	GetTransform().Update();

	if (!m_pComModelInstance || !m_pComModelInstance->GetModel())
		return;

	if (!CGameInstance::Get().IsInstancingEnabled())
	{
		return;
	}

	CGameInstance::Get().Add_Instance(
		m_pComModelInstance,
		*GetTransform().GetCombinedWorldMatrix());
}

HRESULT CMyMagicSquareStep::Render_Instanced(
	ID3D11DeviceContext* pContext,
	const RENDER_CTX&,
	const MODEL_INSTANCE_BATCH& batch)
{
	return m_pComModelInstance
		? m_pComModelInstance->RenderDynamicInstances(pContext, batch)
		: E_FAIL;
}

HRESULT CMyMagicSquareStep::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
	return S_OK;
}

void CMyMagicSquareStep::UpdateGUI()
{
	CGameObject::UpdateGUI();

	_float3 vMoveTarget;
	XMStoreFloat3(&vMoveTarget, GetMoveTarget());

	if (ImGui::DragFloat3("MoveTarget", (float*)&vMoveTarget, 0.1f))
	{
		SetMoveTarget(XMVectorSet(vMoveTarget.x, vMoveTarget.y, vMoveTarget.z, 1.f));
	}
	if (ImGui::Button("SetRandRarget"))
	{
		SetMoveTarget(XMVectorSet(
			Randf(-1.f, 1.f),
			Randf(-1.f, 1.f),
			Randf(-1.f, 1.f), 1.f));
	}
}

void CMyMagicSquareStep::SetMoveTarget(_fvector vMoveTarget)
{
	XMStoreFloat3(&m_vMoveTarget, vMoveTarget);
	m_vFinalMoveTarget = m_vMoveTarget;
	m_eState = STATE::MOVE;
}

void CMyMagicSquareStep::SetBounceMoveTarget(
	_fvector vFinalTarget,
	_float fRiseSpeed,
	_float fBounceHeight,
	_float fSettleSpeed)
{
	XMStoreFloat3(
		&m_vFinalMoveTarget,
		vFinalTarget);
	m_vMoveTarget = m_vFinalMoveTarget;
	m_vMoveTarget.y += fBounceHeight;
	m_fSpeed = fRiseSpeed;
	m_fBounceSettleSpeed = fSettleSpeed;
	m_eState = STATE::BOUNCE_RISE;
}

void CMyMagicSquareStep::SetKinematicPosition(
	const _float3& vPosition)
{
	m_vMoveTarget = vPosition;
	m_vFinalMoveTarget = vPosition;
	m_eState = STATE::IDLE;
	GetTransform().SetPosition(vPosition);

	if (m_pComPxRigidBody)
	{
		m_pComPxRigidBody->SetKinematicTarget(
			vPosition,
			GetTransform().GetQuaternion());
	}
}

_bool CMyMagicSquareStep::OnAcquireFromPool(void* pArg)
{
	const auto* pDesc = static_cast<POOL_ACQUIRE_DESC*>(pArg);
	if (!pDesc || !m_pComPxRigidBody || !m_pComPxBoxCollider)
		return false;

	if (!m_pComPxRigidBody->SetPose(
			pDesc->vPosition,
			pDesc->vRotation))
	{
		return false;
	}

	GetTransform().SetPosition(pDesc->vPosition);
	GetTransform().SetQuaternion(pDesc->vRotation);
	GetTransform().Update();

	m_vMoveTarget = pDesc->vPosition;
	m_vFinalMoveTarget = pDesc->vPosition;
	m_fSpeed = 1.f;
	m_fBounceSettleSpeed = 1.f;
	m_eState = STATE::IDLE;
	return true;
}

void CMyMagicSquareStep::OnReleaseToPool()
{
	// [LSY] 비활성 발판은 충돌이 꺼진 뒤 맵 밖으로 이동해 디버그 및 실패 상황에서도 잔류하지 않게 한다.
	constexpr _float3 vParkingPosition{ 0.f, -10000.f, 0.f };

	m_vMoveTarget = vParkingPosition;
	m_vFinalMoveTarget = vParkingPosition;
	m_fSpeed = 1.f;
	m_fBounceSettleSpeed = 1.f;
	m_eState = STATE::IDLE;

	GetTransform().SetPosition(vParkingPosition);
	GetTransform().Update();

	if (m_pComPxRigidBody)
	{
		m_pComPxRigidBody->SetPose(
			vParkingPosition,
			GetTransform().GetQuaternion());
	}
}

void CMyMagicSquareStep::OnManagedUpdateEnabled()
{
	if (!m_pComPxBoxCollider)
		return;

	const _bool bSimulationEnabled =
		m_pComPxBoxCollider->SetSimulationEnabled(true);
	const _bool bQueryEnabled =
		m_pComPxBoxCollider->SetQueryEnabled(true);
	if (!bSimulationEnabled || !bQueryEnabled)
	{
		DEBUG_LOG("[MagicStepPool] Failed to enable pooled step collision.\n");
	}
}

void CMyMagicSquareStep::OnManagedUpdateDisabled()
{
	if (!m_pComPxBoxCollider)
		return;

	// [LSY] Managed Update만 끄면 PhysX Shape는 씬에 남으므로 Simulation과 Query도 함께 제외한다.
	const _bool bSimulationDisabled =
		m_pComPxBoxCollider->SetSimulationEnabled(false);
	const _bool bQueryDisabled =
		m_pComPxBoxCollider->SetQueryEnabled(false);
	if (!bSimulationDisabled || !bQueryDisabled)
	{
		DEBUG_LOG("[MagicStepPool] Failed to disable pooled step collision.\n");
	}
}

UPtr<CMyMagicSquareStep> CMyMagicSquareStep::Create()
{
	auto pInstance = ToUPtr(new CMyMagicSquareStep{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Create Failed CMyMagicSquareStep");
		return nullptr;
	}
	return pInstance;
}

UPtr<CPrototype> CMyMagicSquareStep::Clone(void* pArg)
{
	auto pInstnace = ToUPtr(new CMyMagicSquareStep{ *this });
	if (FAILED(pInstnace->Initialize(pArg)))
	{
		MSG_BOX("Clone Failed CMyMagicSquareStep");
		return nullptr;
	}
	return pInstnace;
}
