#include "pch.h"
#include "MapChunkGUI.h"
#include "GameInstance.h"
NS_USING(Client)

namespace
{
	constexpr const char* MAP_SAVE_ROOT = "./Resources/Engine/MapSaved/";

	std::string MakeMapPath(const char* mapName)
	{
		std::string cleanName = mapName;
		if (cleanName.empty())
		{
			cleanName = "Default";
		}

		for (char& ch : cleanName)
		{
			switch (ch)
			{
			case '/':
			case '\\':
			case ':':
			case '*':
			case '?':
			case '"':
			case '<':
			case '>':
			case '|':
				ch = '_';
				break;
			default:
				break;
			}
		}

		return std::string(MAP_SAVE_ROOT) + cleanName + "/";
	}

	std::string CoordToString(const E::MAPCHUNK_COORD& coord)
	{
		return "(" + std::to_string(coord.x) + ", " + std::to_string(coord.y) + ", " + std::to_string(coord.z) + ")";
	}

	const char* LoadStateText(E::EChunkLoadState state)
	{
		switch (state)
		{
		case E::EChunkLoadState::Unloaded:
			return "Unloaded";
		case E::EChunkLoadState::Loading:
			return "Loading";
		case E::EChunkLoadState::Loaded:
			return "Loaded";
		case E::EChunkLoadState::Unloading:
			return "Unloading";
		default:
			return "Unknown";
		}
	}

	const char* SaveStateText(E::EChunkSaveState state)
	{
		switch (state)
		{
		case E::EChunkSaveState::Unsaved:
			return "Unsaved";
		case E::EChunkSaveState::Saved:
			return "Saved";
		default:
			return "Unknown";
		}
	}

	ImU32 ChunkColor(const E::CMapChunk& chunk, bool selected)
	{
		if (selected)
		{
			return IM_COL32(255, 215, 96, 220);
		}
		if (chunk.GetSaveState() == E::EChunkSaveState::Unsaved)
		{
			return IM_COL32(255, 145, 80, 170);
		}
		return IM_COL32(120, 150, 170, 140);
	}

	void DrawBoundsText(const BoundingBox& bounds)
	{
		ImGui::Text("Center  %.2f, %.2f, %.2f", bounds.Center.x, bounds.Center.y, bounds.Center.z);
		ImGui::Text("Extents %.2f, %.2f, %.2f", bounds.Extents.x, bounds.Extents.y, bounds.Extents.z);
	}
}

CMapChunkGUI::CMapChunkGUI()
{
}

CMapChunkGUI::~CMapChunkGUI()
{
}

