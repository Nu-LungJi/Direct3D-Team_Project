#include "pch.h"
#include "AccioBall.h"

#include "ComConstantBuffer.h"
#include "ComPxRigidBody.h"
#include "ComPxSphereCollider.h"
#include "ComStaticModelInstance.h"
#include "GameInstance.h"
#include "ResPhysXMaterial.h"
#include "ResPhysXSphereGeometry.h"
#include "Resources.h"

NS_USING(Client)

CAccioBall::CAccioBall() = default;

CAccioBall::CAccioBall(const CAccioBall& prototype)
	: CGameObject{ prototype }
	, m_pResVertexShader{ prototype.m_pResVertexShader }
	, m_pResPixelShader{ prototype.m_pResPixelShader }
{
}

HRESULT CAccioBall::InitializePrototype(void*)
{
	m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelAnim");
	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelAnim");
	if (!m_pResVertexShader || !m_pResPixelShader ||
		FAILED(m_pResVertexShader->Load()) || FAILED(m_pResPixelShader->Load()))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CAccioBall::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc || FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	GetTransform().SetPosition(pDesc->vInitialPosition);
	GetTransform().SetRotationEuler(pDesc->vInitialRotation);
	GetTransform().SetScale(pDesc->vInitialScale);
	GetTransform().Update();
	m_vInitialPosition = pDesc->vInitialPosition;
	m_vInitialRotation = GetTransform().GetQuaternion();

	const _float fScale = std::max({
		std::abs(pDesc->vInitialScale.x),
		std::abs(pDesc->vInitialScale.y),
		std::abs(pDesc->vInitialScale.z),
		0.001f });
	m_fSphereRadius = std::max(pDesc->fSphereRadius * fScale, 0.001f);

	{
		CComConstantBuffer::DESC desc{};
		desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto(
			"PERMANENT", "Prototype_Component_ConstantBuffer",
			"ComCBufferPerObject", &desc, &m_pComCBufferPerObject)))
		{
			return E_FAIL;
		}
	}

	{
		CComStaticModelInstance::DESC desc{};
		desc.sGroupTag = pDesc->sResourceGroup;
		desc.sResTag = pDesc->sModelResourceTag;
		if (FAILED(AddComponentFromProto(
			"PERMANENT", "Prototype_Component_StaticModelInstance",
			"ComModelInstance", &desc, &m_pComModelInstance)))
		{
			return E_FAIL;
		}
	}

	{
		CComPxRigidBody::DESC desc{};
		desc.eType = CComPxRigidBody::TYPE::DYNAMIC;
		desc.fMass = std::max(pDesc->fMass, 0.001f);
		desc.vPosition = pDesc->vInitialPosition;
		desc.vRotation = GetTransform().GetQuaternion();
		if (FAILED(AddComponentFromProto(
			"PHYSX", "Prototype_Component_ComPxRigidBody",
			"ComPxRigidBody", &desc, &m_pComPxRigidBody)))
		{
			return E_FAIL;
		}
	}

	{
		m_pResPhysXMaterial = CResPhysXMaterial::CreateAndLoad({});
		m_pResSphereGeometry = CResPhysXSphereGeometry::CreateAndLoad(
			{ .fRadius = m_fSphereRadius });

		CComPxSphereCollider::DESC desc{};
		desc.pComPxRigidBody = m_pComPxRigidBody;
		desc.pResMaterial = m_pResPhysXMaterial;
		desc.pResSphereGeo = m_pResSphereGeometry;
		desc.bIsTrigger = false;
		desc.tFilter = pDesc->tFilter;
		if (!m_pResPhysXMaterial || !m_pResSphereGeometry ||
			FAILED(AddComponentFromProto(
				"PHYSX", "Prototype_Component_ComPxSphereCollider",
				"ComPxSphereCollider", &desc, &m_pComPxSphereCollider)))
		{
			return E_FAIL;
		}
	}

	if (!m_pComPxRigidBody->SetGravityEnabled(true) ||
		!m_pComPxRigidBody->SetLinearDamping(pDesc->fLinearDamping) ||
		!m_pComPxRigidBody->SetAngularDamping(pDesc->fAngularDamping) ||
		!m_pComPxRigidBody->SetMaxDepenetrationVelocity(5.f) ||
		!m_pComPxRigidBody->WakeUp())
	{
		return E_FAIL;
	}

	return S_OK;
}

