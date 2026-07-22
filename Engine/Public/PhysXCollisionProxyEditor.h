#pragma once
#include "PhysXCollisionProxyData.h"
#include "Engine_Base.h"

NS_BEGIN(Engine)

class CPhysXCollisionProxyEditor final : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CPhysXCollisionProxyEditor, CEngineBase)

private:
	struct SNAPSHOT
	{
		std::vector<PX_COLLISION_PROXY_BOX> boxes{};
		std::optional<uint64_t> selectedID{};
	};

private:
	CPhysXCollisionProxyEditor() = default;
	~CPhysXCollisionProxyEditor() override = default;

public:
	void UpdateGUI(E::_float fTimeDelta);
	HRESULT Save() const;
	HRESULT Load();
	void Clear();

public:
	static UPtr<CPhysXCollisionProxyEditor> Create();

private:
	void DrawWindow();
	void DrawDebugBoxes();
	void RenderGizmo();
	void HandleSceneInput();
	void CreateBox(const E::_float3& position);
	void DuplicateSelected();
	void DeleteSelected();
	void SelectAtMouse();
	_bool MakeMouseRay(E::_float3& outOrigin, E::_float3& outDirection) const;
	_bool IntersectPlacementPlane(E::_float3& outPosition) const;
	PX_COLLISION_PROXY_BOX* GetSelectedBox();
	const PX_COLLISION_PROXY_BOX* GetSelectedBox() const;
	void PushUndo();
	void Undo();
	void Redo();
	void RestoreSnapshot(SNAPSHOT snapshot);

private:
	std::vector<PX_COLLISION_PROXY_BOX> m_Boxes{};
	std::optional<uint64_t> m_SelectedID{};
	uint64_t m_iNextID{ 1 };
	_bool m_bEditMode{};
	_bool m_bPlaceMode{};
	_bool m_bVisible{ true };
	_bool m_bDepthTest{ true };
	_bool m_bWasUsingGizmo{};
	E::_float m_fPlacementY{};
	E::_float3 m_vDefaultSize{ 2.f, 0.2f, 2.f };
	ImGuizmo::OPERATION m_GizmoOperation{ ImGuizmo::TRANSLATE };
	ImGuizmo::MODE m_GizmoMode{ ImGuizmo::WORLD };
	std::vector<SNAPSHOT> m_UndoStack{};
	std::vector<SNAPSHOT> m_RedoStack{};
	std::optional<SNAPSHOT> m_GizmoStartSnapshot{};
	std::string m_Status{};
	char m_CollisionFileName[128]{ "LevelA" };
};

NS_END
