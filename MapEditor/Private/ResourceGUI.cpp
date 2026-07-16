#include "pch.h"
#include "ResourceGUI.h"
#include "EditorCommandManager.h"
#include "CreateMapMeshCommand.h"
#include "MapMeshCommandCommon.h"
#include "GameInstance.h"
#include "MapMeshObject.h"
#include "Resources.h"
#include "ResCBuffer.h"
#include "ResPixelShader.h"
#include "ResVertexShader.h"
#include "ResOffscreenTexture.h"
#include "ResModelMaterial.h"
#include <commdlg.h>
#include <nlohmann/json.hpp>

NS_USING(Client)

namespace
{
	struct ModelResourceDragPayload
	{
		char groupName[128]{};
		char resourceName[128]{};
	};

	constexpr const char* PAYLOAD_MODEL_RESOURCE = "MAPEDITOR_MODEL_RESOURCE";

	const char* const CATEGORY_NAMES[] =
	{
		"All", "Model", "StaticModel", "Texture", "Shader", "Buffer", "State", "Audio", "Data", "Other"
	};

	std::string SafeDbgStr(const E::StringID& id)
	{
		const char* text = id.GetDbgStr();
		return text != nullptr ? text : "<unnamed>";
	}

	std::string MakeStaticModelResourceTag(const std::filesystem::path& rootPath,
		const std::filesystem::path& binPath)
	{
		std::filesystem::path relativePath = binPath.lexically_relative(rootPath);
		const bool isOutsideRoot = !relativePath.empty() && *relativePath.begin() == "..";
		if (relativePath.empty() || isOutsideRoot)
		{
			// The manifest may be selected from the external team-resource tree
			// while MapEditor loaded the copied Resources tree. Both use the same
			// logical path below Models/Static, so rebuild that suffix here.
			relativePath.clear();
			bool foundStatic = false;
			for (const auto& part : binPath)
			{
				if (!foundStatic)
				{
					if (_stricmp(part.string().c_str(), "Static") == 0)
						foundStatic = true;
					continue;
				}
				relativePath /= part;
			}
		}
		if (relativePath.empty())
			relativePath = binPath.filename();
		relativePath.replace_extension();

		std::string resourceTag = relativePath.string();
		for (char& ch : resourceTag)
		{
			if (!std::isalnum(static_cast<unsigned char>(ch)))
				ch = '_';
		}
		return resourceTag;
	}

	const char* GetResourceStateName(E::CResource::STATE state)
	{
		switch (state)
		{
		case E::CResource::STATE::UNLOAD:
			return "UNLOAD";
		case E::CResource::STATE::LOADING:
			return "LOADING";
		case E::CResource::STATE::LOADFAIL:
			return "LOADFAIL";
		case E::CResource::STATE::LOADED:
			return "LOADED";
		default:
			return "UNKNOWN";
		}
	}

