#include "pch.h"
#include "TestSquareStep.h"

#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComPxBoxCollider.h"
#include "ComPxRigidBody.h"
#include "ComStaticModelInstance.h"
#include "GameInstance.h"
#include "ResPhysXBoxGeometry.h"
#include "ResPhysXMaterial.h"
#include "Resources.h"

NS_USING(Client)

namespace
{
	constexpr _float3 SQUARE_STEP_BOX_HALF_EXTENTS{
		0.5031f, 1.5048f, 0.5031f };
	constexpr _float3 SQUARE_STEP_BOX_LOCAL_OFFSET{
		0.f, -1.4953f, 0.f };
}

CTestSquareStep::CTestSquareStep() = default;

CTestSquareStep::CTestSquareStep(const CTestSquareStep& prototype)
	: CGameObject{ prototype }
	, m_pResBoxGeometry{ prototype.m_pResBoxGeometry }
	, m_pResPhysXMaterial{ prototype.m_pResPhysXMaterial }
	, m_pResVertexShader{ prototype.m_pResVertexShader }
	, m_pResPixelShader{ prototype.m_pResPixelShader }
{
}

HRESULT CTestSquareStep::InitializePrototype(void*)
{
	m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim");
	if (!m_pResVertexShader || FAILED(m_pResVertexShader->Load()))
		return E_FAIL;

	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim");
	if (!m_pResPixelShader || FAILED(m_pResPixelShader->Load()))
		return E_FAIL;

	m_pResBoxGeometry = CResPhysXBoxGeometry::CreateAndLoad({
		.vHalfExtents = SQUARE_STEP_BOX_HALF_EXTENTS });
	m_pResPhysXMaterial = CResPhysXMaterial::CreateAndLoad({});
	if (!m_pResBoxGeometry || !m_pResPhysXMaterial)
		return E_FAIL;

	return S_OK;
}

HRESULT CTestSquareStep::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc || FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	GetTransform().SetPosition(pDesc->vInitialPosition);
	GetTransform().SetRotationEuler(pDesc->vInitialRotation);
	GetTransform().SetScale(pDesc->vInitialScale);
	m_vBasePosition = pDesc->vInitialPosition;
	m_fTargetY = m_vBasePosition.y;

	{
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto(
			"PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject",
			&Desc, &m_pComCBufferPerObject)))
			return E_FAIL;
	}

	{
		CComStaticModelInstance::DESC Desc{};
		Desc.sGroupTag = "LEVEL_CREATURE";
		Desc.sResTag = "Static_SquareStep_A_Resource";
		if (FAILED(AddComponentFromProto(
			"PERMANENT", "Prototype_Component_StaticModelInstance", "ComModelInstance",
			&Desc, &m_pComModelInstance)))
			return E_FAIL;
	}

	if (pDesc->bEnablePhysics)
	{
		{
			CComPxRigidBody::DESC Desc{};
			Desc.eType = CComPxRigidBody::TYPE::KINEMATIC;
			Desc.vPosition = pDesc->vInitialPosition;
			Desc.vRotation = GetTransform().GetQuaternion();
			if (FAILED(AddComponentFromProto(
				"PHYSX", "Prototype_Component_ComPxRigidBody", "ComPxRigidBody",
				&Desc, &m_pComPxRigidBody)))
				return E_FAIL;
		}

		{
			CComPxBoxCollider::DESC Desc{};
			Desc.pComPxRigidBody = m_pComPxRigidBody;
			Desc.pResBoxGeo = m_pResBoxGeometry;
			Desc.pResMaterial = m_pResPhysXMaterial;
			Desc.vLocalOffset = SQUARE_STEP_BOX_LOCAL_OFFSET;
			Desc.tFilter = pDesc->tFilter;
			if (!Desc.pResBoxGeo || !Desc.pResMaterial ||
				FAILED(AddComponentFromProto(
					"PHYSX", "Prototype_Component_ComPxBoxCollider", "ComPxBoxCollider",
					&Desc, &m_pComPxBoxCollider)))
				return E_FAIL;
		}
	}

	return S_OK;
}

