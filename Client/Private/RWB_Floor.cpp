#include "RWB_Floor.h"
#include "pch.h"
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

CRWB_Floor::CRWB_Floor() : CGameObject{} {}
CRWB_Floor::CRWB_Floor(const CRWB_Floor& prototype)
	: CGameObject{ prototype }
	, m_pResVertexShader{ prototype.m_pResVertexShader }
	, m_pResPixelShader{ prototype.m_pResPixelShader }
{
}

HRESULT CRWB_Floor::InitializePrototype(void* pArg) {
	m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim");
	if (!m_pResVertexShader || FAILED(m_pResVertexShader->Load()))	return E_FAIL;

	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, TAG_RES_PERMANENT_NONBLENDSHADER);
	if (!m_pResPixelShader || FAILED(m_pResPixelShader->Load()))	return E_FAIL;

	return S_OK;
}

HRESULT CRWB_Floor::Initialize(void* pArg) {
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
		modelDesc.sGroupTag = LEVEL::BOSS_CHARLES_ROOKWOOD;
		modelDesc.sResTag = "Static_RWBFloor_Resource";
		if (FAILED(AddComponentFromProto(
			"PERMANENT", "Prototype_Component_StaticModelInstance", "ComModelInstance",
			&modelDesc, &m_pComModelInstance)))
			return E_FAIL;
	}

	if (m_pResReflectionPixelShader = CGameInstance::Get().AddResourceT<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_Reflection", "./ShaderFiles/Decal/Shader_RWBFloor.hlsl")) {
		if (FAILED(m_pResReflectionPixelShader->Load()))    return E_FAIL;
	}

	return S_OK;
}

void CRWB_Floor::PriorityUpdate(E::_float fTimeDelta) {

}
void CRWB_Floor::Update(E::_float fTimeDelta) {

}
void CRWB_Floor::LateUpdate(E::_float fTimeDelta) {
	GetTransform().Update();

	if (!m_pComModelInstance || !m_pComModelInstance->GetModel())
		return;

	CGameInstance::Get().AddShadowRenderGroup(ACTORTYPE::DYNAMIC, this);
	CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);

	if (CGameInstance::Get().GetCurrentLevelID() != ETOUI(LEVEL::BOSS_CHARLES_ROOKWOOD)) return;

	constexpr float ReflectionPlaneOffset = 0.f;
	XMVECTOR FloorUp = XMVector3Normalize(GetTransform().GetState(STATE::UP));
	XMVECTOR FloorPoint = GetTransform().GetLoadedPostion() + FloorUp * ReflectionPlaneOffset;

	PLANAR_REFLECTION_DESC Desc{};
	XMStoreFloat3(&Desc.FloorPoint, FloorPoint);
	XMStoreFloat3(&Desc.FloorNormal, FloorUp);
	Desc.enabled = 1.f;

	CGameInstance::Get().Set_PlanarReflection(Desc);
}

HRESULT CRWB_Floor::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) {
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

		const bool isReflective = (model->GetMeshes()[i]->Get_MaterialIndex() == 4); // material_9

		pContext->PSSetShader(isReflective ? m_pResReflectionPixelShader->GetPixelShader().Get() : m_pResPixelShader->GetPixelShader().Get(), nullptr, 0);

		pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);
	}

	return S_OK;
}

HRESULT CRWB_Floor::Render_Shadow(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) {
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

bool CRWB_Floor::GetShadowBounds(BoundingBox& OutBounds) const {
	if (m_pComModelInstance == nullptr)	return false;

	const auto& Model = m_pComModelInstance->GetModel();
	if (Model == nullptr || !Model->HasLocalBounds())		return false;

	Model->GetLocalBounds().Transform(OutBounds, GetTransform().GetLoadedCombinedWorldMatrix());

	return true;
}
E::UPtr<CRWB_Floor> CRWB_Floor::Create() {
	auto pInstance = E::ToUPtr(new CRWB_Floor{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CRWB_Floor");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CRWB_Floor::Clone(void* pArg) {
	auto	pInstance = E::ToUPtr(new CRWB_Floor{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CRWB_Floor");
		return nullptr;
	}

	return pInstance;
}