	Client::ResourceViewItem MakeViewItem(const E::StringID& groupId, const E::StringID& resourceId, const E::SPtr<E::CResource>& resource, size_t index, size_t count)
	{
		Client::ResourceViewItem item{};
		item.groupName = SafeDbgStr(groupId);
		item.resourceTag = SafeDbgStr(resourceId);
		item.resourceName = item.resourceTag;
		if (count > 1)
		{
			item.resourceName += " #" + std::to_string(index);
		}

		if (resource)
		{
			item.path = resource->GetPath();
			item.state = GetResourceStateName(resource->GetState());
		}

		if (!resource)
		{
			item.category = "Other";
			item.icon = "?";
			item.color = ImVec4(0.34f, 0.34f, 0.36f, 1.f);
		}
		else if (resource->IsA(E::CResStaticModel::StaticType) || resource->IsA(E::CResStaticModelMesh::StaticType))
		{
			item.category = "StaticModel";
			item.icon = "SM";
			item.color = ImVec4(0.18f, 0.56f, 0.68f, 1.f);
			item.bCanCreateMapMeshObject = resource->IsA(E::CResStaticModel::StaticType);
			if (item.bCanCreateMapMeshObject)
			{
				item.staticModel = std::static_pointer_cast<E::CResStaticModel>(resource);
			}
		}
		else if (resource->IsA(E::CResModel::StaticType) || resource->IsA(E::CResModelMesh::StaticType) || resource->IsA(E::CResModelMaterial::StaticType) || resource->IsA(E::CResModelAnim::StaticType) || resource->IsA(E::CResModelChanel::StaticType) || resource->IsA(E::CResModelBone::StaticType))
		{
			item.category = "Model";
			item.icon = "M";
			item.color = ImVec4(0.25f, 0.45f, 0.80f, 1.f);
		}
		else if (resource->IsA(E::CResTexture2D::StaticType) || resource->IsA(E::CResTexture2DArray::StaticType) || resource->IsA(E::CResTextureCubeMap::StaticType) || resource->IsA(E::CResDynamicTexture2D::StaticType) || resource->IsA(E::CResOffscreenTexture::StaticType))
		{
			item.category = "Texture";
			item.icon = "T";
			item.color = ImVec4(0.26f, 0.62f, 0.46f, 1.f);
		}
		else if (resource->IsA(E::CResVertexShader::StaticType) || resource->IsA(E::CResPixelShader::StaticType) || resource->IsA(E::CResGeometryShader::StaticType) || resource->IsA(E::CResComputeShader::StaticType) || resource->IsA(E::CResTessHullShader::StaticType) || resource->IsA(E::CResTessDomainShader::StaticType) || resource->IsA(E::CResGeoShaderStreamOut::StaticType))
		{
			item.category = "Shader";
			item.icon = "S";
			item.color = ImVec4(0.61f, 0.38f, 0.78f, 1.f);
		}
		else if (resource->IsA(E::CResCBuffer::StaticType) || resource->IsA(E::CResDynamicBuffer::StaticType) || resource->IsA(E::CResDynamicVIBuffer::StaticType) || resource->IsA(E::CResStructuredBuffer::StaticType) || resource->IsA(E::CResVIBuffer::StaticType))
		{
			item.category = "Buffer";
			item.icon = "B";
			item.color = ImVec4(0.71f, 0.49f, 0.24f, 1.f);
		}
		else if (resource->IsA(E::CResBlendState::StaticType) || resource->IsA(E::CResDepthStencilState::StaticType) || resource->IsA(E::CResRasterizerState::StaticType) || resource->IsA(E::CResSamplerState::StaticType) || resource->IsA(E::CResViewPort::StaticType))
		{
			item.category = "State";
			item.icon = "ST";
			item.color = ImVec4(0.37f, 0.51f, 0.57f, 1.f);
		}
		else if (resource->IsA(E::CResFmodSound::StaticType))
		{
			item.category = "Audio";
			item.icon = "A";
			item.color = ImVec4(0.73f, 0.33f, 0.39f, 1.f);
		}
		else if (resource->IsA(E::CResJson::StaticType) || resource->IsA(E::CResFont::StaticType) || resource->IsA(E::CResFontCustom::StaticType))
		{
			item.category = "Data";
			item.icon = "D";
			item.color = ImVec4(0.44f, 0.58f, 0.30f, 1.f);
		}
		else
		{
			item.category = "Other";
			item.icon = "R";
			item.color = ImVec4(0.40f, 0.40f, 0.43f, 1.f);
		}

		return item;
	}

	bool ContainsIgnoreCase(std::string_view text, std::string_view pattern)
	{
		if (pattern.empty())
		{
			return true;
		}

		std::string lowerText{ text };
		std::string lowerPattern{ pattern };
		std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		std::transform(lowerPattern.begin(), lowerPattern.end(), lowerPattern.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return lowerText.find(lowerPattern) != std::string::npos;
	}

	bool PassFilter(const Client::ResourceViewItem& item, int categoryIndex, std::string_view searchText)
	{
		if (categoryIndex > 0 && item.category != std::string_view{ CATEGORY_NAMES[categoryIndex] })
		{
			return false;
		}

		return ContainsIgnoreCase(item.resourceName, searchText)
			|| ContainsIgnoreCase(item.groupName, searchText)
			|| ContainsIgnoreCase(item.path, searchText)
			|| ContainsIgnoreCase(item.category, searchText);
	}

	void DrawCenteredText(const char* text, float width)
	{
		const float textWidth = ImGui::CalcTextSize(text).x;
		const float cursorX = ImGui::GetCursorPosX();
		if (textWidth < width)
		{
			ImGui::SetCursorPosX(cursorX + (width - textWidth) * 0.5f);
		}
		ImGui::TextUnformatted(text);
	}
}

class Client::CModelThumbnailCache final : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CModelThumbnailCache, CEngineBase)
private:
	struct Thumbnail
	{
		E::SPtr<E::CResOffscreenTexture> color{};
	};

