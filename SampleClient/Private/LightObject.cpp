#include "pch.h"
#include "LightObject.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComModelInstance.h"
#include "ComAnimator.h"
#include "Resources.h"
#include "GameInstance.h"

NS_USING(Client)

CLightObject::CLightObject() : CGameObject{} {}
CLightObject::~CLightObject()  {}

void CLightObject::UpdateGUI() {
    CGameObject::UpdateGUI();
}

HRESULT CLightObject::InitializePrototype(void* pArg) {
	//m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim");
	//if (FAILED(m_pResVertexShader->Load()))	return E_FAIL;
	//
	//m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim");
	//if (FAILED(m_pResPixelShader->Load()))	return E_FAIL;

	//m_pResVertexShader	= CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_PBR_BLEND");
	//if (FAILED(m_pResVertexShader->Load()))	return E_FAIL;
	//
	//m_pResPixelShader	= CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_PBR_BLEND");
	//if (FAILED(m_pResPixelShader->Load()))	return E_FAIL;

	m_pResSamplerState	= CGameInstance::Get().GetResourceFirst<CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
	if (!m_pResSamplerState)				return E_FAIL;

	if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>("LOBJ", "Model_Resource", CResModel::Create("./Resources/SampleClient/Models/Skeleton/LightObject/SK_LightObject.bin"))) {
		E::CResModel::DESC pDesc = { XMMatrixIdentity() };
		if (FAILED(res->Load(pDesc)))	return E_FAIL;
	}

	return S_OK;
}

HRESULT CLightObject::Initialize(void* pArg) {
	if (FAILED(CGameObject::Initialize(pArg)))	return E_FAIL;

	{
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject", &Desc, &m_pComCBufferPerObject)))
		{
			return E_FAIL;
		};
	}

	{
		CComModelInstance::DESC Desc{};
		Desc.sGroupTag = "LOBJ";
		Desc.sResTag = "Model_Resource";

		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ModelInstance", "ComCModelIntance", &Desc, &m_pComModelInstance)))
		{
			return E_FAIL;
		};
	}
	{
		CComAnimator::DESC DescAnim{};
		DescAnim.sComTag = "ComCModelIntance";

		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_Animator", "ComCModelAnimator", &DescAnim, &m_pModelAnimator)))
		{
			return E_FAIL;
		};
	}

	m_pComTransform->SetScale(XMVectorSet(100.f, 100.f, 100.f, 1.f));
	m_pComTransform->SetRotation(XMVectorSet(1.f, 0.f, 0.f, 1.f), +90.f);
	m_pComTransform->SetRotation(XMVectorSet(0.f, 0.f, 1.f, 1.f), +90.f);
	m_pComTransform->SetPosition(XMVectorSet(1.f, 3.f, 0.f, 1.f));

	return S_OK;
}

void CLightObject::PriorityUpdate(E::_float fTimeDelta) {

}
void CLightObject::Update(E::_float fTimeDelta) {

}
void CLightObject::LateUpdate(E::_float fTimeDelta) {
	GetTransform().Update();
	CGameInstance::Get().AddRenderObject(RENDERGROUP::BLEND, this);
}

HRESULT CLightObject::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	{
		E::CB_PER_OBJECT cbPerObject{};
		cbPerObject.matWorld = *GetTransform().GetCombinedWorldMatrix();
		XMStoreFloat4x4(&cbPerObject.matWVP, GetTransform().GetLoadedCombinedWorldMatrix() * ctx.matViewProj);
		if (FAILED(m_pComCBufferPerObject->MapDiscard(pContext, &cbPerObject, sizeof(cbPerObject))))	return E_FAIL;

		pContext->VSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	}

	auto pModel = m_pComModelInstance->GetModel();
	if (nullptr == pModel)	return E_FAIL;

	uint32_t	iNumMeshes = pModel->Get_NumMeshes();
	for (uint32_t i = 0; i < iNumMeshes; ++i) {
		const auto& viBuffer = pModel->GetMeshes()[i];

		ID3D11Buffer* vertexBuffers[] = {
			viBuffer->GetVertexBuffer().Get()
		};
		uint32_t strides[] = {
			viBuffer->GetVertexStride()
		};
		uint32_t offsets[] = {
			0
		};
		pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
		pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

		{
			m_pComModelInstance->Bind_Textures(pContext, i);
			m_pComModelInstance->Bind_Materials(pContext, { 1.f, 0.1f, 0.1f }, 10.f, 1.f);	// EmissiveColor -> EmissiveIntensity -> Alpha ��
		}

		pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);
	}

	ID3D11ShaderResourceView* pSRVs[1] = { nullptr };
	pContext->PSSetShaderResources(0, 1, pSRVs);
	pContext->PSSetShaderResources(1, 1, pSRVs);
	pContext->PSSetShaderResources(2, 1, pSRVs);
	pContext->PSSetShaderResources(3, 1, pSRVs);

	return S_OK;
}
E::UPtr<CLightObject>	CLightObject::Create()
{
	auto pInstance = E::ToUPtr(new CLightObject{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CLightObject");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype>	CLightObject::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CLightObject{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CLightObject");
		return nullptr;
	}

	return pInstance;
}
