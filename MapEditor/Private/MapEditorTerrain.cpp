#include "pch.h"
#include "MapEditorTerrain.h"
#include "ResMapEditorTerrainVIBuffer.h"
#include "ComConstantBuffer.h"
#include "Resources.h"
#include "GameInstance.h"

NS_USING(Client)

CMapEditorTerrain::CMapEditorTerrain()
	: CGameObject{}
{
}

CMapEditorTerrain::~CMapEditorTerrain()
{
}

HRESULT CMapEditorTerrain::InitializePrototype(void* pArg)
{
	m_pResMapEditorTerrainVIBuffer = CGameInstance::Get().GetResourceFirst<CResMapEditorTerrainVIBuffer>("MAPEDITOR", "VIBUFFER_Terrain");
	//m_pResMapEditorTerrainVIBuffer = CResMapEditorTerrainVIBuffer::Create("./Resources/SampleClient/Textures/Terrain/Height.bmp");
	if (!m_pResMapEditorTerrainVIBuffer)
	{
		return E_FAIL;
	}

	//"SAMPLE_CLIENT_TEX", "TEX2D_Terrain_Tile0"
	m_pResTerrainTexture2D = CGameInstance::Get().GetResourceFirst<CResTexture2D>("MAPEDITOR", "TEX2D_Terrain_Tile0");
	//m_pResTerrainTexture2D = CResTexture2D::Create("./Resources/SampleClient/Textures/Terrain/Tile0.dds");
	if (!m_pResTerrainTexture2D)
	{
		return E_FAIL;
	}

	m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>("MAP_EDITOR_SHADER", "VS_VTX_NOR_TEX");
	if (!m_pResVertexShader || FAILED(m_pResVertexShader->Load()))
		return E_FAIL;

	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>("MAP_EDITOR_SHADER", "PS_VTX_NOR_TEX");
	if (!m_pResPixelShader || FAILED(m_pResPixelShader->Load()))
		return E_FAIL;

	m_pResSamplerState = CGameInstance::Get().GetResourceFirst<CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
	if (!m_pResSamplerState)
		return E_FAIL;

	return S_OK;
}

HRESULT CMapEditorTerrain::Initialize(void* pArg)
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

void CMapEditorTerrain::PriorityUpdate(E::_float fTimeDelta)
{
}

void CMapEditorTerrain::Update(E::_float fTimeDelta)
{
}

void CMapEditorTerrain::LateUpdate(E::_float fTimeDelta)
{
	GetTransform().Update();
	CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);

	/*auto dbg = CGameInstance::Get().GetDbgLineRender();
	if (!dbg || !m_pResMapEditorTerrainVIBuffer)
		return;

	dbg->SetColor({ 0.f, 1.f, 0.f, 1.f });

	const auto& vertices = m_pResMapEditorTerrainVIBuffer->GetVertices();
	const auto& indices = m_pResMapEditorTerrainVIBuffer->GetIndices();

	for (size_t i = 0; i + 2 < indices.size(); i += 3)
	{
		const auto& p0 = vertices[indices[i + 0]].pos;
		const auto& p1 = vertices[indices[i + 1]].pos;
		const auto& p2 = vertices[indices[i + 2]].pos;

		dbg->AddTriangle(p0, p1, p2);
	}*/

	if (auto* navMeshManager = CGameInstance::Get().GetNavMeshManager())
	{
		const auto& vertices = GetVertices();
		const auto& indices = GetIndices();

		std::vector<E::_float3> navVertices{};
		navVertices.reserve(vertices.size());

		for (const auto& vertex : vertices)
		{
			navVertices.push_back(vertex.pos);
		}

		navMeshManager->DrawBlockedTriangles(navVertices, indices);
	}
	
}

const std::vector<VTX_NORMAL_TEX>& CMapEditorTerrain::GetVertices() const
{
	static const std::vector<VTX_NORMAL_TEX> emptyVertices{};
	return m_pResMapEditorTerrainVIBuffer ? m_pResMapEditorTerrainVIBuffer->GetVertices() : emptyVertices;
}

const std::vector<uint32_t>& CMapEditorTerrain::GetIndices() const
{
	static const std::vector<uint32_t> emptyIndices{};
	return m_pResMapEditorTerrainVIBuffer ? m_pResMapEditorTerrainVIBuffer->GetIndices() : emptyIndices;
}

HRESULT CMapEditorTerrain::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
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

	const auto& viBuffer = m_pResMapEditorTerrainVIBuffer;
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

			CMMAT.EmissiveColor = { 1.f, 0.f, 0.f };
			CMMAT.EmissiveIntensity = 1.f;
			CMMAT.ObjectAlpha = 1.f;

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

bool CMapEditorTerrain::IsOcclusionCullable() const
{
	return m_pResMapEditorTerrainVIBuffer != nullptr &&
		!m_pResMapEditorTerrainVIBuffer->GetVertices().empty();
}

bool CMapEditorTerrain::GetOcclusionBounds(BoundingBox& outBounds) const
{
	if (m_pResMapEditorTerrainVIBuffer == nullptr)
		return false;

	const auto& vertices = m_pResMapEditorTerrainVIBuffer->GetVertices();
	if (vertices.empty())
		return false;

	XMFLOAT3 minPos{
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::max()
	};

	XMFLOAT3 maxPos{
		-std::numeric_limits<float>::max(),
		-std::numeric_limits<float>::max(),
		-std::numeric_limits<float>::max()
	};

	for (const auto& vertex : vertices)
	{
		const auto& pos = vertex.pos;

		minPos.x = std::min(minPos.x, pos.x);
		minPos.y = std::min(minPos.y, pos.y);
		minPos.z = std::min(minPos.z, pos.z);

		maxPos.x = std::max(maxPos.x, pos.x);
		maxPos.y = std::max(maxPos.y, pos.y);
		maxPos.z = std::max(maxPos.z, pos.z);
	}

	const XMFLOAT3 center{
		(minPos.x + maxPos.x) * 0.5f,
		(minPos.y + maxPos.y) * 0.5f,
		(minPos.z + maxPos.z) * 0.5f
	};

	const XMFLOAT3 extents{
		(maxPos.x - minPos.x) * 0.5f,
		(maxPos.y - minPos.y) * 0.5f,
		(maxPos.z - minPos.z) * 0.5f
	};

	BoundingBox localBox(center, extents);
	localBox.Transform(outBounds, GetTransform().GetLoadedCombinedWorldMatrix());

	return true;
}

E::UPtr<CMapEditorTerrain> CMapEditorTerrain::Create()
{
	auto pInstance = E::ToUPtr(new CMapEditorTerrain{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CMapEditorTerrain");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CMapEditorTerrain::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CMapEditorTerrain{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CMapEditorTerrain");
		return nullptr;
	}

	return pInstance;
}
