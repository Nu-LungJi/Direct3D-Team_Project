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

	if (!CGameInstance::Get().IsInstancingEnabled())
	{
		return;
	}

	const auto& pModel = m_pComModelInstance->GetModel();
	if (!pModel->HasLocalBounds())
		return;

	MAPMESH_INSTANCE_DATA InstanceData{};
	XMStoreFloat4x4(
		&InstanceData.world,
		GetTransform().GetLoadedCombinedWorldMatrix());

	BoundingBox WorldBounds{};
	pModel->GetLocalBounds().Transform(
		WorldBounds,
		GetTransform().GetLoadedCombinedWorldMatrix());

	MAPMESH_OCCLUSION_DATA OcclusionData{};
	OcclusionData.worldCenter = WorldBounds.Center;
	OcclusionData.worldExtents = WorldBounds.Extents;

	CGameInstance::Get().PushMapObjectInstance(
		pModel,
		InstanceData,
		OcclusionData);
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
