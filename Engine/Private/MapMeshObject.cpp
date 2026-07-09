#include "pch.h"
#include "MapMeshObject.h"
#include "ComConstantBuffer.h"
#include "ComStaticModelInstance.h"
#include "ComModelInstance.h"
#include "GameInstance.h"
#include "Resources.h"

NS_USING(Engine)

std::unordered_map<SPtr<CResStaticModel>, std::vector<MAPMESH_INSTANCE_DATA>> CMapMeshObject::s_InstanceBatches{};
SPtr<CResDynamicBuffer> CMapMeshObject::s_pInstanceBuffer{};
size_t CMapMeshObject::s_iInstanceCapacity = 0;
std::optional<CHandle> CMapMeshObject::s_hRenderRepresentative = {};
_bool CMapMeshObject::s_bInstancingEnabled = true;
CMapMeshObject::INSTANCING_STATS CMapMeshObject::s_FrameStats{ true };
CMapMeshObject::INSTANCING_STATS CMapMeshObject::s_LastStats{ true };

CMapMeshObject::CMapMeshObject()
	: CGameObject{}
{
}

CMapMeshObject::CMapMeshObject(const CMapMeshObject& Prototype)
	: CGameObject{ Prototype }
	, m_pResVertexShader{ Prototype.m_pResVertexShader }
	, m_pResPixelShader{ Prototype.m_pResPixelShader }
{
}

CMapMeshObject::~CMapMeshObject()
{
}

HRESULT CMapMeshObject::InitializePrototype(void* pArg)
{
	m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim");
	if (!m_pResVertexShader || FAILED(m_pResVertexShader->Load()))
	{
		return E_FAIL;
	}

	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim");
	if (!m_pResPixelShader || FAILED(m_pResPixelShader->Load()))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CMapMeshObject::Initialize(void* pArg)
{
	if (FAILED(CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	}

	auto* pDesc = static_cast<MAP_MESH_OBJECT_DESC*>(pArg);
	if (pDesc == nullptr)
	{
		return E_FAIL;
	}

	m_modelResourceGroup = pDesc->modelGroupTag;
	m_modelResourceTag = pDesc->modelResTag;

	{
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject", &Desc, &m_pComCBufferPerObject)))
		{
			return E_FAIL;
		}
	}

	{
		CComStaticModelInstance::DESC Desc{};
		Desc.sGroupTag = m_modelResourceGroup;
		Desc.sResTag = m_modelResourceTag;
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_StaticModelInstance", "ComCModelInstance", &Desc, &m_pComModelInstance)))
		{
			return E_FAIL;
		}
	}

	return S_OK;
}

void CMapMeshObject::LateUpdate(_float fTimeDelta)
{
	GetTransform().Update();

	// 컬링된 애면 렌더러 등록x
	if (m_bRenderEnable == false)
		return;

	m_bRenderEnable = false;

	if (m_pComModelInstance == nullptr || m_pComModelInstance->GetModel() == nullptr)
	{
		return;
	}

	++s_FrameStats.iObjects;

	// ------------------------------------------- 인스턴싱 OFF --------------------------------------
	if (!s_bInstancingEnabled)
	{
		CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
		return;
	}
	//------------------------------------------------------------------------------------------------



	// ------------------------------------------- 인스턴싱 ON --------------------------------------
	MAPMESH_INSTANCE_DATA instanceData{};
	XMStoreFloat4x4(&instanceData.world, GetTransform().GetLoadedCombinedWorldMatrix());
	PushInstance(m_pComModelInstance->GetModel(), instanceData);

	// 대표오브젝트 렌더러에 등록
	if (!s_hRenderRepresentative.has_value())
	{
		s_hRenderRepresentative = GetHandle();
		CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
	}
	//------------------------------------------------------------------------------------------------
}

HRESULT CMapMeshObject::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
	// ------------------------------------------- 인스턴싱 OFF --------------------------------------
	if (!s_bInstancingEnabled)
	{
		if (m_pComModelInstance == nullptr || m_pComModelInstance->GetModel() == nullptr)
		{
			return E_FAIL;
		}

		
		{
			CB_PER_OBJECT cbPerObject{};
			cbPerObject.matWorld = *GetTransform().GetCombinedWorldMatrix();
			XMStoreFloat4x4(&cbPerObject.matWVP, GetTransform().GetLoadedCombinedWorldMatrix() * ctx.matViewProj);
			if (FAILED(m_pComCBufferPerObject->MapDiscard(pContext, &cbPerObject, sizeof(cbPerObject))))
			{
				return E_FAIL;
			}
			pContext->VSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
			pContext->PSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
		}

		auto pModel = m_pComModelInstance->GetModel();
		if (nullptr == pModel)	return E_FAIL;
		const uint32_t iNumMeshes = pModel->Get_NumMeshes();
		for (uint32_t i = 0; i < iNumMeshes; ++i) {
			const auto& viBuffer = pModel->GetMeshes()[i];

			ID3D11Buffer* vertexBuffers[] = { viBuffer->GetVertexBuffer().Get() };
			uint32_t strides[] = { viBuffer->GetVertexStride() };
			uint32_t offsets[] = { 0 };

			pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
			pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
			pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

			/*----------- 광윤 추가 -----------*/
			m_pComModelInstance->Bind_Textures(pContext, i);
			m_pComModelInstance->Bind_Materials(pContext, { 1.f, 1.f, 1.f }, 0.f, 1.f);	// EmissiveColor -> EmissiveIntensity -> Alpha 순
			/*---------------------------------*/

			pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);
		}
		++s_FrameStats.iDrawCalls;
		

		return S_OK;
	}
	//------------------------------------------------------------------------------------------------


	// ------------------------------------------- 인스턴싱 ON --------------------------------------
	// 대표오브젝트만 렌더콜(DrawIndexedInstanced)
	if (!s_hRenderRepresentative.has_value() ||
		s_hRenderRepresentative.value() != GetHandle())
	{
		return S_OK;
	}

	return RenderInstancedBatches(pContext, ctx);
}

