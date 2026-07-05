#include "pch.h"
#include "FlipBook.h"
#include "GameInstance.h"
#include "CameraObject.h"
#include "Resources.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "Resources.h"

NS_USING(Client)

CFlipBook::CFlipBook()
{
}

CFlipBook::~CFlipBook()
{
}

HRESULT CFlipBook::Initialize(void* pArg)
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
	m_UIType = ETOUI(UI_TYPE::FLIPBOOK);

	return S_OK;
}

void CFlipBook::PriorityUpdate(E::_float fTimeDelta)
{
}

void CFlipBook::Update(E::_float fTimeDelta)
{
	CUIObject::Update(fTimeDelta);

	m_fPadding = 2 / cellsize;
	m_Columns = static_cast<int>(std::round(std::sqrt(m_TotalFrame)));
	m_Rows = static_cast<int>(std::round(std::sqrt(m_TotalFrame)));

	if (m_Loop == false && m_CurrentFrame == m_TotalFrame)
		return;

	if (m_CurrentFrame % m_iPuaseFrame == 0 && m_fPauseSumTime < m_fPauseTime && m_CurrentFrame != 0 && m_isPause)
	{
		m_fPauseSumTime += fTimeDelta;
	}
	else
	{
		m_fPauseSumTime = 0.f;
		m_fSumTime += fTimeDelta;

		uint32_t frameCount = (m_TotalFrame - m_StartFrame + 1);
		float delta = m_fDuration / frameCount;

		if (m_fSumTime >= delta)
		{
			m_fSumTime = 0.f;
			m_CurrentFrame = (m_CurrentFrame + 1) % frameCount;
		}
	}

	m_curColum = m_CurrentFrame % m_Columns;
	m_curRow = m_CurrentFrame / m_Rows;

	m_texcoord = { m_curColum / (float)m_Columns + m_fPadding, m_curRow / (float)m_Rows + m_fPadding };
	m_uvSize = { 1 / (float)m_Columns - m_fPadding * 2 , 1 / (float)m_Rows - m_fPadding * 2 };
}

void CFlipBook::LateUpdate(E::_float fTimeDelta)
{
	E::CGameInstance::Get().AddRenderObject(E::RENDERGROUP::UI, this);
	GetTransform().Update();
}

HRESULT CFlipBook::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	std::string currentLevel = "LEVEL_UIEDITOR";

	//VS_QuadTex
	const auto& vs = E::CGameInstance::Get().GetResourceFirst<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_QuadTexFlipBook");
	const auto& ps = E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_QuadTexFlipBook");
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
		perUI.texCoord = m_texcoord;
		perUI.uvSize = m_uvSize;
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

		const auto& sampler = E::CGameInstance::GetConst().GetResourceFirst<E::CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
		pContext->PSSetSamplers(0, 1, sampler->GetSamplerState().GetAddressOf());
	}

	pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);



	return S_OK;
}

E::UPtr<CFlipBook> CFlipBook::Create()
{
	auto pInstance = E::ToUPtr(new CFlipBook{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CFlipBook");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CFlipBook::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CFlipBook{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CFlipBook");
		return nullptr;
	}

	return pInstance;
}