void CTestSquareStep::FixedUpdate(_float fTimeDelta)
{
	_float3 vPosition = GetTransform().GetPosition();
	const _float fHeightDelta = m_fTargetY - vPosition.y;
	_bool bHeightChanged = false;
	if (std::abs(fHeightDelta) > FLT_EPSILON && m_fMoveSpeed > 0.f)
	{
		const _float fMaxMove = m_fMoveSpeed * fTimeDelta;
		vPosition.y += std::clamp(
			fHeightDelta, -fMaxMove, fMaxMove);
		GetTransform().SetPosition(vPosition);
		bHeightChanged = true;
	}

	if (!m_pComPxRigidBody || !bHeightChanged)
		return;

	m_pComPxRigidBody->SetKinematicTarget(
		GetTransform().GetPosition(),
		GetTransform().GetQuaternion());
}

void CTestSquareStep::SetHeightTarget(
	_float fTargetY, _float fMoveSpeed)
{
	m_fTargetY = fTargetY;
	m_fMoveSpeed = std::max(fMoveSpeed, 0.f);
}

void CTestSquareStep::LateUpdate(_float)
{
	GetTransform().Update();

	if (!m_pComModelInstance || !m_pComModelInstance->GetModel())
		return;

	if (!CGameInstance::Get().IsInstancingEnabled())
	{
		CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
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

HRESULT CTestSquareStep::Render(
	ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
	if (!pContext || !m_pComModelInstance || !m_pComCBufferPerObject ||
		!m_pResVertexShader || !m_pResPixelShader)
		return E_FAIL;

	CB_PER_OBJECT cbPerObject{};
	cbPerObject.matWorld = *GetTransform().GetCombinedWorldMatrix();
	XMStoreFloat4x4(
		&cbPerObject.matWVP,
		GetTransform().GetLoadedCombinedWorldMatrix() * ctx.matViewProj);
	if (FAILED(m_pComCBufferPerObject->MapDiscard(
		pContext, &cbPerObject, sizeof(cbPerObject))))
		return E_FAIL;

	pContext->VSSetConstantBuffers(
		0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	pContext->PSSetConstantBuffers(
		0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	pContext->IASetInputLayout(m_pResVertexShader->GetInputLayout().Get());
	pContext->VSSetShader(m_pResVertexShader->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(m_pResPixelShader->GetPixelShader().Get(), nullptr, 0);

	const auto pModel = m_pComModelInstance->GetModel();
	if (!pModel)
		return E_FAIL;

	for (uint32_t i = 0; i < pModel->Get_NumMeshes(); ++i)
	{
		const auto& pVIBuffer = pModel->GetMeshes()[i];
		ID3D11Buffer* pVertexBuffer = pVIBuffer->GetVertexBuffer().Get();
		const uint32_t iStride = pVIBuffer->GetVertexStride();
		const uint32_t iOffset = 0;
		pContext->IASetVertexBuffers(
			0, 1, &pVertexBuffer, &iStride, &iOffset);
		pContext->IASetIndexBuffer(
			pVIBuffer->GetIndexBuffer().Get(), pVIBuffer->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(pVIBuffer->GetPrimitiveType());

		m_pComModelInstance->Bind_Textures(pContext, i);
		m_pComModelInstance->Bind_Materials(
			pContext, { 1.f, 1.f, 1.f }, 0.f,
			{ 1.f, 1.f, 1.f }, 0.f, 1.f);
		pContext->DrawIndexed(pVIBuffer->GetNumIndices(), 0, 0);
	}

	return S_OK;
}

UPtr<CTestSquareStep> CTestSquareStep::Create()
{
	auto pInstance = ToUPtr(new CTestSquareStep{});
	if (FAILED(pInstance->InitializePrototype()))
		return nullptr;
	return pInstance;
}

UPtr<CPrototype> CTestSquareStep::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CTestSquareStep{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
		return nullptr;
	return pInstance;
}
