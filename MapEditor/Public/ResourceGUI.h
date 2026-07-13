#pragma once
#include "GUIWindow.h"

NS_BEGIN(Client)

class CModelThumbnailCache;
class CEditorCommandManager;

class CResourceGUI : public CGUIWindow
{
public:
	DECLARE_DERIVED_TYPE(CResourceGUI, CGUIWindow)

private:
	CResourceGUI();
	~CResourceGUI() override;

public:
	void UpdateGUI(E::_float fTimeDelta) override;

public:
	static E::UPtr<CResourceGUI> Create(E::CHandle* pSelectedObject,
		CEditorCommandManager* pCommandManager);

private:
	void HandleModelDropToScene();
	void CreateDroppedMapMeshObject(const E::_float3& worldPosition);

private:
	char m_SearchBuffer[128]{};
	int m_SelectedCategory{};
	E::UPtr<CModelThumbnailCache> m_pThumbnailCache{};
	std::string m_DragModelGroup{};
	std::string m_DragModelTag{};
	bool m_bDraggingModel = false;
	float m_fSceneDropDistance = 20.f;
	CEditorCommandManager* m_pCommandManager = nullptr;
};

NS_END
