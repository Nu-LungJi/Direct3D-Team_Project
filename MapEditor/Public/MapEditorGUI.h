#pragma once
#include "GUIWindow.h"
#include "Hierarchy.h"
#include "Inspector.h"

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

private:
	E::UPtr<CHierarchy> m_pHierarchy{};
	E::UPtr<CInspector> m_pInspector{};
	ImGuizmo::OPERATION m_GizmoOperation{ ImGuizmo::TRANSLATE };
	ImGuizmo::MODE m_GizmoMode{ ImGuizmo::WORLD };
};

NS_END
