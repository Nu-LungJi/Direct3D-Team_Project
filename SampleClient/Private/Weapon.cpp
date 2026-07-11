#include "pch.h"
#include "Weapon.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComStaticModelInstance.h"
#include "Resources.h"
#include "GameInstance.h"
NS_USING(Client)

CWeapon::CWeapon()
	: CGameObject{}
{
}

CWeapon::~CWeapon()
{
}

void CWeapon::UpdateGUI()
{
	CGameObject::UpdateGUI();

}

HRESULT CWeapon::InitializePrototype(void* pArg)
{

	m_pResVertexNonAnimShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim");
	if (FAILED(m_pResVertexNonAnimShader->Load()))
	{
		return E_FAIL;
	}
	m_pResPixelNonAnimShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim");
	if (FAILED(m_pResPixelNonAnimShader->Load()))
	{
		return E_FAIL;
	}


	return S_OK;
}

HRESULT CWeapon::Initialize(void* pArg)
{
	if (FAILED(CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	}

	{
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject", &Desc, &m_pComCBufferPerObject)))
		{
			return E_FAIL;
		};
	}

	{
		CComStaticModelInstance::DESC Desc{};
		Desc.sGroupTag = "TEST";
		Desc.sResTag = "Static_Axe_Model_Resource";

		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_StaticModelInstance", "ComCModelIntance", &Desc, &m_pComModelInstance)))
		{
			return E_FAIL;
		};
	}




	return S_OK;
}

void CWeapon::PriorityUpdate(E::_float fTimeDelta)
{
}

void CWeapon::Update(E::_float fTimeDelta)
{

}

void CWeapon::LateUpdate(E::_float fTimeDelta)
{
	GetTransform().Update();
	CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
}

HRESULT CWeapon::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	{
		E::CB_PER_OBJECT cbPerObject{};
		cbPerObject.matWorld = *GetTransform().GetCombinedWorldMatrix();
		XMStoreFloat4x4(&cbPerObject.matWVP, GetTransform().GetLoadedCombinedWorldMatrix() * ctx.matViewProj);
		if (FAILED(m_pComCBufferPerObject->MapDiscard(pContext, &cbPerObject, sizeof(cbPerObject))))
		{
			return E_FAIL;
		}
		pContext->VSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	}

	const auto& vs = m_pResVertexNonAnimShader;
	const auto& ps = m_pResPixelNonAnimShader;

	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

	auto pModel = m_pComModelInstance->GetModel();

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
			m_pComModelInstance->Bind_Materials(pContext, { 1.f, 1.f, 1.f }, 0.f, 1.f);	// EmissiveColor -> EmissiveIntensity -> Alpha 순
		}

		pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);
	}


	return S_OK;
}

E::UPtr<CWeapon> CWeapon::Create()
{
	auto pInstance = E::ToUPtr(new CWeapon{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CWeapon");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CWeapon::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CWeapon{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CWeapon");
		return nullptr;
	}

	return pInstance;
}
