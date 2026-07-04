#include "pch.h"
#include "ResourceGUI.h"
#include "GameInstance.h"
#include "Resources.h"

NS_USING(Client)

namespace
{
	struct ResourceViewItem
	{
		const char* category{};
		const char* icon{};
		ImVec4 color{};
		std::string groupName{};
		std::string resourceName{};
		std::string path{};
		std::string state{};
	};

	const char* const CATEGORY_NAMES[] =
	{
		"All", "Model", "Texture", "Shader", "Buffer", "State", "Audio", "Data", "Other"
	};

	std::string SafeDbgStr(const E::StringID& id)
	{
		const char* text = id.GetDbgStr();
		return text != nullptr ? text : "<unnamed>";
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

	ResourceViewItem MakeViewItem(const E::StringID& groupId, const E::StringID& resourceId, const E::SPtr<E::CResource>& resource, size_t index, size_t count)
	{
		ResourceViewItem item{};
		item.groupName = SafeDbgStr(groupId);
		item.resourceName = SafeDbgStr(resourceId);
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
		else if (resource->IsA(E::CResTestModel::StaticType) || resource->IsA(E::CResTestModelMesh::StaticType) || resource->IsA(E::CResTestModelMaterial::StaticType) || resource->IsA(E::CResTestModelAnim::StaticType))
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

	bool PassFilter(const ResourceViewItem& item, int categoryIndex, std::string_view searchText)
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

CResourceGUI::CResourceGUI()
{
}

CResourceGUI::~CResourceGUI()
{
}

void CResourceGUI::UpdateGUI(E::_float fTimeDelta)
{
	ImGui::SetNextWindowSize(ImVec2(560.f, 420.f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Resources");

	ImGui::SetNextItemWidth(220.f);
	ImGui::InputTextWithHint("##ResourceSearch", "Search resources...", m_SearchBuffer, sizeof(m_SearchBuffer));

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

	std::vector<ResourceViewItem> items{};
	const auto& resourceGroups = E::CGameInstance::Get().GetResources();
	for (const auto& [groupId, resources] : resourceGroups)
	{
		for (const auto& [resourceId, resourceList] : resources)
		{
			for (size_t i = 0; i < resourceList.size(); ++i)
			{
				items.push_back(MakeViewItem(groupId, resourceId, resourceList[i], i, resourceList.size()));
			}
		}
	}

	std::sort(items.begin(), items.end(), [](const ResourceViewItem& lhs, const ResourceViewItem& rhs)
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

	ImGui::Separator();
	ImGui::BeginChild("##ResourceGrid", ImVec2(0.f, 0.f), true, ImGuiWindowFlags_HorizontalScrollbar);

	constexpr float iconSize = 64.f;
	constexpr float cellWidth = 96.f;
	constexpr float cellHeight = 112.f;
	const float availableWidth = std::max(ImGui::GetContentRegionAvail().x, cellWidth);
	const int columns = std::max(1, static_cast<int>(availableWidth / cellWidth));
	int visibleIndex = 0;

	for (const auto& item : items)
	{
		if (!PassFilter(item, m_SelectedCategory, m_SearchBuffer))
		{
			continue;
		}

		ImGui::PushID(visibleIndex);
		ImGui::BeginGroup();

		const float cursorX = ImGui::GetCursorPosX();
		ImGui::SetCursorPosX(cursorX + (cellWidth - iconSize) * 0.5f);
		ImGui::PushStyleColor(ImGuiCol_Button, item.color);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(item.color.x + 0.08f, item.color.y + 0.08f, item.color.z + 0.08f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(item.color.x * 0.82f, item.color.y * 0.82f, item.color.z * 0.82f, 1.f));
		ImGui::Button(item.icon, ImVec2(iconSize, iconSize));
		ImGui::PopStyleColor(3);

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

		++visibleIndex;
		if (visibleIndex % columns != 0)
		{
			ImGui::SameLine();
		}
	}

	if (visibleIndex == 0)
	{
		ImGui::TextDisabled("No resources found.");
	}

	ImGui::EndChild();
	ImGui::End();
}

E::UPtr<CResourceGUI> CResourceGUI::Create(E::CHandle* pSelectedObject)
{
	auto pInstance = E::UPtr<CResourceGUI>(new CResourceGUI{});
	if (FAILED(pInstance->Initialize(pSelectedObject)))
	{
		MSG_BOX("Failed to Created : CResourceGUI");
		return nullptr;
	}

	return pInstance;
}