void CAccioBall::LateUpdate(_float)
{
	UpdatePhysicData();
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

	MAPMESH_INSTANCE_DATA instanceData{};
	XMStoreFloat4x4(
		&instanceData.world,
		GetTransform().GetLoadedCombinedWorldMatrix());

	BoundingBox worldBounds{};
	pModel->GetLocalBounds().Transform(
		worldBounds,
		GetTransform().GetLoadedCombinedWorldMatrix());

	MAPMESH_OCCLUSION_DATA occlusionData{};
	occlusionData.worldCenter = worldBounds.Center;
	occlusionData.worldExtents = worldBounds.Extents;

	CGameInstance::Get().PushMapObjectInstance(
		pModel, instanceData, occlusionData);
}

HRESULT CAccioBall::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
	if (!pContext || !m_pComModelInstance || !m_pComCBufferPerObject ||
		!m_pResVertexShader || !m_pResPixelShader)
	{
		return E_FAIL;
	}

	CB_PER_OBJECT cbPerObject{};
	cbPerObject.matWorld = *GetTransform().GetCombinedWorldMatrix();
	XMStoreFloat4x4(
		&cbPerObject.matWVP,
		GetTransform().GetLoadedCombinedWorldMatrix() * ctx.matViewProj);
	if (FAILED(m_pComCBufferPerObject->MapDiscard(
		pContext, &cbPerObject, sizeof(cbPerObject))))
	{
		return E_FAIL;
	}
	pContext->VSSetConstantBuffers(
		0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	pContext->PSSetConstantBuffers(
		0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());

	pContext->IASetInputLayout(m_pResVertexShader->GetInputLayout().Get());
	pContext->VSSetShader(
		m_pResVertexShader->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(
		m_pResPixelShader->GetPixelShader().Get(), nullptr, 0);

	const auto& pModel = m_pComModelInstance->GetModel();
	for (uint32_t meshIndex = 0; meshIndex < pModel->Get_NumMeshes(); ++meshIndex)
	{
		const auto& mesh = pModel->GetMeshes()[meshIndex];
		ID3D11Buffer* vertexBuffer = mesh->GetVertexBuffer().Get();
		const uint32_t stride = mesh->GetVertexStride();
		const uint32_t offset{};
		pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		pContext->IASetIndexBuffer(
			mesh->GetIndexBuffer().Get(), mesh->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(mesh->GetPrimitiveType());
		m_pComModelInstance->Bind_Textures(pContext, meshIndex);
		m_pComModelInstance->Bind_Materials(
			pContext, { 1.f, 1.f, 1.f }, 0.f,
			{ 1.f, 1.f, 1.f }, 0.f, 1.f);
		pContext->DrawIndexed(mesh->GetNumIndices(), 0, 0);
	}

	return S_OK;
}

_bool CAccioBall::ApplyImpulse(const _float3& vImpulse)
{
	return m_pComPxRigidBody && m_pComPxRigidBody->AddImpulse(vImpulse);
}

_bool CAccioBall::ApplyTorque(const _float3& vTorque)
{
	return m_pComPxRigidBody && m_pComPxRigidBody->AddTorque(vTorque);
}

_bool CAccioBall::SetMotionTuning(
	_float fMass,
	_float fLinearDamping,
	_float fAngularDamping)
{
	return m_pComPxRigidBody &&
		m_pComPxRigidBody->SetMass(std::max(fMass, 0.001f)) &&
		m_pComPxRigidBody->SetLinearDamping(std::max(fLinearDamping, 0.f)) &&
		m_pComPxRigidBody->SetAngularDamping(std::max(fAngularDamping, 0.f));
}

_bool CAccioBall::ResetToInitialPose()
{
	if (!m_pComPxRigidBody ||
		!m_pComPxRigidBody->SetPose(m_vInitialPosition, m_vInitialRotation) ||
		!m_pComPxRigidBody->SetLinearVelocity({}) ||
		!m_pComPxRigidBody->SetAngularVelocity({}) ||
		!m_pComPxRigidBody->WakeUp())
	{
		return false;
	}

	GetTransform().SetPosition(m_vInitialPosition);
	GetTransform().SetQuaternion(m_vInitialRotation);
	GetTransform().Update();
	return true;
}

UPtr<CAccioBall> CAccioBall::Create()
{
	auto pInstance = ToUPtr(new CAccioBall{});
	if (FAILED(pInstance->InitializePrototype()))
		return nullptr;
	return pInstance;
}

UPtr<CPrototype> CAccioBall::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CAccioBall{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
		return nullptr;
	return pInstance;
}
