#pragma once
#include "GUIWindow.h"


namespace Engine
{
	class CResStaticModel;
}

NS_BEGIN(Client)

class CModelThumbnailCache;
class CEditorCommandManager;

struct ResourceViewItem
{
	const char* category{};
	const char* icon{};
	ImVec4 color{};
	std::string groupName{};
	std::string resourceTag{};
	std::string resourceName{};
	std::string path{};
	std::string state{};
	bool bCanCreateMapMeshObject = false;
	E::SPtr<E::CResStaticModel> staticModel{};
};

class CResourceGUI : public CGUIWindow
{
public:
	DECLARE_DERIVED_TYPE(CResourceGUI, CGUIWindow)

private:
	CResourceGUI();
	~CResourceGUI() override;

public:
	HRESULT CachingAllResource();
	void UpdateGUI(E::_float fTimeDelta) override;

public:
	static E::UPtr<CResourceGUI> Create(E::CHandle* pSelectedObject,
		CEditorCommandManager* pCommandManager);

private:
	void HandleModelDropToScene();
	void CreateDroppedMapMeshObject(const E::_float3& worldPosition);
	void SelectAndImportWholeMapManifest();
	void SelectAndImportObjectMapManifest();
	void SelectAndLoadStaticModelFolder();
	_bool ImportWholeMapManifest(const std::filesystem::path& manifestPath);
	_bool ImportObjectMapManifest(const std::filesystem::path& manifestPath);

private:
	char m_SearchBuffer[128]{};
	int m_SelectedCategory{};
	E::UPtr<CModelThumbnailCache> m_pThumbnailCache{};
	std::string m_DragModelGroup{};
	std::string m_DragModelTag{};
	bool m_bDraggingModel = false;
	float m_fSceneDropDistance = 5.f;
	CEditorCommandManager* m_pCommandManager = nullptr;
	std::string m_WholeMapImportStatus{};
	std::string m_ModelFolderLoadStatus{};
	float m_fWholeMapScale = 0.3f;
	E::_float3 m_vWholeMapOrigin{};
	E::_float3 m_vWholeMapRotationDegrees{};
private:
	std::vector<ResourceViewItem> m_Items{};
	std::vector<size_t> m_FilteredItemIndices{};

	// 광윤 추가
	float m_fDropInitialScale = 1.f;
};

NS_END
