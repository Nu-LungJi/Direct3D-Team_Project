#include "pch.h"
#include "SkyCloudyCube.h"
#include "Client_Defines.h"
#include "ComConstantBuffer.h"
#include "GameInstance.h"
#include "Resources.h"

NS_USING(Client)

CSkyCloudyCube::CSkyCloudyCube()
{
}

CSkyCloudyCube::~CSkyCloudyCube()
{
}

HRESULT CSkyCloudyCube::InitializePrototype(void* pArg)
{
    return __super::InitializePrototype(pArg);
}

HRESULT CSkyCloudyCube::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	m_pCubeBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResCubeColBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_SkyCube");
	m_pCubeTexture = E::CGameInstance::Get().GetResourceFirst<E::CResTextureCubeMap>("SKYBOX", "TEX_SkyCloudyCube");
	if (!m_pCubeBuffer || !m_pCubeTexture)
		return E_FAIL;

	{
		E::CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject", &Desc, &m_pComCBufferPerObject)))
		{
			return E_FAIL;
		};
	}

    return S_OK;
}

void CSkyCloudyCube::PriorityUpdate(E::_float fTimeDelta)
{
	__super::PriorityUpdate(fTimeDelta);
}

void CSkyCloudyCube::Update(E::_float fTimeDelta)
{
	__super::Update(fTimeDelta);
}

void CSkyCloudyCube::LateUpdate(E::_float fTimeDelta)
{
	__super::LateUpdate(fTimeDelta);
	E::CGameInstance::Get().AddRenderObject(E::RENDERGROUP::SKYBOX, this);
}

HRESULT CSkyCloudyCube::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
	const auto& gameInstance = E::CGameInstance::Get();
	const auto& vs = gameInstance.GetResourceFirst<E::CResVertexShader>("SKYBOX", "VS_SkyCloudy");
	const auto& ps = gameInstance.GetResourceFirst<E::CResPixelShader>("SKYBOX", "PS_SkyCloudy");
	const auto& depthState = gameInstance.GetResourceFirst<E::CResDepthStencilState>(TAG_RES_GRP_PERMANENT_STATE, "DS_NO_DEPTHWRITE");
	const auto& rasterizer = gameInstance.GetResourceFirst<E::CResRasterizerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_NOCULL);
	const auto& sampler = gameInstance.GetResourceFirst<E::CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
	if (!vs || !ps || !depthState || !rasterizer || !sampler)
		return E_FAIL;

	E::CB_PER_OBJECT perObject{};
	XMStoreFloat4x4(&perObject.matWorld, XMMatrixIdentity());
	E::_matrix viewNoTranslation = ctx.matView;
	viewNoTranslation.r[3] = XMVectorSet(0.f, 0.f, 0.f, 1.f);
	XMStoreFloat4x4(&perObject.matWVP,viewNoTranslation * ctx.matProj);
	if (FAILED(m_pComCBufferPerObject->MapDiscard(pContext, &perObject, sizeof(perObject))))
		return E_FAIL;

	ComPtr<ID3D11DepthStencilState> previousDepthState{};
	ComPtr<ID3D11RasterizerState> previousRasterizer{};
	UINT previousStencilRef = 0;
	pContext->OMGetDepthStencilState(previousDepthState.GetAddressOf(), &previousStencilRef);
	pContext->RSGetState(previousRasterizer.GetAddressOf());

	pContext->OMSetDepthStencilState(depthState->GetDepthStencilState().Get(), 0);
	pContext->RSSetState(rasterizer->GetRasterizerState().Get());
	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

	ID3D11Buffer* vertexBuffer = m_pCubeBuffer->GetVertexBuffer().Get();
	const UINT stride = m_pCubeBuffer->GetVertexStride();
	const UINT offset = 0;
	pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	pContext->IASetIndexBuffer(m_pCubeBuffer->GetIndexBuffer().Get(), m_pCubeBuffer->GetIndexFormat(), 0);
	pContext->IASetPrimitiveTopology(m_pCubeBuffer->GetPrimitiveType());
	pContext->VSSetConstantBuffers(ETOUI(E::B_SLOTNUMBER::PER_OBJECT), 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	pContext->PSSetShaderResources(0, 1, m_pCubeTexture->GetSRV().GetAddressOf());
	pContext->PSSetSamplers(0, 1, sampler->GetSamplerState().GetAddressOf());
	pContext->DrawIndexed(m_pCubeBuffer->GetNumIndices(), 0, 0);

	ID3D11ShaderResourceView* nullSrv = nullptr;
	pContext->PSSetShaderResources(0, 1, &nullSrv);
	pContext->OMSetDepthStencilState(previousDepthState.Get(), previousStencilRef);
	pContext->RSSetState(previousRasterizer.Get());
    return S_OK;
}

E::UPtr<CSkyCloudyCube> CSkyCloudyCube::Create()
{
	auto pInstance = E::ToUPtr(new CSkyCloudyCube{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CSkyCloudyCube");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CSkyCloudyCube::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CSkyCloudyCube{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CSkyCloudyCube");
		return nullptr;
	}

	return pInstance;
}
