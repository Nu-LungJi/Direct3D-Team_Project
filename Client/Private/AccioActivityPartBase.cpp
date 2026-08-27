#include "pch.h"
#include "AccioActivityPartBase.h"

#include "ComConstantBuffer.h"
#include "ComStaticModelInstance.h"
#include "GameInstance.h"
#include "Resources.h"

NS_USING(Client)

CAccioActivityPartBase::CAccioActivityPartBase() = default;

CAccioActivityPartBase::CAccioActivityPartBase(
	const CAccioActivityPartBase& prototype)
	: CGameObject{ prototype }
	, m_pResVertexShader{ prototype.m_pResVertexShader }
	, m_pResPixelShader{ prototype.m_pResPixelShader }
{
}

HRESULT CAccioActivityPartBase::InitializePrototype(void*)
{
	m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim");
	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(
		TAG_RES_GRP_PERMANENT_SHADER, TAG_RES_PERMANENT_NONBLENDSHADER);
	if (!m_pResVertexShader || !m_pResPixelShader ||
		FAILED(m_pResVertexShader->Load()) || FAILED(m_pResPixelShader->Load()))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CAccioActivityPartBase::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc || FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	GetTransform().SetPosition(pDesc->vInitialPosition);
	GetTransform().SetRotationEuler(pDesc->vInitialRotation);
	GetTransform().SetScale(pDesc->vInitialScale);
	GetTransform().Update();

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
		desc.sResTag = GetModelResourceTag();
		if (FAILED(AddComponentFromProto(
			"PERMANENT", "Prototype_Component_StaticModelInstance",
			"ComModelInstance", &desc, &m_pComModelInstance)))
		{
			return E_FAIL;
		}
	}

	return S_OK;
}

void CAccioActivityPartBase::LateUpdate(_float)
{
	GetTransform().Update();

	if (!m_pComModelInstance || !m_pComModelInstance->GetModel())
		return;

	if (!CGameInstance::Get().IsInstancingEnabled())
	{
		CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND_MAPMESH, this);
		return;
	}

	CGameInstance::Get().Add_Instance(
		m_pComModelInstance,
		*GetTransform().GetCombinedWorldMatrix());
}

HRESULT CAccioActivityPartBase::Render_Instanced(
	ID3D11DeviceContext* pContext,
	const RENDER_CTX&,
	const MODEL_INSTANCE_BATCH& batch)
{
	return m_pComModelInstance
		? m_pComModelInstance->RenderDynamicInstances(pContext, batch)
		: E_FAIL;
}

HRESULT CAccioActivityPartBase::Render(
	ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
	if (CGameInstance::Get().IsInstancingEnabled())
		return S_OK;

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
		ETOUI(B_SLOTNUMBER::PER_OBJECT), 1,
		m_pComCBufferPerObject->GetAdressOfBuffer());
	pContext->PSSetConstantBuffers(
		ETOUI(B_SLOTNUMBER::PER_OBJECT), 1,
		m_pComCBufferPerObject->GetAdressOfBuffer());
	pContext->IASetInputLayout(m_pResVertexShader->GetInputLayout().Get());
	pContext->VSSetShader(m_pResVertexShader->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(m_pResPixelShader->GetPixelShader().Get(), nullptr, 0);

	const auto& pModel = m_pComModelInstance->GetModel();
	for (uint32_t meshIndex = 0; meshIndex < pModel->Get_NumMeshes(); ++meshIndex)
	{
		const auto& mesh = pModel->GetMeshes()[meshIndex];
		ID3D11Buffer* vertexBuffer = mesh->GetVertexBuffer().Get();
		const uint32_t stride = mesh->GetVertexStride();
		const uint32_t offset{};
		pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		pContext->IASetIndexBuffer(mesh->GetIndexBuffer().Get(), mesh->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(mesh->GetPrimitiveType());
		m_pComModelInstance->Bind_Textures(pContext, meshIndex);
		m_pComModelInstance->Bind_Materials(
			pContext, { 1.f, 1.f, 1.f }, 0.f, { 1.f, 1.f, 1.f }, 0.f, 1.f);
		pContext->DrawIndexed(mesh->GetNumIndices(), 0, 0);
	}

	return S_OK;
}

bool CAccioActivityPartBase::IsOcclusionCullable() const
{
	return m_pComModelInstance && m_pComModelInstance->GetModel();
}

bool CAccioActivityPartBase::GetOcclusionBounds(BoundingBox& outBounds) const
{
	if (!m_pComModelInstance || !m_pComModelInstance->GetModel() ||
		!m_pComModelInstance->GetModel()->HasLocalBounds())
	{
		return false;
	}

	m_pComModelInstance->GetModel()->GetLocalBounds().Transform(
		outBounds, GetTransform().GetLoadedCombinedWorldMatrix());
	return true;
}
