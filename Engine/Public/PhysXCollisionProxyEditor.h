#pragma once
#include "PhysXCollisionProxyData.h"
#include "Engine_Base.h"
#include "Handle.h"

NS_BEGIN(Engine)

class CPhysXCollisionProxyEditor final : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CPhysXCollisionProxyEditor, CEngineBase)

private:
	struct SNAPSHOT
	{
		std::vector<PX_COLLISION_PROXY_ACTOR> actors{};
		std::optional<uint64_t> selectedActorID{};
		std::optional<uint64_t> selectedShapeID{};
	};

private:
	CPhysXCollisionProxyEditor() = default;
	~CPhysXCollisionProxyEditor() override = default;

public:
	void UpdateGUI(_float fTimeDelta);
	void SetCollisionLayerNames(std::vector<std::pair<uint32_t, std::string>> layerNames);
	HRESULT Save() const;
	HRESULT Load();
	void Clear();

public:
	static UPtr<CPhysXCollisionProxyEditor> Create();

private:
	void DrawWindow();
	void DrawHierarchy();
	void DrawInspector();
	void DrawLayerSelector(const char* label, uint32_t& layer) const;
	void DrawLayerMaskSelector(const char* label, uint32_t& mask) const;
	void DrawDebugShapes();
	void RenderGizmo();
	void HandleSceneInput();
	void CreateActor(const _float3& position = {});
	void CreateShapeAtCamera(PX_COLLISION_PROXY_SHAPE_TYPE eType);
	void CreateCylinderAtCamera();
	void CreateWedgeAtCamera();
	void DuplicateSelected();
	void DeleteSelected();
	void SelectAtMouse();
	_bool MakeMouseRay(_float3& outOrigin, _float3& outDirection) const;
	PX_COLLISION_PROXY_ACTOR* GetSelectedActor();
	const PX_COLLISION_PROXY_ACTOR* GetSelectedActor() const;
	PX_COLLISION_PROXY_SHAPE* GetSelectedShape();
	const PX_COLLISION_PROXY_SHAPE* GetSelectedShape() const;
	void PushUndo();
	void Undo();
	void Redo();
	void RestoreSnapshot(SNAPSHOT snapshot);
	void RecalculateNextID();
	void CreatePhysicsPreview();
	void RemovePhysicsPreview();
	void QueueResultPopup(std::string message, _bool success);

private:
	std::vector<PX_COLLISION_PROXY_ACTOR> m_Actors{};
	std::optional<uint64_t> m_SelectedActorID{};
	std::optional<uint64_t> m_SelectedShapeID{};
	uint64_t m_iNextID{ 1 };
	_bool m_bEditMode{};
	_bool m_bVisible{ true };
	_bool m_bDepthTest{ true };
	_bool m_bEditCollisionFileName{};
	_bool m_bWasUsingGizmo{};
	PX_COLLISION_PROXY_SHAPE_TYPE m_eCreateShapeType{ PX_COLLISION_PROXY_SHAPE_TYPE::BOX };
	ImGuizmo::OPERATION m_GizmoOperation{ ImGuizmo::TRANSLATE };
	ImGuizmo::MODE m_GizmoMode{ ImGuizmo::WORLD };
	std::vector<SNAPSHOT> m_UndoStack{};
	std::vector<SNAPSHOT> m_RedoStack{};
	std::vector<std::pair<uint32_t, std::string>> m_CollisionLayerNames{};
	std::optional<SNAPSHOT> m_GizmoStartSnapshot{};
	std::vector<CHandle> m_PhysicsPreviewHandles{};
	std::string m_Status{};
	std::string m_ResultPopupMessage{};
	_bool m_bOpenResultPopup{};
	_bool m_bResultPopupSuccess{};
	char m_CollisionFileName[128]{ "LevelA" };
};

NS_END
