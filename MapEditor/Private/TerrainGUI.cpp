#include "pch.h"
#include "TerrainGUI.h"

#include "DbgLineRender.h"
#include "GameInstance.h"
#include "Terrain.h"
#include "TerrainBrushController.h"
#include "TerrainPickingPass.h"
#include "Resources.h"

NS_USING(Client)

void CTerrainGUI::UpdateGUI(E::_float fTimeDelta)
{
	auto* selectedHandle = GetSelectedHandle();
	if (!selectedHandle)
		return;
	auto* terrain = E::CGameInstance::Get().GetGameObjectByHandleT<E::CTerrain>(*selectedHandle);
	if (!terrain)
		return;

	ImGui::Separator();
	ImGui::TextDisabled("Terrain");
	ImGui::Text("Chunks: %u / %u visible",
		terrain->GetVisibleChunkCount(), static_cast<uint32_t>(terrain->GetChunks().size()));
	if (ImGui::Button("Add Chunk +X"))
		terrain->AddChunkPositiveX();
	ImGui::SameLine();
	if (ImGui::Button("Add Chunk +Z"))
		terrain->AddChunkPositiveZ();
	if (ImGui::Button("Add Chunk -X"))
		terrain->AddChunkNegativeX();
	ImGui::SameLine();
	if (ImGui::Button("Add Chunk -Z"))
		terrain->AddChunkNegativeZ();
	if (ImGui::Checkbox("Sculpt", &m_bSculptEnabled) && m_bSculptEnabled)
		m_bTexturePaintEnabled = false;
	ImGui::SameLine();
	if (ImGui::Checkbox("Paint Texture", &m_bTexturePaintEnabled) && m_bTexturePaintEnabled)
		m_bSculptEnabled = false;
	ImGui::Checkbox("GPU Picking Debug", &m_bPickingDebug);
	auto& brush = m_pBrushController->GetSettings();
	int brushMode = brush.mode == ETerrainBrushMode::Raise ? 0 : 1;
	ImGui::RadioButton("Raise", &brushMode, 0);
	ImGui::SameLine();
	ImGui::RadioButton("Lower", &brushMode, 1);
	brush.mode = brushMode == 0 ? ETerrainBrushMode::Raise : ETerrainBrushMode::Lower;
	ImGui::SliderFloat("Brush Radius", &brush.radius, 0.5f, 50.f, "%.1f");
	ImGui::SliderFloat("Brush Strength", &brush.strength, 0.1f, 30.f, "%.1f");
	ImGui::SliderFloat("Brush Falloff", &brush.falloff, 0.1f, 8.f, "%.1f");

	if (m_bTexturePaintEnabled)
	{
		int selectedLayer = static_cast<int>(brush.tileLayer);
		ImGui::TextUnformatted("Paint Layer");
		for (int layer = 0; layer < 4; ++layer)
		{
			if (layer > 0) ImGui::SameLine();
			const std::string label = "Layer " + std::to_string(layer);
			ImGui::RadioButton(label.c_str(), &selectedLayer, layer);
		}
		brush.tileLayer = static_cast<uint32_t>(selectedLayer);

		for (uint32_t layer = 0; layer < 4; ++layer)
		{
			ImGui::PushID(static_cast<int>(layer));
			auto current = terrain->GetTileTexture(layer);
			const std::string preview = current ? std::filesystem::path(current->GetPath()).filename().string() : "None";
			ImGui::Separator();
			ImGui::Text("Tile %u%s", layer, brush.tileLayer == layer ? " (Paint)" : "");
			if (current && current->GetSRV())
			{
				const auto& textureDesc = current->GetTexture2DDesc();
				constexpr float thumbnailExtent = 64.f;
				const float aspect = textureDesc.Height > 0
					? static_cast<float>(textureDesc.Width) / static_cast<float>(textureDesc.Height)
					: 1.f;
				const ImVec2 thumbnailSize = aspect >= 1.f
					? ImVec2{ thumbnailExtent, thumbnailExtent / aspect }
					: ImVec2{ thumbnailExtent * aspect, thumbnailExtent };
				ImGui::Image(
					reinterpret_cast<ImTextureID>(current->GetSRV().Get()), thumbnailSize);
			}
			else
			{
				ImGui::Dummy({ 64.f, 64.f });
			}
			ImGui::SameLine();
			ImGui::BeginGroup();
			ImGui::TextWrapped("%s", current ? current->GetPath().c_str() : "No texture selected");
			ImGui::TextDisabled("%s", preview.c_str());
			ImGui::EndGroup();
			ImGui::PopID();
		}

		ImGui::Separator();
		ImGui::Text("Available Tiles -> Layer %u", brush.tileLayer);
		const auto groups = E::CGameInstance::Get().GetResources();
		const auto groupIt = groups.find(E::StringID{ "MAPEDITOR_TERRAIN_TILE" });
		if (groupIt == groups.end())
		{
			ImGui::TextDisabled("MAPEDITOR_TERRAIN_TILE is empty");
		}
		else
		{
			constexpr float tileSize = 72.f;
			constexpr float tilePanelHeight = 260.f;
			ImGui::BeginChild("TerrainTilePalette", { 0.f, tilePanelHeight }, true,
				ImGuiWindowFlags_AlwaysVerticalScrollbar);
			const float cellWidth = tileSize + ImGui::GetStyle().ItemSpacing.x;
			const int columnCount = std::max(1,
				static_cast<int>(ImGui::GetContentRegionAvail().x / cellWidth));
			int tileIndex = 0;
			for (const auto& [resourceId, list] : groupIt->second)
			{
				for (const auto& resource : list)
				{
					if (!resource || !resource->IsA(E::CResTexture2D::StaticType)) continue;
					auto texture = std::static_pointer_cast<E::CResTexture2D>(resource);
					if (!texture->GetSRV()) continue;
					ImGui::PushID(texture.get());
					if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(texture->GetSRV().Get()),
						{ tileSize, tileSize }))
						terrain->SetTileTexture(brush.tileLayer, texture);
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip("%s", texture->GetPath().c_str());
					ImGui::PopID();
					++tileIndex;
					if (tileIndex % columnCount != 0)
						ImGui::SameLine();
				}
			}
			ImGui::EndChild();
		}
	}

	if (!m_bPickingDebug && !m_bSculptEnabled && !m_bTexturePaintEnabled)
	{
		m_PickedPosition.reset();
		m_pBrushController->EndStroke();
		return;
	}

	const ImGuiIO& io = ImGui::GetIO();
	const float brushTimeDelta = fTimeDelta > 0.f ? fTimeDelta : io.DeltaTime;
	const E::_float2 mouse = E::CGameInstance::Get().GetMousePos();
	const E::_float2 clientSize = E::CGameInstance::Get().GetClientScreenSize();
	const bool mouseInClient = mouse.x >= 0.f && mouse.y >= 0.f &&
		mouse.x < clientSize.x && mouse.y < clientSize.y;
	if (!io.WantCaptureMouse && mouseInClient && !ImGuizmo::IsOver() && m_pPickingPass)
	{
		m_PickedPosition = m_pPickingPass->Pick(
			*terrain, static_cast<uint32_t>(mouse.x), static_cast<uint32_t>(mouse.y));
	}
	else if (!mouseInClient)
	{
		m_PickedPosition.reset();
	}

	if (!m_PickedPosition)
	{
		ImGui::TextUnformatted("Hit: none");
		m_pBrushController->EndStroke();
		return;
	}

	const auto& hit = *m_PickedPosition;
	ImGui::Text("Hit: %.2f, %.2f, %.2f", hit.x, hit.y, hit.z);
	if (m_bSculptEnabled)
	{
		m_pBrushController->DrawPreview(*terrain, hit);
		if (!io.WantCaptureMouse && ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
			!ImGuizmo::IsOver() && !ImGuizmo::IsUsing())
		{
			m_pBrushController->UpdateStroke(*terrain, hit, brushTimeDelta);
		}
		else
		{
			m_pBrushController->EndStroke();
		}
	}
	else if (m_bTexturePaintEnabled)
	{
		m_pBrushController->DrawPreview(*terrain, hit);
		if (!io.WantCaptureMouse && ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
			!ImGuizmo::IsOver() && !ImGuizmo::IsUsing())
			m_pBrushController->UpdateTextureStroke(*terrain, hit, brushTimeDelta);
		else
			m_pBrushController->EndStroke();
	}
	if (auto* debugDraw = E::CGameInstance::Get().GetDbgLineRender())
	{
		constexpr float markerSize = 0.5f;
		debugDraw->SetColor({ 1.f, 0.2f, 0.1f, 1.f });
		debugDraw->AddLine({ hit.x - markerSize, hit.y, hit.z }, { hit.x + markerSize, hit.y, hit.z });
		debugDraw->AddLine({ hit.x, hit.y - markerSize, hit.z }, { hit.x, hit.y + markerSize, hit.z });
		debugDraw->AddLine({ hit.x, hit.y, hit.z - markerSize }, { hit.x, hit.y, hit.z + markerSize });
	}
}

E::UPtr<CTerrainGUI> CTerrainGUI::Create(E::CHandle* selectedObject)
{
	auto instance = E::ToUPtr(new CTerrainGUI{});
	if (FAILED(instance->Initialize(selectedObject)))
		return nullptr;
	instance->m_pPickingPass = CTerrainPickingPass::Create();
	if (!instance->m_pPickingPass)
		return nullptr;
	instance->m_pBrushController = CTerrainBrushController::Create();
	if (!instance->m_pBrushController)
		return nullptr;
	return instance;
}
