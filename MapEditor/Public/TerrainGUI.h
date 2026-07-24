#pragma once

#include "GUIWindow.h"
#include "TerrainEditCommand.h"
#include "MapMeshCommandCommon.h"

NS_BEGIN(Client)

class CTerrainPickingPass;
class CTerrainBrushController;
class CEditorCommandManager;

class CTerrainGUI final : public CGUIWindow
{
public:
	DECLARE_DERIVED_TYPE(CTerrainGUI, CGUIWindow)

private:
	CTerrainGUI() = default;
	~CTerrainGUI() override = default;

public:
	void UpdateGUI(E::_float fTimeDelta) override;
	bool IsSculptEnabled() const { return m_bSculptEnabled || m_bTexturePaintEnabled || m_bScatterEnabled; }
	static E::UPtr<CTerrainGUI> Create(E::CHandle* selectedObject, CEditorCommandManager* commandManager);

private:
	E::UPtr<CTerrainPickingPass> m_pPickingPass{};
	E::UPtr<CTerrainBrushController> m_pBrushController{};
	std::optional<E::_float3> m_PickedPosition{};
	bool m_bPickingDebug = false;
	bool m_bSculptEnabled = false;
	bool m_bTexturePaintEnabled = false;
	char m_TerrainDataPath[512] = "./Resources/json/MapSaved/LevelName/Terrain/terrain.json";
	std::string m_TerrainIOStatus{};
	CEditorCommandManager* m_pCommandManager = nullptr;
	std::unique_ptr<CTerrainEditCommand> m_pActiveEditCommand{};
	bool m_bScatterEnabled = false;
	int m_iScatterCount = 5;
	float m_fScatterSpacing = 3.f;
	float m_fScatterScaleMin = 0.8f;
	float m_fScatterScaleMax = 1.2f;
	bool m_bScatterRandomYaw = true;
	std::string m_ScatterModelGroup{};
	std::string m_ScatterModelTag{};
	std::optional<E::_float3> m_PreviousScatterHit{};
	std::vector<MAPMESH_OBJECT_SNAPSHOT> m_ScatterSnapshots{};
	std::vector<E::CHandle> m_ScatterHandles{};
};

NS_END
