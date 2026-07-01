#include "pch.h"
#include "CTexUI.h"
#include "GameInstance.h"
#include "CameraObject.h"
#include "Resources.h"

NS_USING(Client)

CTexUI::CTexUI()
{

}

CTexUI::~CTexUI()
{
}

HRESULT CTexUI::Initialize(void* pArg)
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

void CTexUI::PriorityUpdate(E::_float fTimeDelta)
{
}

void CTexUI::Update(E::_float fTimeDelta)
{

}

void CTexUI::LateUpdate(E::_float fTimeDelta)
{
	E::CGameInstance::Get().AddRenderObject(E::RENDERGROUP::UI, this);
	GetTransform().Update();
}

HRESULT CTexUI::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
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

		const auto& sampler = E::CGameInstance::GetConst().GetResourceFirst<E::CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
		pContext->PSSetSamplers(0, 1, sampler->GetSamplerState().GetAddressOf());
	}

	pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);



	return S_OK;
}

E::UPtr<CTexUI> CTexUI::Create()
{
	auto pInstance = E::ToUPtr(new CTexUI{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CTexUI");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CTexUI::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CTexUI{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTexUI");
		return nullptr;
	}

	return pInstance;
}
