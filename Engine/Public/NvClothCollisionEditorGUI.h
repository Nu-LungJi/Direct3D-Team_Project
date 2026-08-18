#pragma once

#include "Engine_Base.h"
#include "NvClothCollisionRigData.h"

#include <filesystem>

NS_BEGIN(Engine)

class CResModel;
class CNvClothCollisionPreviewRenderer;

class CNvClothCollisionEditorGUI final :
	public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(
		CNvClothCollisionEditorGUI,
		CEngineBase)

private:
	enum class SELECTION_TYPE : uint8_t
	{
		NONE,
		BONE,
		SHAPE
	};

private:
	CNvClothCollisionEditorGUI() = default;
	~CNvClothCollisionEditorGUI() override;

public:
	void Open()
	{
		m_bOpen = true;
	}

	void UpdateGUI();

	static UPtr<CNvClothCollisionEditorGUI> Create();

private:
	void DrawWindow();
	void DrawModelSelector();
	void DrawPreviewControls();
	void DrawHierarchy();
	void DrawInspector();
	void DrawFilePopups();
	void DrawPreview();
	void RenderGizmo();

	void SelectModel(
		const SPtr<CResModel>& pModel,
		const StringID& sGroupTag,
		const StringID& sResourceTag,
		std::string sLabel);
	_matrix MakePreviewWorld() const;
	void PlacePreviewAtCamera();
	void AddShape(NVCLOTH_COLLISION_SHAPE_TYPE eType);
	void RemoveSelectedShape();
	void Clear();
	HRESULT Save() const;
	HRESULT Load();
	_bool Validate(std::vector<std::string>& Errors) const;
	std::filesystem::path MakeFilePath() const;
	void QueueResultPopup(
		std::string sMessage,
		_bool bSuccess);

	NVCLOTH_COLLISION_SHAPE_DESC* GetSelectedShape();
	_bool GetBoneRigidWorld(
		std::string_view sBoneName,
		_float4x4& OutWorld) const;

private:
	NVCLOTH_COLLISION_RIG_DESC m_Rig{};
	SPtr<CResModel> m_pSelectedModel{};
	UPtr<CNvClothCollisionPreviewRenderer>
		m_pPreviewRenderer{};
	std::vector<_float4x4> m_BindPoses{};
	std::string m_sSelectedModelLabel{
		"<select model>"
	};

	SELECTION_TYPE m_eSelection{
		SELECTION_TYPE::NONE
	};
	int32_t m_iSelectedBone{ -1 };
	int32_t m_iSelectedShape{ -1 };
	uint64_t m_iNextShapeID{ 1 };

	_bool m_bOpen{};
	_bool m_bPreviewVisible{ true };
	_bool m_bPreviewModel{ true };
	_bool m_bPreviewSkeleton{ true };
	_bool m_bPreviewShapes{ true };
	_bool m_bPreviewDepthTest{};
	_bool m_bEditFileName{};
	_bool m_bDirty{};
	_bool m_bOpenResultPopup{};
	_bool m_bResultPopupSuccess{};
	std::string m_sStatus{
		"Select a skeleton model resource."
	};
	std::string m_sResultPopupMessage{};
	std::vector<std::string> m_ValidationErrors{};
	_float3 m_vPreviewPosition{};
	_float m_fPreviewScale{ 1.f };
	ImGuizmo::OPERATION m_eGizmoOperation{
		ImGuizmo::TRANSLATE
	};
	char m_FileName[128]{
		"ProfessorCape"
	};
};

NS_END