void CMapMeshObject::UpdateGUI()
{
	CGameObject::UpdateGUI();
}

HRESULT CMapMeshObject::SetModelResource(const std::string& modelGroupTag, const std::string& modelResTag)
{
	if (m_pComModelInstance == nullptr)
	{
		return E_FAIL;
	}

	if (FAILED(m_pComModelInstance->ChangeModel(modelGroupTag, modelResTag)))
	{
		return E_FAIL;
	}

	m_modelResourceGroup = modelGroupTag;
	m_modelResourceTag = modelResTag;

	return S_OK;
}

void CMapMeshObject::SetInstancingEnabled(_bool bEnabled)
{
	if (s_bInstancingEnabled == bEnabled)
	{
		return;
	}

	s_bInstancingEnabled = bEnabled;
	ClearInstancingData();
}

void CMapMeshObject::ClearInstancingData()
{
	s_FrameStats.bEnabled = s_bInstancingEnabled;
	s_FrameStats.iInstances = 0;
	for (const auto& [pModel, instances] : s_InstanceBatches)
	{
		s_FrameStats.iInstances += static_cast<uint32_t>(instances.size());
	}
	s_FrameStats.iBatches = static_cast<uint32_t>(s_InstanceBatches.size());
	s_LastStats = s_FrameStats;
	s_FrameStats = {};
	s_FrameStats.bEnabled = s_bInstancingEnabled;

	s_InstanceBatches.clear();
	s_hRenderRepresentative.reset();
}

void CMapMeshObject::ReleaseInstancingResources()
{
	s_InstanceBatches.clear();
	s_hRenderRepresentative.reset();

	s_pInstanceBuffer.reset();
	s_iInstanceCapacity = 0;
}

HRESULT CMapMeshObject::PushInstance(const SPtr<CResStaticModel>& pModel, const MAPMESH_INSTANCE_DATA& instanceData)
{
	if (pModel == nullptr)
	{
		return E_FAIL;
	}

	s_InstanceBatches[pModel].push_back(instanceData);
	++s_FrameStats.iInstances;
	return S_OK;
}

HRESULT CMapMeshObject::EnsureInstanceBuffer(size_t instanceCount)
{
	if (instanceCount == 0)
	{
		return S_OK;
	}

	if (s_pInstanceBuffer && s_iInstanceCapacity >= instanceCount)
	{
		return S_OK;
	}

	size_t newCapacity = std::max<size_t>(instanceCount, 256);
	while (newCapacity < instanceCount)
	{
		newCapacity *= 2;
	}

	auto pBuffer = CResDynamicBuffer::Create();
	if (pBuffer == nullptr)
	{
		return E_FAIL;
	}

	CResDynamicBuffer::DESC bufferDesc{};
	bufferDesc.desc = {
		.ByteWidth = static_cast<UINT>(sizeof(MAPMESH_INSTANCE_DATA) * newCapacity),
		.Usage = D3D11_USAGE_DYNAMIC,
		.BindFlags = D3D11_BIND_VERTEX_BUFFER,
		.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
		.MiscFlags = 0,
		.StructureByteStride = 0,
	};

	if (FAILED(pBuffer->Load(bufferDesc)))
	{
		return E_FAIL;
	}

	s_pInstanceBuffer = pBuffer;
	s_iInstanceCapacity = newCapacity;
	return S_OK;
}

