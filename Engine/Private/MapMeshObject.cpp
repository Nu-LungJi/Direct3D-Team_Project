#include "pch.h"
#include "MapMeshObject.h"
#include "ComConstantBuffer.h"
#include "ComStaticModelInstance.h"
#include "ComModelInstance.h"
#include "GameInstance.h"
#include "MapMeshGpuCuller.h"
#include "Resources.h"

NS_USING(Engine)

std::unordered_map<SPtr<CResStaticModel>, MAPMESH_INSTANCE_BATCH> CMapMeshObject::s_InstanceBatches{};
std::optional<CHandle> CMapMeshObject::s_hRenderRepresentative = {};
UPtr<CMapMeshGpuCuller> CMapMeshObject::s_pGpuCuller{};
_bool CMapMeshObject::s_bInstancingEnabled = true;
_bool CMapMeshObject::s_bDebugBoundsEnabled = false;
CMapMeshObject::INSTANCING_STATS CMapMeshObject::s_FrameStats{ true };
CMapMeshObject::INSTANCING_STATS CMapMeshObject::s_LastStats{ true };

namespace
{
	constexpr size_t MAPMESH_TEXTURE_COUNT = 4;
	using MAPMESH_TEXTURE_SET = std::array<SPtr<CResTexture2D>, MAPMESH_TEXTURE_COUNT>;
	std::unordered_map<SPtr<CResStaticModel>, std::vector<MAPMESH_TEXTURE_SET>> s_MapMeshTextureCache;

	SPtr<CResTexture2D> GetMapMeshTexture(const SPtr<CResStaticModel>& pModel, uint32_t meshIndex, AI_TEXTURE_TYPE materialType)
	{
		if (pModel == nullptr)
		{
			return nullptr;
		}

		auto& meshes = pModel->GetMeshes();
		if (meshIndex >= meshes.size() || meshes[meshIndex] == nullptr)
		{
			return nullptr;
		}

		auto& materials = pModel->GetMaterials();
		const uint32_t materialIndex = meshes[meshIndex]->Get_MaterialIndex();
		if (materialIndex >= materials.size() || materials[materialIndex] == nullptr)
		{
			return nullptr;
		}

		auto textures = materials[materialIndex]->GetTextures();
		if (textures[materialType].empty())
		{
			return nullptr;
		}

		return textures[materialType].front();
	}

	const std::vector<MAPMESH_TEXTURE_SET>* GetOrCreateMapMeshTextureCache(const SPtr<CResStaticModel>& pModel)
	{
		if (pModel == nullptr)
		{
			return nullptr;
		}

		if (const auto iter = s_MapMeshTextureCache.find(pModel); iter != s_MapMeshTextureCache.end())
		{
			return &iter->second;
		}

		MAPMESH_TEXTURE_SET defaultTextures{
			CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_DIFFUSE"),
			CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_NORMAL"),
			CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_SMRO"),
			CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_EMISSIVE")
		};
		if (std::ranges::any_of(defaultTextures, [](const auto& texture) { return texture == nullptr; }))
		{
			return nullptr;
		}

		std::vector<MAPMESH_TEXTURE_SET> textureSets(pModel->Get_NumMeshes(), defaultTextures);
		for (uint32_t meshIndex = 0; meshIndex < textureSets.size(); ++meshIndex)
		{
			auto& textures = textureSets[meshIndex];
			if (auto texture = GetMapMeshTexture(pModel, meshIndex, AI_TEXTURE_TYPE::aiTextureType_DIFFUSE))
				textures[0] = std::move(texture);
			if (auto texture = GetMapMeshTexture(pModel, meshIndex, AI_TEXTURE_TYPE::aiTextureType_NORMALS))
				textures[1] = std::move(texture);
			if (auto texture = GetMapMeshTexture(pModel, meshIndex, AI_TEXTURE_TYPE::aiTextureType_METALNESS))
				textures[2] = std::move(texture);
			if (auto texture = GetMapMeshTexture(pModel, meshIndex, AI_TEXTURE_TYPE::aiTextureType_EMISSIVE))
				textures[3] = std::move(texture);
		}

		auto [iter, inserted] = s_MapMeshTextureCache.emplace(pModel, std::move(textureSets));
		return &iter->second;
	}

