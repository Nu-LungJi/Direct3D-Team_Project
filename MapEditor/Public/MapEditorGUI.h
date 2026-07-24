#pragma once
#include "GUIWindow.h"
#include "Hierarchy.h"
#include "Inspector.h"
#include "ResourceGUI.h"
#include "MapChunkGUI.h"
#include "NavMeshGUI.h"
#include "TerrainGUI.h"

NS_BEGIN(Client)

class CMapPickingPass;
class CEditorCommandManager;
class CEditorSelection;

class CMapEditorGUI : public CGUIWindow
{
public:
	DECLARE_DERIVED_TYPE(CMapEditorGUI, CGUIWindow)

private:
	CMapEditorGUI();
	~CMapEditorGUI() override;

public:
	void UpdateGUI(E::_float fTimeDelta) override;

public:
	static E::UPtr<CMapEditorGUI> Create(E::CHandle* pSelectedObject);

private:
	void DrawGizmoToolbar();
	void RenderGizmo();
	void PickMapMeshObject();
	void DrawMapMeshContextMenu();
	//void AddDefaultCameraLight();

private:
	std::unique_ptr<CEditorSelection> m_pSelection{};
	E::UPtr<CEditorCommandManager> m_pCommandManager{};
	E::UPtr<CHierarchy> m_pHierarchy{};
	E::UPtr<CInspector> m_pInspector{};
	E::UPtr<CResourceGUI> m_pResourceGUI{};
	E::UPtr<CMapChunkGUI> m_pMapChunkGUI{};
	E::UPtr<CNavMeshGUI> m_pNavMeshGUI{};
	E::UPtr<CTerrainGUI> m_pTerrainGUI{};
	E::UPtr<CMapPickingPass> m_pMapPickingPass{};
	char m_MapName[64] = "LevelA";
	ImGuizmo::OPERATION m_GizmoOperation{ ImGuizmo::TRANSLATE };
	ImGuizmo::MODE m_GizmoMode{ ImGuizmo::WORLD };
	bool m_bWasUsingGizmo = false;
	E::_float4x4 m_MultiGizmoStartMatrix{};
	E::_float4x4 m_MultiGizmoCurrentMatrix{};
	std::vector<std::pair<E::CHandle, E::_float4x4>> m_MultiGizmoStartTransforms{};
	bool m_bOpenSaveComplete = false;
	bool m_bOpenLoadComplete = false;
	std::optional<E::CHandle> m_ContextMapMeshHandle{};
};

NS_END
