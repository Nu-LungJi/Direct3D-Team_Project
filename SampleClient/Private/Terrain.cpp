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
	auto Desc = static_cast<DESC*>(pArg);
	//본인 레벨 네임 넣기
	m_pResTerrainVIBuffer = CGameInstance::Get().GetResourceFirst<CResTerrainVIBuffer>(Desc->tagLevelName, "VIBUFFER_Terrain");
	if (!m_pResTerrainVIBuffer)
	{
		return E_FAIL;
	}

	m_pResTerrainTexture2D = CGameInstance::Get().GetResourceFirst<CResTexture2D>(Desc->tagLevelName, "TEX2D_Terrain_Tile0");
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

	return S_OK;
}

HRESULT CTerrain::Initialize(void* pArg)
{
	m_RenderPassFlags = ETOUI(RENDERPASS::DEFAULT) | ETOUI(RENDERPASS::SHADOW);
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
		if (FAILED(m_pComCBufferPerObject->MapDiscard(pContext, &cbPerObject, sizeof(cbPerObject))))	return E_FAIL;

		pContext->VSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	}
	const auto& vs = m_pResVertexShader;
	const auto& ps = m_pResPixelShader;
	
	const auto& viBuffer = m_pResTerrainVIBuffer;
	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	
	if (ctx.pass == RENDERPASS::DEFAULT) {			// 오류 메세지 ID3D11DeviceContext::DrawIndexed 제거용
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

	SPtr<CResTexture2D> DiffuseTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_DIFFUSE");
	if (auto Resource = m_pResTerrainTexture2D) {
		DiffuseTexture = Resource;
	}
	pContext->PSSetShaderResources(0, 1, DiffuseTexture->GetSRV().GetAddressOf());

	auto MaterialConstantBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_MATERIAL");
	D3D11_MAPPED_SUBRESOURCE MRES;
	if (SUCCEEDED(pContext->Map(MaterialConstantBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
	{
		CB_MATERIAL   CMMAT;
		CMMAT.EmissiveColor		= _float3(1.f, 1.f, 1.f);
		CMMAT.EmissiveIntensity = 0.f;
		CMMAT.ObjectAlpha		= 1.f;

		memcpy(MRES.pData, &CMMAT, sizeof(CB_MATERIAL));
		pContext->Unmap(MaterialConstantBuffer->GetCBuffer().Get(), 0);
	}
	pContext->PSSetConstantBuffers(3, 1, MaterialConstantBuffer->GetCBuffer().GetAddressOf());
	pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);

	ID3D11ShaderResourceView* pSRVs[1] = { nullptr };
	pContext->PSSetShaderResources(0, 1, pSRVs);
	pContext->PSSetShaderResources(1, 1, pSRVs);
	pContext->PSSetShaderResources(2, 1, pSRVs);
	pContext->PSSetShaderResources(3, 1, pSRVs);

	CGameInstance::Get().Reset_DefaultShader(RENDERGROUP::NONBLEND);
	// 오브젝트 렌더 할 떄, VSSetShader, PSSetShader 를 해야한다면,
	// 다시 원래 쉐이더로 돌려놓아야 이후에 렌더하는 오브젝트들이 정상적으로 렌더 됨.
	// 나중에 오브젝트들 정리해서 배칭으로 전환할 때 삭제 예정.

	return S_OK;
}

E::UPtr<CTerrain> CTerrain::Create(void* pArg)
{
	auto pInstance = E::ToUPtr(new CTerrain{});
	if (FAILED(pInstance->InitializePrototype(pArg)))
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
