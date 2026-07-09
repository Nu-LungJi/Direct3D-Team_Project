#include "pch.h"
#include "UI_Item.h"
#include "GameInstance.h"
#include "CameraObject.h"
#include "Resources.h"

NS_USING(Client)

CUI_Item::CUI_Item()
{

}

CUI_Item::~CUI_Item()
{
}

HRESULT CUI_Item::Initialize(void* pArg)
{
	auto		pDesc = static_cast<CUIObject::UIOBJECT_DESC*>(pArg);

	if (FAILED(CUIObject::Initialize(pDesc)))
		return E_FAIL;


	{
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, "CB_PerUI" };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerUI", &Desc, &m_pComCBufferPerUI)))
		{
			return E_FAIL;
		};
	}

	m_UIType = ETOUI(UI_TYPE::TEXUI);

	return S_OK;
}

void CUI_Item::PriorityUpdate(E::_float fTimeDelta)
{
}

void CUI_Item::Update(E::_float fTimeDelta)
{
	CUIObject::Update(fTimeDelta);

	if (m_bMouseTracking)
	{
		_float2 mousePos = E::CGameInstance::Get().GetMousePos();
		m_fX = mousePos.x;
		m_fY = mousePos.y;
		CalcUICoord();
	}
}

void CUI_Item::LateUpdate(E::_float fTimeDelta)
{
	E::CGameInstance::Get().AddRenderObject(E::RENDERGROUP::UI, this);
	GetTransform().Update();
}

HRESULT CUI_Item::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	std::string currentLevel = "LEVEL_UIEDITOR";

	//VS_QuadTex
	const auto& vs = E::CGameInstance::Get().GetResourceFirst<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_QuadTexUI");
	const auto& ps = E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_QuadTexUI");
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
		E::CB_PER_UI perUI{};
		perUI.texCoord = { 0.f, 0.f };
		perUI.uvSize = { 0.f, 0.f };
		perUI.color = { 0.f, 0.f, 0.f, m_fAlpha };

		if (FAILED(m_pComCBufferPerUI->MapDiscard(pContext, &perUI, sizeof(perUI))))
		{
			return E_FAIL;
		}
		pContext->VSSetConstantBuffers(7, 1, m_pComCBufferPerUI->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(7, 1, m_pComCBufferPerUI->GetAdressOfBuffer());
	}

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
		const auto& srv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, m_sRestag);
		pContext->PSSetShaderResources(0, 1, srv->GetSRV().GetAddressOf());
	}

	pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);



	return S_OK;
}

void CUI_Item::Creating()
{
}

void CUI_Item::StartHovering()
{
}

void CUI_Item::Hovering()
{

}

void CUI_Item::EndHovering()
{
}

void CUI_Item::Ending()
{
}

E::UPtr<CUI_Item> CUI_Item::Create()
{
	auto pInstance = E::ToUPtr(new CUI_Item{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CTexUI");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CUI_Item::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CUI_Item{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CUI_Item");
		return nullptr;
	}

	return pInstance;
}
