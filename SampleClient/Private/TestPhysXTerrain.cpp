#include "pch.h"
#include "TestPhysXTerrain.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "Resources.h"
#include "GameInstance.h"

#include "ComPxRigidBody.h"
#include "ComPxTriMeshCollider.h"

NS_USING(Client)

CTestPhysXTerrain::CTestPhysXTerrain()
	: CGameObject{}
{
}

CTestPhysXTerrain::~CTestPhysXTerrain()
{
}

HRESULT CTestPhysXTerrain::InitializePrototype(void* pArg)
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
	//m_pResVertexShader = CResVertexShader::Create("./ShaderFiles/Shader_VtxNorTex.hlsl");
	if (FAILED(m_pResVertexShader->Load()))
	{
		return E_FAIL;
	}
	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>("SAMPLE_CLIENT_SHADER", "PS_VTX_NOR_TEX");
	//m_pResPixelShader = CResPixelShader::Create("./ShaderFiles/Shader_VtxNorTex.hlsl");
	if (FAILED(m_pResPixelShader->Load()))
	{
		return E_FAIL;
	}

	m_pResSamplerState = CGameInstance::Get().GetResourceFirst<CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
	if (!m_pResSamplerState)
	{
		return E_FAIL;
	}

	auto v = m_pResTerrainVIBuffer->GetVertices();

	for (const auto& vTmp : v)
	{
		m_vecPoses.push_back(vTmp.pos);
	}
	auto indices = m_pResTerrainVIBuffer->GetIndices();
	auto triangleCount = static_cast<uint32_t>(indices.size() / 3);
	for (uint32_t i = 0; i < triangleCount; ++i)
	{
		int32_t i0 = indices[i * 3 + 0];
		int32_t i1 = indices[i * 3 + 1];
		int32_t i2 = indices[i * 3 + 2];
		m_vecTriangles.push_back({ i0, i1, i2 });

		// 0 1
		{
			m_vecPreBuiltedDbgLineVertices.push_back(VTX_COL{ .pos = v[i0].pos, .color = {0.f, 0.f, 0.f, 1.f} });
			m_vecPreBuiltedDbgLineVertices.push_back(VTX_COL{ .pos = v[i1].pos, .color = {0.f, 0.f, 0.f, 1.f} });
		}

		// 1 2
		{
			m_vecPreBuiltedDbgLineVertices.push_back(VTX_COL{ .pos = v[i1].pos, .color = {0.f, 0.f, 0.f, 1.f} });
			m_vecPreBuiltedDbgLineVertices.push_back(VTX_COL{ .pos = v[i2].pos, .color = {0.f, 0.f, 0.f, 1.f} });
		}

		// 2 0
		{
			m_vecPreBuiltedDbgLineVertices.push_back(VTX_COL{ .pos = v[i2].pos, .color = {0.f, 0.f, 0.f, 1.f} });
			m_vecPreBuiltedDbgLineVertices.push_back(VTX_COL{ .pos = v[i0].pos, .color = {0.f, 0.f, 0.f, 1.f} });
		}
	}


	m_pResTriMesh = CResPhysXTriMeshGeometry::Create();
	if (FAILED(m_pResTriMesh->Load(CResPhysXTriMeshGeometry::DESC{.pVecVertices = &m_vecPoses, .pVecTriangles = &m_vecTriangles})))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CTestPhysXTerrain::Initialize(void* pArg)
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
		CComPxRigidBody::DESC Desc{};
		Desc.eType = CComPxRigidBody::TYPE::STATIC;
		if (FAILED(AddComponentFromProto("PHYSX", "Prototype_Component_ComPxRigidBody", "ComPxRigidBody", &Desc, &m_pComPxRigidBody)))
		{
			return E_FAIL;
		};
	}

	{
		CComPxTriMeshCollider::DESC Desc{};
		Desc.pComPxRigidBody = m_pComPxRigidBody;
		Desc.pResTriMesh = m_pResTriMesh;
		Desc.pResMaterial = CResPhysXMaterial::Create({});
		if (FAILED(AddComponentFromProto("PHYSX", "Prototype_Component_ComPxTriMeshCollider", "ComPxTriMeshCollider", &Desc, &m_pComPxTriMeshCollider)))
		{
			return E_FAIL;
		};
	}


	return S_OK;
}

void CTestPhysXTerrain::PriorityUpdate(E::_float fTimeDelta)
{
}

void CTestPhysXTerrain::Update(E::_float fTimeDelta)
{
	//CGameInstance::Get().GetDbgLineRender()->AddBuiltedVertices(m_vecPreBuiltedDbgLineVertices);

}

void CTestPhysXTerrain::LateUpdate(E::_float fTimeDelta)
{
	GetTransform().Update();
	CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
}

HRESULT CTestPhysXTerrain::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
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
		pContext->PSSetShaderResources(0, 1, m_pResTerrainTexture2D->GetSRV().GetAddressOf());
	}

	pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);


	return S_OK;
}

E::UPtr<CTestPhysXTerrain> CTestPhysXTerrain::Create()
{
	auto pInstance = E::ToUPtr(new CTestPhysXTerrain{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CTestPhysXTerrain");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CTestPhysXTerrain::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CTestPhysXTerrain{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTestPhysXTerrain");
		return nullptr;
	}

	return pInstance;
}