public:
	void BeginFrame()
	{
		m_bRenderedThisFrame = false;
	}

	ID3D11ShaderResourceView* Request(const std::string& key, const E::SPtr<E::CResStaticModel>& model)
	{
		if (const auto it = m_Cache.find(key); it != m_Cache.end())
		{
			return it->second.color->GetSRV().Get();
		}

		if (m_bRenderedThisFrame || model == nullptr || model->GetState() != E::CResource::STATE::LOADED || !model->HasLocalBounds())
		{
			return nullptr;
		}

		m_bRenderedThisFrame = true;
		Thumbnail thumbnail{};
		if (FAILED(CreateTarget(thumbnail)) || FAILED(RenderModel(model, thumbnail)))
		{
			return nullptr;
		}

		auto [it, inserted] = m_Cache.emplace(key, std::move(thumbnail));
		return it->second.color->GetSRV().Get();
	}

private:
	HRESULT CreateTarget(Thumbnail& thumbnail)
	{
		constexpr UINT SIZE = 128;
		thumbnail.color = E::CResOffscreenTexture::Create();
		if (thumbnail.color == nullptr || FAILED(thumbnail.color->Load(E::CResOffscreenTexture::DESC{ SIZE, SIZE })))
		{
			return E_FAIL;
		}
		if (m_pSharedDSV != nullptr)
		{
			return S_OK;
		}

		auto device = E::CGameInstance::Get().GetGraphicDevice();
		D3D11_TEXTURE2D_DESC depthDesc{};
		depthDesc.Width = SIZE;
		depthDesc.Height = SIZE;
		depthDesc.MipLevels = 1;
		depthDesc.ArraySize = 1;
		depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthDesc.SampleDesc.Count = 1;
		depthDesc.Usage = D3D11_USAGE_DEFAULT;
		depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		if (FAILED(device->CreateTexture2D(&depthDesc, nullptr, m_pSharedDepth.GetAddressOf())))
		{
			return E_FAIL;
		}
		return device->CreateDepthStencilView(m_pSharedDepth.Get(), nullptr, m_pSharedDSV.GetAddressOf());
	}

	HRESULT RenderModel(const E::SPtr<E::CResStaticModel>& model, Thumbnail& thumbnail)
	{
		auto context = E::CGameInstance::Get().GetGraphicDeviceContext();
		auto vs = E::CGameInstance::Get().GetResourceFirst<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim");
		auto ps = E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim");
		auto objectCB = E::CGameInstance::Get().GetResourceFirst<E::CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT);
		auto passCB = E::CGameInstance::Get().GetResourceFirst<E::CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_PASS);
		auto materialCB = E::CGameInstance::Get().GetResourceFirst<E::CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_MATERIAL");
		if (!context || !vs || !ps || !objectCB || !passCB || !materialCB)
		{
			return E_FAIL;
		}

		ID3D11RenderTargetView* oldRTV = nullptr;
		ID3D11DepthStencilView* oldDSV = nullptr;
		context->OMGetRenderTargets(1, &oldRTV, &oldDSV);
		UINT oldViewportCount = 1;
		D3D11_VIEWPORT oldViewport{};
		context->RSGetViewports(&oldViewportCount, &oldViewport);

		ID3D11RenderTargetView* targetRTV = thumbnail.color->GetRTV().Get();
		context->OMSetRenderTargets(1, &targetRTV, m_pSharedDSV.Get());
		const float clearColor[4] = { 0.10f, 0.11f, 0.13f, 1.f };
		context->ClearRenderTargetView(targetRTV, clearColor);
		context->ClearDepthStencilView(m_pSharedDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
		D3D11_VIEWPORT viewport{ 0.f, 0.f, 128.f, 128.f, 0.f, 1.f };
		context->RSSetViewports(1, &viewport);

		const auto& bounds = model->GetLocalBounds();
		const E::_float3 center = bounds.Center;
		const float radius = std::max(DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMLoadFloat3(&bounds.Extents))), 0.01f);
		const E::_vector direction = DirectX::XMVector3Normalize(DirectX::XMVectorSet(1.f, 0.65f, -1.f, 0.f));
		const E::_vector target = DirectX::XMLoadFloat3(&center);
		const E::_vector eye = DirectX::XMVectorSubtract(target, DirectX::XMVectorScale(direction, radius * 2.7f));
		const E::_matrix view = DirectX::XMMatrixLookAtLH(eye, target, DirectX::XMVectorSet(0.f, 1.f, 0.f, 0.f));
		const E::_matrix proj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(35.f), 1.f, std::max(0.01f, radius * 0.02f), radius * 8.f);

		E::CB_PER_OBJECT objectData{};
		DirectX::XMStoreFloat4x4(&objectData.matWorld, DirectX::XMMatrixIdentity());
		DirectX::XMStoreFloat4x4(&objectData.matWVP, view * proj);
		E::CB_PER_PASS passData{};
		DirectX::XMStoreFloat4x4(&passData.matView, view);
		DirectX::XMStoreFloat4x4(&passData.matProj, proj);
		DirectX::XMStoreFloat4x4(&passData.matViewProj, view * proj);
		DirectX::XMStoreFloat4x4(&passData.matInvView, DirectX::XMMatrixInverse(nullptr, view));
		DirectX::XMStoreFloat4x4(&passData.matInvViewProj, DirectX::XMMatrixInverse(nullptr, view * proj));
		DirectX::XMStoreFloat3(&passData.vCamPos, eye);
		E::CB_MATERIAL materialData{ { 1.f, 1.f, 1.f }, 0.f, {1.f, 1.f, 1.f}, 0.f, 1.f, {} };

		if (FAILED(UpdateBuffer(context.Get(), objectCB->GetCBuffer().Get(), objectData)) ||
			FAILED(UpdateBuffer(context.Get(), passCB->GetCBuffer().Get(), passData)) ||
			FAILED(UpdateBuffer(context.Get(), materialCB->GetCBuffer().Get(), materialData)))
		{
			RestoreTarget(context.Get(), oldRTV, oldDSV, oldViewportCount, oldViewport);
			return E_FAIL;
		}

		ID3D11Buffer* objectBuffer = objectCB->GetCBuffer().Get();
		ID3D11Buffer* passBuffer = passCB->GetCBuffer().Get();
		ID3D11Buffer* materialBuffer = materialCB->GetCBuffer().Get();
		context->IASetInputLayout(vs->GetInputLayout().Get());
		context->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
		context->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);
		context->VSSetConstantBuffers(0, 1, &objectBuffer);
		context->VSSetConstantBuffers(1, 1, &passBuffer);
		context->PSSetConstantBuffers(3, 1, &materialBuffer);

		const auto& meshes = model->GetMeshes();
		for (uint32_t i = 0; i < meshes.size(); ++i)
		{
			const auto& mesh = meshes[i];
			if (!mesh) continue;
			ID3D11Buffer* vertexBuffer = mesh->GetVertexBuffer().Get();
			UINT stride = mesh->GetVertexStride();
			UINT offset = 0;
			context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
			context->IASetIndexBuffer(mesh->GetIndexBuffer().Get(), mesh->GetIndexFormat(), 0);
			context->IASetPrimitiveTopology(mesh->GetPrimitiveType());
			BindTextures(context.Get(), model, i);
			context->DrawIndexed(mesh->GetNumIndices(), 0, 0);
		}

		ID3D11ShaderResourceView* nullSRVs[4]{};
		context->PSSetShaderResources(0, 4, nullSRVs);
		RestoreTarget(context.Get(), oldRTV, oldDSV, oldViewportCount, oldViewport);
		return S_OK;
	}

	template<typename T>
	static HRESULT UpdateBuffer(ID3D11DeviceContext* context, ID3D11Buffer* buffer, const T& data)
	{
		D3D11_MAPPED_SUBRESOURCE mapped{};
		if (FAILED(context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return E_FAIL;
		memcpy(mapped.pData, &data, sizeof(T));
		context->Unmap(buffer, 0);
		return S_OK;
	}

	static void RestoreTarget(ID3D11DeviceContext* context, ID3D11RenderTargetView* rtv, ID3D11DepthStencilView* dsv, UINT viewportCount, const D3D11_VIEWPORT& viewport)
	{
		context->OMSetRenderTargets(1, &rtv, dsv);
		if (viewportCount > 0) context->RSSetViewports(1, &viewport);
		if (rtv) rtv->Release();
		if (dsv) dsv->Release();
	}

	static void BindTextures(ID3D11DeviceContext* context, const E::SPtr<E::CResStaticModel>& model, uint32_t meshIndex)
	{
		const AI_TEXTURE_TYPE types[4] = { AI_TEXTURE_TYPE::aiTextureType_DIFFUSE, AI_TEXTURE_TYPE::aiTextureType_NORMALS, AI_TEXTURE_TYPE::aiTextureType_METALNESS, AI_TEXTURE_TYPE::aiTextureType_EMISSIVE };
		const char* defaults[4] = { "TEX_DEFAULT_DIFFUSE", "TEX_DEFAULT_NORMAL", "TEX_DEFAULT_SMRO", "TEX_DEFAULT_EMISSIVE" };
		ID3D11ShaderResourceView* srvs[4]{};
		const auto& mesh = model->GetMeshes()[meshIndex];
		const auto& materials = model->GetMaterials();
		for (size_t i = 0; i < 4; ++i)
		{
			auto texture = E::CGameInstance::Get().GetResourceFirst<E::CResTexture2D>("DEFAULT_TEXTURE", defaults[i]);
			if (mesh->Get_MaterialIndex() < materials.size() && materials[mesh->Get_MaterialIndex()])
			{
				auto textures = materials[mesh->Get_MaterialIndex()]->GetTextures();
				if (!textures[types[i]].empty()) texture = textures[types[i]].front();
			}
			srvs[i] = texture ? texture->GetSRV().Get() : nullptr;
		}
		context->PSSetShaderResources(0, 4, srvs);
	}

private:
	std::unordered_map<std::string, Thumbnail> m_Cache{};
	ComPtr<ID3D11Texture2D> m_pSharedDepth{};
	ComPtr<ID3D11DepthStencilView> m_pSharedDSV{};
	bool m_bRenderedThisFrame = false;
};

CResourceGUI::CResourceGUI()
	: m_pThumbnailCache{ E::ToUPtr(new CModelThumbnailCache{}) }
{
}

CResourceGUI::~CResourceGUI()
{
}
HRESULT CResourceGUI::CachingAllResource()
{
	m_Items.clear();
	if(m_Items.capacity() < 500)
		m_Items.reserve(500);

	const auto& resourceGroups = E::CGameInstance::Get().GetResources();
	for (const auto& [groupId, resources] : resourceGroups)
	{
		for (const auto& [resourceId, resourceList] : resources)
		{
			for (size_t i = 0; i < resourceList.size(); ++i)
			{
				m_Items.emplace_back(MakeViewItem(groupId, resourceId, resourceList[i], i, resourceList.size()));
			}
		}
	}

	std::sort(m_Items.begin(), m_Items.end(), [](const ResourceViewItem& lhs, const ResourceViewItem& rhs)
		{
			if (lhs.category != rhs.category)
			{
				return std::string_view{ lhs.category } < std::string_view{ rhs.category };
			}
			if (lhs.groupName != rhs.groupName)
			{
				return lhs.groupName < rhs.groupName;
			}
			return lhs.resourceName < rhs.resourceName;
		});



	return S_OK;
}
void CResourceGUI::UpdateGUI(E::_float fTimeDelta)
{
	m_pThumbnailCache->BeginFrame();
	ImGui::SetNextWindowSize(ImVec2(560.f, 420.f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Resources"))
	{
		ImGui::End();
		return;
	}

	ImGui::SetNextItemWidth(220.f);
	ImGui::InputTextWithHint("##ResourceSearch", "Search resources...", m_SearchBuffer, sizeof(m_SearchBuffer));
	ImGui::SameLine();
	if (ImGui::Button("Import Whole Map Manifest"))
	{
		SelectAndImportWholeMapManifest();
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80.f);
	if (ImGui::DragFloat("Whole Map Scale", &m_fWholeMapScale, 0.01f, 0.01f, 100.f, "%.2f"))
		m_fWholeMapScale = std::clamp(m_fWholeMapScale, 0.01f, 100.f);
	ImGui::SetNextItemWidth(300.f);
	ImGui::DragFloat3("Whole Map Origin", &m_vWholeMapOrigin.x, 0.1f, 0.f, 0.f, "%.2f");
	ImGui::SameLine();
	if (ImGui::Button("Reset Origin"))
		m_vWholeMapOrigin = {};
	if (!m_WholeMapImportStatus.empty())
	{
		ImGui::TextWrapped("%s", m_WholeMapImportStatus.c_str());
	}
	if (ImGui::Button("Refresh Resouce List"))
	{
		CachingAllResource();
	}

	if (ImGui::BeginTabBar("##ResourceCategories"))
	{
		for (int i = 0; i < static_cast<int>(std::size(CATEGORY_NAMES)); ++i)
		{
			if (ImGui::BeginTabItem(CATEGORY_NAMES[i]))
			{
				m_SelectedCategory = i;
				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();
	}


	ImGui::Separator();
	ImGui::BeginChild("##ResourceGrid", ImVec2(0.f, 0.f), true, ImGuiWindowFlags_HorizontalScrollbar);

	constexpr float iconSize = 64.f;
	constexpr float cellWidth = 96.f;
	constexpr float cellHeight = 112.f;
	const float availableWidth = std::max(ImGui::GetContentRegionAvail().x, cellWidth);
	const int columns = std::max(1, static_cast<int>(availableWidth / cellWidth));

	m_FilteredItemIndices.clear();
	if (m_FilteredItemIndices.capacity() < m_Items.size())
		m_FilteredItemIndices.reserve(m_Items.size());

	for (size_t i = 0; i < m_Items.size(); ++i)
	{
		if (PassFilter(m_Items[i], m_SelectedCategory, m_SearchBuffer))
			m_FilteredItemIndices.push_back(i);
	}

	const int itemCount = static_cast<int>(m_FilteredItemIndices.size());
	const int rowCount = (itemCount + columns - 1) / columns;

	ImGuiListClipper clipper;
	clipper.Begin(rowCount, cellHeight);
	while (clipper.Step())
	{
		for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
		{
			const ImVec2 rowStart = ImGui::GetCursorPos();

			for (int column = 0; column < columns; ++column)
			{
				const int visibleIndex = row * columns + column;
				if (visibleIndex >= itemCount)
					break;

				const auto& item = m_Items[m_FilteredItemIndices[visibleIndex]];
				ImGui::SetCursorPos(ImVec2(rowStart.x + column * cellWidth, rowStart.y));

				ImGui::PushID(visibleIndex);
				ImGui::BeginGroup();

				const float cursorX = ImGui::GetCursorPosX();
				ImGui::SetCursorPosX(cursorX + (cellWidth - iconSize) * 0.5f);
				ID3D11ShaderResourceView* thumbnail = nullptr;
				if (item.staticModel)
				{
					const std::string thumbnailKey = item.groupName + "\x1f" + item.resourceTag;
					thumbnail = m_pThumbnailCache->Request(thumbnailKey, item.staticModel);
				}
				if (thumbnail != nullptr)
				{
					ImGui::ImageButton(reinterpret_cast<ImTextureID>(thumbnail), ImVec2(iconSize, iconSize));
				}
				else
				{
					ImGui::PushStyleColor(ImGuiCol_Button, item.color);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(item.color.x + 0.08f, item.color.y + 0.08f, item.color.z + 0.08f, 1.f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(item.color.x * 0.82f, item.color.y * 0.82f, item.color.z * 0.82f, 1.f));
					ImGui::Button(item.icon, ImVec2(iconSize, iconSize));
					ImGui::PopStyleColor(3);
				}

				if (item.bCanCreateMapMeshObject && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
				{
					ModelResourceDragPayload payload{};
					strcpy_s(payload.groupName, item.groupName.c_str());
					strcpy_s(payload.resourceName, item.resourceTag.c_str());
					ImGui::SetDragDropPayload(PAYLOAD_MODEL_RESOURCE, &payload, sizeof(payload));
					m_DragModelGroup = item.groupName;
					m_DragModelTag = item.resourceTag;
					m_bDraggingModel = true;
					ImGui::Text("Create MapMeshObject");
					ImGui::Text("%s / %s", payload.groupName, payload.resourceName);
					ImGui::EndDragDropSource();
				}

				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::Text("Name: %s", item.resourceName.c_str());
					ImGui::Text("Type: %s", item.category);
					ImGui::Text("Group: %s", item.groupName.c_str());
					ImGui::Text("State: %s", item.state.c_str());
					if (!item.path.empty())
					{
						ImGui::TextWrapped("Path: %s", item.path.c_str());
					}
					ImGui::EndTooltip();
				}

				ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + cellWidth - 6.f);
				DrawCenteredText(item.resourceName.c_str(), cellWidth - 6.f);
				ImGui::PopTextWrapPos();

				ImGui::EndGroup();
				ImGui::PopID();
			}

			ImGui::SetCursorPos(ImVec2(rowStart.x, rowStart.y + cellHeight));
		}
	}

	if (itemCount == 0)
	{
		ImGui::TextDisabled("No resources found.");
	}

	ImGui::EndChild();
	HandleModelDropToScene();
	ImGui::End();
}

void CResourceGUI::HandleModelDropToScene()
{
	if (!m_bDraggingModel || !ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		return;

	const bool droppedOnScene = !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
	if (droppedOnScene)
	{
		auto* camera = E::CGameInstance::Get().GetActiveCamera();
		const E::_float2 mouse = E::CGameInstance::Get().GetMousePos();
		const E::_float2 clientSize = E::CGameInstance::Get().GetClientScreenSize();
		if (camera != nullptr && clientSize.x > 0.f && clientSize.y > 0.f &&
			mouse.x >= 0.f && mouse.y >= 0.f && mouse.x < clientSize.x && mouse.y < clientSize.y)
		{
			const E::_matrix identity = XMMatrixIdentity();
			const E::_vector nearPoint = XMVector3Unproject(
				XMVectorSet(mouse.x, mouse.y, 0.f, 1.f),
				0.f, 0.f, clientSize.x, clientSize.y, 0.f, 1.f,
				camera->GetProj(), camera->GetView(), identity);
			const E::_vector farPoint = XMVector3Unproject(
				XMVectorSet(mouse.x, mouse.y, 1.f, 1.f),
				0.f, 0.f, clientSize.x, clientSize.y, 0.f, 1.f,
				camera->GetProj(), camera->GetView(), identity);
			const E::_vector worldPosition = nearPoint +
				XMVector3Normalize(farPoint - nearPoint) * m_fSceneDropDistance;

			E::_float3 position{};
			XMStoreFloat3(&position, worldPosition);
			CreateDroppedMapMeshObject(position);
		}
	}

	m_bDraggingModel = false;
	m_DragModelGroup.clear();
	m_DragModelTag.clear();
}

void CResourceGUI::CreateDroppedMapMeshObject(const E::_float3& worldPosition)
{
	if (m_DragModelGroup.empty() || m_DragModelTag.empty() || m_pCommandManager == nullptr)
		return;

	static uint32_t s_iSceneDropIndex = 1;

	MAPMESH_OBJECT_SNAPSHOT snapshot{};
	snapshot.objectTag = "MapMesh_" + m_DragModelTag + "_" +
		std::to_string(s_iSceneDropIndex++);
	snapshot.modelGroupTag = m_DragModelGroup;
	snapshot.modelResTag = m_DragModelTag;
	snapshot.layerTag = E::MAPMESHOBJECTLAYER;
	snapshot.position = worldPosition;
	m_pCommandManager->Submit(
		std::make_unique<CCreateMapMeshCommand>(std::move(snapshot), GetSelectedHandle()));
}

void CResourceGUI::SelectAndImportWholeMapManifest()
{
	char selectedPath[MAX_PATH]{};
	OPENFILENAMEA dialog{};
	dialog.lStructSize = sizeof(dialog);
	dialog.hwndOwner = g_hWnd;
	dialog.lpstrFilter = "Whole Map Manifest (*_RenderChunks.json)\0*_RenderChunks.json\0JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
	dialog.lpstrFile = selectedPath;
	dialog.nMaxFile = MAX_PATH;
	dialog.lpstrInitialDir = E::PATH_MAPEDITOR_STATIC_MODEL_DIR;
	dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

	if (!GetOpenFileNameA(&dialog))
		return;

	ImportWholeMapManifest(selectedPath);
}

_bool CResourceGUI::ImportWholeMapManifest(const std::filesystem::path& manifestPath)
{
	if (m_pCommandManager == nullptr)
	{
		m_WholeMapImportStatus = "Whole-map import failed: command manager is unavailable.";
		return false;
	}

	std::ifstream file(manifestPath);
	if (!file.is_open())
	{
		m_WholeMapImportStatus = "Whole-map import failed: cannot open manifest.";
		return false;
	}

	nlohmann::json manifest;
	try
	{
		file >> manifest;
	}
	catch (const std::exception& exception)
	{
		m_WholeMapImportStatus = std::string("Whole-map import failed: ") + exception.what();
		return false;
	}

	if (!manifest.contains("chunks") || !manifest["chunks"].is_array())
	{
		m_WholeMapImportStatus = "Whole-map import failed: chunks array is missing.";
		return false;
	}

	const std::filesystem::path resourceRoot = E::PATH_MAPEDITOR_STATIC_MODEL_DIR;
	const std::string modelName = manifest.value("modelName", manifestPath.stem().string());
	uint32_t createdCount = 0;
	uint32_t skippedCount = 0;

	for (const auto& chunk : manifest["chunks"])
	{
		if (!chunk.contains("file") || !chunk["file"].is_string() ||
			!chunk.contains("localOrigin") || !chunk["localOrigin"].is_array() ||
			chunk["localOrigin"].size() != 3)
		{
			++skippedCount;
			continue;
		}

		const std::filesystem::path binPath = manifestPath.parent_path() /
			chunk["file"].get<std::string>();
		const std::string resourceTag = MakeStaticModelResourceTag(resourceRoot, binPath);
		const auto modelResource = E::CGameInstance::Get().GetResourceFirst<E::CResStaticModel>(
			E::TAG_RES_GRP_MAPEDITOR_STATIC_MODEL, resourceTag);
		if (modelResource == nullptr)
		{
			++skippedCount;
			continue;
		}

		const auto& origin = chunk["localOrigin"];
		MAPMESH_OBJECT_SNAPSHOT snapshot{};
		snapshot.objectTag = "WholeMap_" + modelName + "_" +
			std::to_string(chunk.value("x", 0)) + "_" +
			std::to_string(chunk.value("y", 0)) + "_" +
			std::to_string(chunk.value("z", 0));
		snapshot.modelGroupTag = E::TAG_RES_GRP_MAPEDITOR_STATIC_MODEL;
		snapshot.modelResTag = resourceTag;
		snapshot.layerTag = E::MAPMESHOBJECTLAYER;

		const float wholeMapScale = std::clamp(m_fWholeMapScale, 0.01f, 100.f);

		snapshot.position = {
			m_vWholeMapOrigin.x + origin[0].get<float>() * wholeMapScale,
			m_vWholeMapOrigin.y + origin[1].get<float>() * wholeMapScale,
			m_vWholeMapOrigin.z + origin[2].get<float>() * wholeMapScale
		};

		snapshot.scale = {
			wholeMapScale,
			wholeMapScale,
			wholeMapScale
		};

		m_pCommandManager->Submit(
			std::make_unique<CCreateMapMeshCommand>(std::move(snapshot), GetSelectedHandle()));
		++createdCount;
	}

	m_WholeMapImportStatus = "Whole-map import: " + std::to_string(createdCount) +
		" chunks created";
	if (skippedCount > 0)
		m_WholeMapImportStatus += ", " + std::to_string(skippedCount) + " skipped";
	m_WholeMapImportStatus += ".";
	return createdCount > 0;
}

E::UPtr<CResourceGUI> CResourceGUI::Create(E::CHandle* pSelectedObject,
	CEditorCommandManager* pCommandManager)
{
	auto pInstance = E::UPtr<CResourceGUI>(new CResourceGUI{});
	if (FAILED(pInstance->Initialize(pSelectedObject)) || FAILED(pInstance->CachingAllResource()))
	{
		MSG_BOX("Failed to Created : CResourceGUI");
		return nullptr;
	}
	pInstance->m_pCommandManager = pCommandManager;

	return pInstance;
}
