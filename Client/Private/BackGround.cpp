#include "pch.h"
#include "BackGround.h"
#include "GameInstance.h"
#include "Resources.h"

NS_USING(Client)

CBackGround::CBackGround()
{
}


CBackGround::~CBackGround()
{
}

HRESULT CBackGround::Initialize(void* pArg)
{
	auto		pDesc = static_cast<CUIObject::UIOBJECT_DESC*>(pArg);
	pDesc->fSizeX = g_iWinSizeX;
	pDesc->fSizeY = 200.f;

	pDesc->fX = g_iWinSizeX * 0.5f;
	pDesc->fY = g_iWinSizeY - pDesc->fSizeY * 0.5f;

	if (FAILED(CUIObject::Initialize(pDesc)))
		return E_FAIL;

	return S_OK;
}

void CBackGround::PriorityUpdate(E::_float fTimeDelta)
{
}

void CBackGround::Update(E::_float fTimeDelta)
{
}

void CBackGround::LateUpdate(E::_float fTimeDelta)
{
	E::CGameInstance::Get().AddRenderObject(E::RENDERGROUP::UI, this);
	GetTransform().Update();
}

HRESULT CBackGround::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	//VS_QuadTex
	const auto& vs = E::CGameInstance::Get().GetResourceFirst<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_QuadTex");
	const auto& ps = E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_QuadTex");
	const auto& viBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResQuadTexBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_QuadTex");

	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

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
		//auto pUICam = E::CGameInstance::Get().GetActiveUICamera();
		{
			auto pCbPerObject = E::CGameInstance::Get().GetResourceFirst<E::CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_PerObject");
			D3D11_MAPPED_SUBRESOURCE mappedSubResource;
			if (SUCCEEDED(pContext->Map(pCbPerObject->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource)))
			{
				
				E::CB_PER_OBJECT cbPerObject{};
				cbPerObject.matWorld = *GetTransform().GetWorldMatrix();
				XMStoreFloat4x4(&cbPerObject.matWVP, GetTransform().GetLoadedWorldMatrix() * ctx.matProj);

				memcpy(mappedSubResource.pData, &cbPerObject, sizeof(cbPerObject));
				pContext->Unmap(pCbPerObject->GetCBuffer().Get(), 0);
			}
			pContext->VSSetConstantBuffers(0, 1, pCbPerObject->GetCBuffer().GetAddressOf());
			pContext->PSSetConstantBuffers(0, 1, pCbPerObject->GetCBuffer().GetAddressOf());
		}
	}
	{
		const auto& srv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>("LEVEL_LOGO", "TEX_SHM");
		pContext->PSSetShaderResources(0, 1, srv->GetSRV().GetAddressOf());
	}

	pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);



	return S_OK;
}

E::UPtr<CBackGround> CBackGround::Create()
{
	auto pInstance = E::ToUPtr(new CBackGround{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBackGround");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CBackGround::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBackGround{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBackGround");
		return nullptr;
	}

	return pInstance;
}