void CMapChunkGUI::UpdateGUI(E::_float fTimeDelta)
{
	if (m_bAutoRebuild)
	{
		E::CGameInstance::Get().RebuildMapChunks();
	}

	ImGui::SetNextWindowSize(ImVec2(520.f, 460.f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Map Chunks");

	ImGui::SetNextItemWidth(180.f);
	ImGui::InputText("Map", m_MapName, sizeof(m_MapName));

	if (ImGui::Button("Rebuild Chunks", ImVec2(130.f, 0.f)))
	{
		E::CGameInstance::Get().RebuildMapChunks();
	}
	ImGui::SameLine();
	if (ImGui::Button("Load Meta", ImVec2(100.f, 0.f)))
	{
		E::CGameInstance::Get().LoadMapData(MakeMapPath(m_MapName));
		m_bHasSelection = false;
	}
	ImGui::SameLine();
#ifdef _DEBUG
	if (ImGui::Button("DebugDraw Chunks", ImVec2(130.f, 0.f)))
	{
		m_bDebugDrawChunk = !m_bDebugDrawChunk;
		E::CGameInstance::Get().SetDebugDrawMapChunk(m_bDebugDrawChunk);
	}
	ImGui::SameLine();
#endif
	m_bChunkStreaming = E::CGameInstance::Get().IsMapChunkStreaming();
	bool chunkStreaming = m_bChunkStreaming;
	if (ImGui::Checkbox("Streaming", &chunkStreaming))
	{
		m_bChunkStreaming = chunkStreaming;
		E::CGameInstance::Get().SetMapChunkStreaming(m_bChunkStreaming);
	}
	ImGui::SameLine();
	ImGui::Checkbox("Auto", reinterpret_cast<bool*>(&m_bAutoRebuild));


	const auto& chunks = E::CGameInstance::Get().GetMapChunks();
	const auto& chunkSize = E::CGameInstance::Get().GetMapChunkSize();
	ImGui::Text("Chunk Count: %zu", chunks.size());
	ImGui::Text("Chunk Size : %.1f, %.1f, %.1f", chunkSize.x, chunkSize.y, chunkSize.z);
	ImGui::Separator();

	if (ImGui::BeginTabBar("##MapChunkTabs"))
	{
		if (ImGui::BeginTabItem("List"))
		{
			ImGui::BeginChild("##ChunkList", ImVec2(0.f, 190.f), true);
			for (const auto& [coord, chunk] : chunks)
			{
				const bool selected = m_bHasSelection && coord == m_SelectedCoord;
				std::string label = CoordToString(coord) + "  objects: " + std::to_string(chunk.GetObjectHandles().size());
				if (ImGui::Selectable(label.c_str(), selected))
				{
					m_SelectedCoord = coord;
					m_ViewY = coord.y;
					m_bHasSelection = true;
				}
			}
			ImGui::EndChild();

			if (m_bHasSelection)
			{
				auto iter = chunks.find(m_SelectedCoord);
				if (iter != chunks.end())
				{
					const auto& chunk = iter->second;
					ImGui::Text("Selected: %s", CoordToString(chunk.GetCoord()).c_str());
					DrawBoundsText(chunk.GetBounds());
					ImGui::Text("Load: %s", LoadStateText(chunk.GetLoadState()));
					ImGui::Text("Save: %s", SaveStateText(chunk.GetSaveState()));
					ImGui::Text("File: %s", chunk.GetFilePath().c_str());

					if (ImGui::Button("Load Selected Chunk", ImVec2(160.f, 0.f)))
					{
						E::CGameInstance::Get().LoadMapChunk(m_SelectedCoord);
					}
					ImGui::SameLine();
					if (ImGui::Button("Unload Selected Chunk", ImVec2(170.f, 0.f)))
					{
						E::CGameInstance::Get().UnLoadMapChunk(m_SelectedCoord);
					}

					if (ImGui::TreeNode("Objects"))
					{
						for (const auto& handle : chunk.GetObjectHandles())
						{
							auto* object = E::CGameInstance::Get().GetGameObjectByHandle(handle);
							if (!object)
							{
								continue;
							}
							ImGui::Text("%u:%u  %s", handle.GetIndex(), handle.GetGeneration(), object->GetObjectTag().data());
						}
						ImGui::TreePop();
					}
				}
				else
				{
					m_bHasSelection = false;
				}
			}
			else
			{
				ImGui::TextDisabled("Select a chunk to inspect details.");
			}

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("XZ View"))
		{
			ImGui::Checkbox("Filter Y", reinterpret_cast<bool*>(&m_bFilterY));
			ImGui::SameLine();
			ImGui::SetNextItemWidth(120.f);
			ImGui::InputScalar("Y Layer", ImGuiDataType_S64, &m_ViewY);

			ImVec2 canvasPos = ImGui::GetCursorScreenPos();
			ImVec2 canvasSize = ImGui::GetContentRegionAvail();
			canvasSize.y = std::max(canvasSize.y, 260.f);
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(24, 26, 30, 255));
			drawList->AddRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(90, 94, 102, 255));

			if (!chunks.empty())
			{
				int64_t minX = chunks.begin()->first.x;
				int64_t maxX = chunks.begin()->first.x;
				int64_t minZ = chunks.begin()->first.z;
				int64_t maxZ = chunks.begin()->first.z;
				bool hasVisibleChunk = false;
				for (const auto& [coord, _] : chunks)
				{
					if (m_bFilterY && coord.y != m_ViewY)
					{
						continue;
					}
					if (!hasVisibleChunk)
					{
						minX = maxX = coord.x;
						minZ = maxZ = coord.z;
						hasVisibleChunk = true;
					}
					minX = std::min(minX, coord.x);
					maxX = std::max(maxX, coord.x);
					minZ = std::min(minZ, coord.z);
					maxZ = std::max(maxZ, coord.z);
				}

				if (hasVisibleChunk)
				{
					const float padding = 16.f;
					const float cols = static_cast<float>(maxX - minX + 1);
					const float rows = static_cast<float>(maxZ - minZ + 1);
					const float cell = std::max(6.f, std::min((canvasSize.x - padding * 2.f) / cols, (canvasSize.y - padding * 2.f) / rows));
					const float originX = canvasPos.x + (canvasSize.x - cols * cell) * 0.5f;
					const float originY = canvasPos.y + (canvasSize.y - rows * cell) * 0.5f;

					for (const auto& [coord, chunk] : chunks)
					{
						if (m_bFilterY && coord.y != m_ViewY)
						{
							continue;
						}
						const float x = originX + static_cast<float>(coord.x - minX) * cell;
						const float y = originY + static_cast<float>(maxZ - coord.z) * cell;
				const bool selected = m_bHasSelection && coord == m_SelectedCoord;
				ImU32 color = ChunkColor(chunk, selected);
				if (chunk.GetLoadState() != E::EChunkLoadState::Loaded)
				{
					color = selected ? IM_COL32(255, 215, 96, 150) : IM_COL32(90, 95, 105, 110);
				}
				drawList->AddRectFilled(ImVec2(x + 1.f, y + 1.f), ImVec2(x + cell - 1.f, y + cell - 1.f), color);
						drawList->AddRect(ImVec2(x, y), ImVec2(x + cell, y + cell), IM_COL32(210, 215, 220, selected ? 255 : 90));

						if (cell >= 26.f)
						{
							std::string count = std::to_string(chunk.GetObjectHandles().size());
							ImVec2 textSize = ImGui::CalcTextSize(count.c_str());
							drawList->AddText(ImVec2(x + (cell - textSize.x) * 0.5f, y + (cell - textSize.y) * 0.5f), IM_COL32(255, 255, 255, 230), count.c_str());
						}
					}
				}
				else
				{
					const char* text = "No chunks on this Y layer.";
					ImVec2 textSize = ImGui::CalcTextSize(text);
					drawList->AddText(ImVec2(canvasPos.x + (canvasSize.x - textSize.x) * 0.5f, canvasPos.y + (canvasSize.y - textSize.y) * 0.5f), IM_COL32(180, 185, 195, 255), text);
				}
			}
			else
			{
				const char* text = "No chunks. Press Rebuild Chunks.";
				ImVec2 textSize = ImGui::CalcTextSize(text);
				drawList->AddText(ImVec2(canvasPos.x + (canvasSize.x - textSize.x) * 0.5f, canvasPos.y + (canvasSize.y - textSize.y) * 0.5f), IM_COL32(180, 185, 195, 255), text);
			}

			ImGui::Dummy(canvasSize);
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}

E::UPtr<CMapChunkGUI> CMapChunkGUI::Create(E::CHandle* pSelectedObject)
{
	auto pInstance = E::UPtr<CMapChunkGUI>(new CMapChunkGUI{});
	if (FAILED(pInstance->Initialize(pSelectedObject)))
	{
		MSG_BOX("Failed to Created : CMapChunkGUI");
		return nullptr;
	}

	return pInstance;
}