HRESULT CMapMeshObject::RenderInstancedBatches(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
	if (s_InstanceBatches.empty())
	{
		return S_OK;
	}




	const auto& vertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim_Instanced");
	const auto& pixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim_Instanced");
	const auto& sampler = CGameInstance::Get().GetResourceFirst<CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);

	if (vertexShader == nullptr || pixelShader == nullptr || sampler == nullptr)
	{
		return E_FAIL;
	}

	pContext->IASetInputLayout(vertexShader->GetInputLayout().Get());
	pContext->VSSetShader(vertexShader->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(pixelShader->GetPixelShader().Get(), nullptr, 0);

	for (auto& [pModel, instances] : s_InstanceBatches)
	{
		if (pModel == nullptr || instances.empty())
		{
			continue;
		}

		if (FAILED(EnsureInstanceBuffer(instances.size())))
		{
			return E_FAIL;
		}

		D3D11_MAPPED_SUBRESOURCE mapped{};
		if (FAILED(pContext->Map(s_pInstanceBuffer->GetBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		{
			return E_FAIL;
		}

		std::memcpy(mapped.pData, instances.data(), sizeof(MAPMESH_INSTANCE_DATA) * instances.size());
		pContext->Unmap(s_pInstanceBuffer->GetBuffer().Get(), 0);

		const uint32_t numMeshes = pModel->Get_NumMeshes();
		for (uint32_t i = 0; i < numMeshes; ++i)
		{
			const auto& viBuffer = pModel->GetMeshes()[i];
			if (viBuffer == nullptr)
			{
				continue;
			}

			ID3D11Buffer* vertexBuffers[] = {
				viBuffer->GetVertexBuffer().Get(),
				s_pInstanceBuffer->GetBuffer().Get()
			};
			uint32_t strides[] = {
				viBuffer->GetVertexStride(),
				static_cast<uint32_t>(sizeof(MAPMESH_INSTANCE_DATA)),
			};
			uint32_t offsets[] = { 0, 0 };

			pContext->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);
			pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
			pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

			auto& materials = pModel->GetMaterials();
			const uint32_t materialIndex = viBuffer->Get_MaterialIndex();
			if (materialIndex < materials.size() && materials[materialIndex])
			{
				// 광윤 : 아래는 m_pComModelInstance->Bind_Textures랑 비슷해서 주석쳤습니다. 텍스쳐 없으면 DefaultTexture 들어가게 만들었음
				
				//auto textures = materials[materialIndex]->GetTextures();
				//if (!textures[AI_TEXTURE_TYPE::aiTextureType_DIFFUSE].empty())
				//{
				//	pContext->PSSetShaderResources(AI_TEXTURE_TYPE::aiTextureType_DIFFUSE, 1, textures[AI_TEXTURE_TYPE::aiTextureType_DIFFUSE].front()->GetSRV().GetAddressOf());
				//}
				//else if (!textures[0].empty())
				//{
				//	pContext->PSSetShaderResources(AI_TEXTURE_TYPE::aiTextureType_DIFFUSE, 1, textures[0].front()->GetSRV().GetAddressOf());
				//}
				// 
				/*----------- 광윤 추가 -----------*/
				m_pComModelInstance->Bind_Textures(pContext, i);
				m_pComModelInstance->Bind_Materials(pContext, { 1.f, 1.f, 1.f }, 0.f, 1.f);	// EmissiveColor -> EmissiveIntensity -> Alpha 순
				/*---------------------------------*/
			}
			

			pContext->DrawIndexedInstanced(
				static_cast<UINT>(viBuffer->GetNumIndices()),
				static_cast<UINT>(instances.size()),
				0,
				0,
				0
			);    

			++s_FrameStats.iDrawCalls;
		}
	}

	return S_OK;
}

UPtr<CMapMeshObject> CMapMeshObject::Create()
{
	auto pInstance = ToUPtr(new CMapMeshObject{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CMapMeshObject");
		return nullptr;
	}
	return pInstance;
}

UPtr<CPrototype> CMapMeshObject::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CMapMeshObject{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CMapMeshObject");
		return nullptr;
	}

	return pInstance;
}
