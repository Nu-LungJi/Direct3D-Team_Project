#include "pch.h"
#include "WaterWheel.h"
#include "Client_Defines.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComPxConvexCollider.h"
#include "ComPxDistanceJoint.h"
#include "ComPxFixedJoint.h"
#include "ComPxRigidBody.h"
#include "ComStaticModelInstance.h"
#include "GameInstance.h"
#include "ResPhysXConvexGeometry.h"
#include "ResPhysXMaterial.h"
#include "Resources.h"

NS_USING(Client)

CWaterWheel::CWaterWheel() : CGameObject{} {}
CWaterWheel::CWaterWheel(const CWaterWheel& prototype)
	: CGameObject{ prototype }
	, m_pResVertexShader{ prototype.m_pResVertexShader }
	, m_pResPixelShader{ prototype.m_pResPixelShader }
{
}
HRESULT CWaterWheel::InitializePrototype(void* pArg) {

	m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim");
	if (!m_pResVertexShader || FAILED(m_pResVertexShader->Load()))	return E_FAIL;
	
	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, TAG_RES_PERMANENT_NONBLENDSHADER);
	if (!m_pResPixelShader || FAILED(m_pResPixelShader->Load()))	return E_FAIL;

	return S_OK;
}

HRESULT CWaterWheel::Initialize(void* pArg) {
	if (FAILED(CGameObject::Initialize(pArg))) return E_FAIL;

	const auto* desc = static_cast<DESC*>(pArg);
	GetTransform().SetPosition(desc->vInitialPosition);
	GetTransform().SetRotationEuler(desc->vInitialRotation);
	GetTransform().SetScale(desc->vInitialScale);

	{
		CComConstantBuffer::DESC bufferDesc{};
		bufferDesc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto(
			"PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject",
			&bufferDesc, &m_pComCBufferPerObject)))
			return E_FAIL;
	}

	{
		CComStaticModelInstance::DESC modelDesc{};
		modelDesc.sGroupTag = LEVEL::HOGWART_WORLD;
		modelDesc.sResTag = "Static_WaterWheel_Resource";
		if (FAILED(AddComponentFromProto(
			"PERMANENT", "Prototype_Component_StaticModelInstance", "ComModelInstance",
			&modelDesc, &m_pComModelInstance)))
			return E_FAIL;
	}

	return S_OK;
}

void CWaterWheel::PriorityUpdate(E::_float fTimeDelta)
{
}

void CWaterWheel::Update(E::_float fTimeDelta) {
	m_fWheelRotation = m_fWheelRotation - std::fmod(fTimeDelta * 30.f, 180.f);
	GetTransform().SetRotationEuler(_float3(m_fWheelRotation, 48.f, 0.f));
}

void CWaterWheel::LateUpdate(E::_float fTimeDelta) {
	GetTransform().Update();

	if (!m_pComModelInstance || !m_pComModelInstance->GetModel())
		return;

	/*----------- 광윤 추가 -----------*/
	CGameInstance::Get().AddShadowRenderGroup(ACTORTYPE::DYNAMIC, this);
	/*---------------------------------*/

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

	CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND_MAPMESH, this);
}

HRESULT CWaterWheel::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) {
	if (!pContext || !m_pComModelInstance || !m_pComCBufferPerObject ||
		!m_pResVertexShader || !m_pResPixelShader)
		return E_FAIL;

	E::CB_PER_OBJECT cbPerObject{};
	cbPerObject.matWorld = *GetTransform().GetCombinedWorldMatrix();
	XMStoreFloat4x4(&cbPerObject.matWVP,
		GetTransform().GetLoadedCombinedWorldMatrix() * ctx.matViewProj);
	if (FAILED(m_pComCBufferPerObject->MapDiscard(
		pContext, &cbPerObject, sizeof(cbPerObject))))
		return E_FAIL;
	pContext->VSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	pContext->PSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());

	pContext->IASetInputLayout(m_pResVertexShader->GetInputLayout().Get());
	pContext->VSSetShader(m_pResVertexShader->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(m_pResPixelShader->GetPixelShader().Get(), nullptr, 0);

	const auto model = m_pComModelInstance->GetModel();
	if (!model)
		return E_FAIL;

	for (uint32_t i = 0; i < model->Get_NumMeshes(); ++i)
	{
		const auto& viBuffer = model->GetMeshes()[i];
		ID3D11Buffer* vertexBuffer = viBuffer->GetVertexBuffer().Get();
		const uint32_t stride = viBuffer->GetVertexStride();
		const uint32_t offset = 0;
		pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		pContext->IASetIndexBuffer(
			viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

		m_pComModelInstance->Bind_Textures(pContext, i);
		m_pComModelInstance->Bind_Materials(
			pContext, { 1.f, 1.f, 1.f }, 0.f, { 1.f, 1.f, 1.f }, 0.f, 1.f);
		pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);
	}

	return S_OK;
}

HRESULT CWaterWheel::Render_Shadow(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	if (!pContext || !m_pComModelInstance || !m_pComCBufferPerObject)
		return E_FAIL;

	E::CB_PER_OBJECT cbPerObject{};
	cbPerObject.matWorld = *GetTransform().GetCombinedWorldMatrix();
	XMStoreFloat4x4(&cbPerObject.matWVP, GetTransform().GetLoadedCombinedWorldMatrix() * ctx.matViewProj);
	if (FAILED(m_pComCBufferPerObject->MapDiscard(pContext, &cbPerObject, sizeof(cbPerObject))))	return E_FAIL;

	pContext->VSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	pContext->PSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());

	const auto model = m_pComModelInstance->GetModel();
	if (!model)	return E_FAIL;

	for (uint32_t i = 0; i < model->Get_NumMeshes(); ++i)
	{
		const auto& viBuffer = model->GetMeshes()[i];
		ID3D11Buffer* vertexBuffer = viBuffer->GetVertexBuffer().Get();
		const uint32_t stride = viBuffer->GetVertexStride();
		const uint32_t offset = 0;

		pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());
		pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);
	}

	ID3D11ShaderResourceView* pSRVs[4] = { nullptr, nullptr, nullptr, nullptr };
	pContext->PSSetShaderResources(0, 4, pSRVs);

	return S_OK;
}

bool CWaterWheel::GetShadowBounds(BoundingBox& OutBounds) const
{
	if (m_pComModelInstance == nullptr)	return false;

	const auto& Model = m_pComModelInstance->GetModel();
	if (Model == nullptr || !Model->HasLocalBounds())		return false;

	Model->GetLocalBounds().Transform(OutBounds, GetTransform().GetLoadedCombinedWorldMatrix());

	return true;
}

E::UPtr<CWaterWheel> CWaterWheel::Create() {
	auto pInstance = E::ToUPtr(new CWaterWheel{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CWaterWheel");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CWaterWheel::Clone(void* pArg) {
	auto	pInstance = E::ToUPtr(new CWaterWheel{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CWaterWheel");
		return nullptr;
	}

	return pInstance;
}
