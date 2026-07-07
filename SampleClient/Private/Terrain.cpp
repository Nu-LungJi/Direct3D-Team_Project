#include "pch.h"
#include "Terrain.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "Resources.h"
#include "GameInstance.h"

NS_USING(Client)

CTerrain::CTerrain()
	: CGameObject{}
{
}

CTerrain::~CTerrain()
{
}

HRESULT CTerrain::InitializePrototype(void* pArg)
{
	m_pResTerrainVIBuffer = CGameInstance::Get().GetResourceFirst<CResTerrainVIBuffer>("SAMPLE_CLIENT_BUFFER", "VIBUFFER_Terrain");
	//m_pResTerrainVIBuffer = CResTerrainVIBuffer::Create("./Resources/SampleClient/Textures/Terrain/Height.bmp");
	if (!m_pResTerrainVIBuffer)
	{
		return E_FAIL;
	}

	//"SAMPLE_CLIENT_TEX", "TEX2D_Terrain_Tile0"
	m_pResTerrainTexture2D = CGameInstance::Get().GetResourceFirst<CResTexture2D>("SAMPLE_CLIENT_TEX", "TEX2D_Terrain_Tile0");
	//m_pResTerrainTexture2D = CResTexture2D::Create("./Resources/SampleClient/Textures/Terrain/Tile0.dds");
	if (!m_pResTerrainTexture2D)
	{
		return E_FAIL;
	}

	m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>("SAMPLE_CLIENT_SHADER", "VS_VTX_NOR_TEX");
	if (FAILED(m_pResVertexShader->Load()))
	{
		return E_FAIL;
	}
	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>("SAMPLE_CLIENT_SHADER", "PS_VTX_NOR_TEX");
	if (FAILED(m_pResPixelShader->Load()))
	{
		return E_FAIL;
	}

	m_pResSamplerState = CGameInstance::Get().GetResourceFirst<CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
	if (!m_pResSamplerState)
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CTerrain::Initialize(void* pArg)
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


	return S_OK;
}

void CTerrain::PriorityUpdate(E::_float fTimeDelta)
{
}

void CTerrain::Update(E::_float fTimeDelta)
{
}

void CTerrain::LateUpdate(E::_float fTimeDelta)
{
	GetTransform().Update();
	CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
}

HRESULT CTerrain::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
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
	const auto& vs = m_pResVertexShader;
	const auto& ps = m_pResPixelShader;
	
	const auto& viBuffer = m_pResTerrainVIBuffer;
	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	if (ctx.pass == RENDERPASS::DEPTH) {			// 오류 메세지 ID3D11DeviceContext::DrawIndexed 제거용
		pContext->PSSetShader(nullptr, nullptr, 0);
	}
	else {
		pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);
	}
	

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
		auto MaterialConstantBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_MATERIAL");
		D3D11_MAPPED_SUBRESOURCE MRES;
		if (SUCCEEDED(pContext->Map(MaterialConstantBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
		{
			CB_MATERIAL   CMMAT;
			CMMAT.AlbedoColor = { 1.f, 1.f, 1.f, 0.5f };

			CMMAT.NormalIntensity = 1.f;
			CMMAT.RoughnessIntensity = 1.f;
			CMMAT.MetallicIntensity = 1.f;
			CMMAT.AmbientIntensity = 1.f;
			CMMAT.SpecularIntensity = 1.f;

			CMMAT.EmissiveColor = { 1.f, 0.f, 0.f };
			CMMAT.EmissiveIntensity = 1.f;

			memcpy(MRES.pData, &CMMAT, sizeof(CB_MATERIAL));
			pContext->Unmap(MaterialConstantBuffer->GetCBuffer().Get(), 0);
		}
		pContext->PSSetConstantBuffers(3, 1, MaterialConstantBuffer->GetCBuffer().GetAddressOf());
	}
	{
   auto MaterialConstantBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_MATERIAL");
   D3D11_MAPPED_SUBRESOURCE MRES;
   if (SUCCEEDED(pContext->Map(MaterialConstantBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
   {
      CB_MATERIAL   CMMAT;
      CMMAT.AlbedoColor = { 1.f, 1.f, 1.f, 0.5f };

      CMMAT.NormalIntensity = 1.f;
      CMMAT.RoughnessIntensity = 1.f;
      CMMAT.MetallicIntensity = 1.f;
      CMMAT.AmbientIntensity = 1.f;
      CMMAT.SpecularIntensity = 1.f;

      CMMAT.EmissiveColor = { 1.f, 0.f, 0.f };
      CMMAT.EmissiveIntensity = 1.f;

      memcpy(MRES.pData, &CMMAT, sizeof(CB_MATERIAL));
      pContext->Unmap(MaterialConstantBuffer->GetCBuffer().Get(), 0);
   }
   pContext->PSSetConstantBuffers(3, 1, MaterialConstantBuffer->GetCBuffer().GetAddressOf());
}
	{
		pContext->PSSetShaderResources(0, 1, m_pResTerrainTexture2D->GetSRV().GetAddressOf());
	}

	pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);

	ID3D11ShaderResourceView* pSRVs[1] = { nullptr };
	pContext->PSSetShaderResources(0, 1, pSRVs);

	return S_OK;
}

E::UPtr<CTerrain> CTerrain::Create()
{
	auto pInstance = E::ToUPtr(new CTerrain{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CTerrain");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CTerrain::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CTerrain{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTerrain");
		return nullptr;
	}

	return pInstance;
}