	HRESULT BindMapMeshTextures(ID3D11DeviceContext* pContext, const std::vector<MAPMESH_TEXTURE_SET>& textureCache, uint32_t meshIndex)
	{
		if (pContext == nullptr || meshIndex >= textureCache.size())
		{
			return E_FAIL;
		}

		ID3D11ShaderResourceView* srvs[MAPMESH_TEXTURE_COUNT]{};
		for (size_t i = 0; i < MAPMESH_TEXTURE_COUNT; ++i)
		{
			if (textureCache[meshIndex][i] == nullptr)
				return E_FAIL;
			srvs[i] = textureCache[meshIndex][i]->GetSRV().Get();
		}
		pContext->PSSetShaderResources(0, MAPMESH_TEXTURE_COUNT, srvs);

		return S_OK;
	}

	HRESULT BindMapMeshMaterial(ID3D11DeviceContext* pContext, _float3 emissiveColor, _float emissiveIntensity, _float objectAlpha)
	{
		if (pContext == nullptr)
		{
			return E_FAIL;
		}

		auto materialConstantBuffer = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_MATERIAL");
		if (materialConstantBuffer == nullptr)
		{
			return E_FAIL;
		}

		D3D11_MAPPED_SUBRESOURCE mapped{};
		if (FAILED(pContext->Map(materialConstantBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		{
			return E_FAIL;
		}

		CB_MATERIAL material{};
		material.EmissiveColor = emissiveColor;
		material.EmissiveIntensity = emissiveIntensity;
		material.ObjectAlpha = objectAlpha;

		memcpy(mapped.pData, &material, sizeof(CB_MATERIAL));
		pContext->Unmap(materialConstantBuffer->GetCBuffer().Get(), 0);
		pContext->PSSetConstantBuffers(3, 1, materialConstantBuffer->GetCBuffer().GetAddressOf());

		return S_OK;
	}
}

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

	if (s_bDebugBoundsEnabled)
	{
		BoundingBox debugBounds{};
		if (GetOcclusionBounds(debugBounds))
		{
			auto* pDebugLine = CGameInstance::Get().GetDbgLineRender();
			if (pDebugLine != nullptr)
			{
				pDebugLine->SetColor({ 1.f, 1.f, 0.f, 1.f });
				pDebugLine->AddBox(debugBounds.Extents, XMMatrixTranslation(
					debugBounds.Center.x, debugBounds.Center.y, debugBounds.Center.z));
				pDebugLine->SetColor();
			}
		}
	}

	++s_FrameStats.iObjects;

	// ------------------------------------------- 인스턴싱 OFF --------------------------------------
	if (!s_bInstancingEnabled)
	{
		//if (CGameInstance::Get().IsOcclusionCulled(this))
		//{
		//	return;
		//}

		CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
		return;
	}
	//------------------------------------------------------------------------------------------------



	// ------------------------------------------- 인스턴싱 ON --------------------------------------
	MAPMESH_INSTANCE_DATA instanceData{};
	MAPMESH_OCCLUSION_DATA occlusionData{};
	XMStoreFloat4x4(&instanceData.world, GetTransform().GetLoadedCombinedWorldMatrix());

	BoundingBox boundingBox;
	if (!GetOcclusionBounds(boundingBox))
		return;
	occlusionData.worldCenter = boundingBox.Center;
	occlusionData.worldExtents = boundingBox.Extents;
	PushInstance(m_pComModelInstance->GetModel(), instanceData, occlusionData);

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
			pContext->IASetInputLayout(m_pResVertexShader->GetInputLayout().Get());
			pContext->VSSetShader(m_pResVertexShader->GetVertexShader().Get(), nullptr, 0);
			pContext->PSSetShader(m_pResPixelShader->GetPixelShader().Get(), nullptr, 0);
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
			m_pComModelInstance->Bind_Materials(pContext, { 1.f, 1.f, 1.f }, 0.f, { 1.f, 1.f, 1.f }, 0.f, 1.f);	// EmissiveColor -> EmissiveIntensity -> Alpha 순
			/*---------------------------------*/

			pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);
			++s_FrameStats.iDrawCalls;
		}
		

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

bool CMapMeshObject::IsOcclusionCullable() const
{
	return !s_bInstancingEnabled &&
		m_pComModelInstance != nullptr &&
		m_pComModelInstance->GetModel() != nullptr;
}

bool CMapMeshObject::GetOcclusionBounds(BoundingBox& outBounds) const
{
	if (m_pComModelInstance == nullptr || m_pComModelInstance->GetModel() == nullptr)
		return false;

	const auto model = m_pComModelInstance->GetModel();
	if (!model->HasLocalBounds())
		return false;

	model->GetLocalBounds().Transform(outBounds, GetTransform().GetLoadedCombinedWorldMatrix());

	return true;
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
	for (const auto& [pModel, instancesBatch] : s_InstanceBatches)
	{
		s_FrameStats.iInstances += static_cast<uint32_t>(instancesBatch.instances.size());
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
	s_MapMeshTextureCache.clear();
	s_hRenderRepresentative.reset();

	s_pGpuCuller.reset();
}

HRESULT CMapMeshObject::PushInstance(const SPtr<CResStaticModel>& pModel, const MAPMESH_INSTANCE_DATA& instanceData, MAPMESH_OCCLUSION_DATA& occlusionData)
{
	if (pModel == nullptr)
	{
		return E_FAIL;
	}

	auto& batch = s_InstanceBatches[pModel];

	occlusionData.instanceIndex = static_cast<uint32_t>(batch.instances.size());

	batch.instances.push_back(instanceData);
	batch.occlusionData.push_back(occlusionData);

	++s_FrameStats.iInstances;

	return S_OK;
}

HRESULT CMapMeshObject::RenderInstancedBatches(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
	ZoneScopedN("RenderInstancedBatches");
	if (pContext == nullptr || s_InstanceBatches.empty())
		return S_OK;

	const auto& vertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim_Instanced");
	const auto& pixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim_Instanced");
	const auto& sampler = CGameInstance::Get().GetResourceFirst<CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
	if (!vertexShader || !pixelShader || !sampler)
		return E_FAIL;

	pContext->IASetInputLayout(vertexShader->GetInputLayout().Get());
	pContext->VSSetShader(vertexShader->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(pixelShader->GetPixelShader().Get(), nullptr, 0);
	pContext->PSSetSamplers(0, 1, sampler->GetSamplerState().GetAddressOf());

	if (s_pGpuCuller == nullptr)
	{
		s_pGpuCuller = CMapMeshGpuCuller::Create();
		if (s_pGpuCuller == nullptr)
			return E_FAIL;
	}

	struct DRAW_ITEM
	{
		SPtr<CResStaticModel> model{};
		const std::vector<MAPMESH_TEXTURE_SET>* textureCache = nullptr;
		uint32_t meshIndex = 0;
		uint32_t instanceOffset = 0;
	};

	size_t totalInstances = 0;
	size_t totalDraws = 0;
	for (const auto& [model, batch] : s_InstanceBatches)
	{
		if (model && !batch.instances.empty())
		{
			totalInstances += batch.instances.size();
			totalDraws += model->Get_NumMeshes();
		}
	}

	std::vector<MAPMESH_INSTANCE_DATA> instances;
	std::vector<MAPMESH_OCCLUSION_DATA> occlusionData;
	std::vector<MAPMESH_CULL_META> cullMeta;
	std::vector<uint32_t> drawBatchIndices;
	std::vector<D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS> indirectArgs;
	std::vector<DRAW_ITEM> drawItems;
	instances.reserve(totalInstances);
	occlusionData.reserve(totalInstances);
	cullMeta.reserve(totalInstances);
	drawBatchIndices.reserve(totalDraws);
	indirectArgs.reserve(totalDraws);
	drawItems.reserve(totalDraws);

	uint32_t batchIndex = 0;
	for (const auto& [model, batch] : s_InstanceBatches)
	{
		if (!model || batch.instances.empty())
			continue;
		if (batch.occlusionData.size() != batch.instances.size())
			return E_FAIL;

		const auto* textureCache = GetOrCreateMapMeshTextureCache(model);
		if (textureCache == nullptr)
			return E_FAIL;

		const uint32_t instanceOffset = static_cast<uint32_t>(instances.size());
		instances.insert(instances.end(), batch.instances.begin(), batch.instances.end());
		occlusionData.insert(occlusionData.end(), batch.occlusionData.begin(), batch.occlusionData.end());
		cullMeta.insert(cullMeta.end(), batch.instances.size(), MAPMESH_CULL_META{ instanceOffset, batchIndex });

		for (uint32_t meshIndex = 0; meshIndex < model->Get_NumMeshes(); ++meshIndex)
		{
			const auto& mesh = model->GetMeshes()[meshIndex];
			if (mesh == nullptr)
				continue;

			drawBatchIndices.push_back(batchIndex);
			D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS args{};
			args.IndexCountPerInstance = static_cast<uint32_t>(mesh->GetNumIndices());
			indirectArgs.push_back(args);
			drawItems.push_back({ model, textureCache, meshIndex, instanceOffset });
		}
		++batchIndex;
	}

	if (instances.empty() || drawItems.empty())
		return S_OK;

	if (FAILED(s_pGpuCuller->BuildVisibleInstancesAndIndirectArgs(
		pContext, instances, occlusionData, cullMeta, batchIndex,
		drawBatchIndices, indirectArgs,
		CGameInstance::Get().GetPrevHizBuffer(), ctx.matViewProj,
		CGameInstance::Get().GetClientScreenSize())))
	{
		return E_FAIL;
	}

	ID3D11Buffer* visibleInstanceBuffer = s_pGpuCuller->GetVisibleInstanceBuffer();
	ID3D11Buffer* argsBuffer = s_pGpuCuller->GetIndirectArgsBuffer();
	if (!visibleInstanceBuffer || !argsBuffer)
		return E_FAIL;
	if (FAILED(BindMapMeshMaterial(pContext, { 1.f, 1.f, 1.f }, 0.f, 1.f)))
		return E_FAIL;

	for (uint32_t drawIndex = 0; drawIndex < drawItems.size(); ++drawIndex)
	{
		const auto& item = drawItems[drawIndex];
		const auto& mesh = item.model->GetMeshes()[item.meshIndex];
		ID3D11Buffer* vertexBuffers[] = { mesh->GetVertexBuffer().Get(), visibleInstanceBuffer };
		uint32_t strides[] = { mesh->GetVertexStride(), static_cast<uint32_t>(sizeof(MAPMESH_INSTANCE_DATA)) };
		uint32_t offsets[] = { 0, item.instanceOffset * static_cast<uint32_t>(sizeof(MAPMESH_INSTANCE_DATA)) };
		pContext->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);
		pContext->IASetIndexBuffer(mesh->GetIndexBuffer().Get(), mesh->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(mesh->GetPrimitiveType());
		if (FAILED(BindMapMeshTextures(pContext, *item.textureCache, item.meshIndex)))
			return E_FAIL;

		pContext->DrawIndexedInstancedIndirect(
			argsBuffer,
			drawIndex * static_cast<uint32_t>(sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS)));
		++s_FrameStats.iDrawCalls;
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
