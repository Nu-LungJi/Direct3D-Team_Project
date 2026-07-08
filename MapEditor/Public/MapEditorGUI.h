#pragma once
#include "GUIWindow.h"
#include "Hierarchy.h"
#include "Inspector.h"
#include "ResourceGUI.h"
#include "MapChunkGUI.h"

NS_BEGIN(Client)

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
	//void AddDefaultCameraLight();

private:
	E::UPtr<CHierarchy> m_pHierarchy{};
	E::UPtr<CInspector> m_pInspector{};
	E::UPtr<CResourceGUI> m_pResourceGUI{};
	E::UPtr<CMapChunkGUI> m_pMapChunkGUI{};
	char m_MapName[64] = "LevelA";
	ImGuizmo::OPERATION m_GizmoOperation{ ImGuizmo::TRANSLATE };
	ImGuizmo::MODE m_GizmoMode{ ImGuizmo::WORLD };
};

NS_END
