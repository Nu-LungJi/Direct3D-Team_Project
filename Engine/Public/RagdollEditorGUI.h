#pragma once
#include "Engine_Base.h"
#include "PxRagdollAuthoring.h"

NS_BEGIN(Engine)

class CResModel;
class CRagdollPreviewRenderer;
class CRagdollPhysicsPreviewOwner;

class CRagdollEditorGUI final : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CRagdollEditorGUI, CEngineBase)

private:
	enum class SELECTION_TYPE : uint8_t
	{
		NONE,
		BONE,
		BODY,
		SHAPE,
		JOINT
	};

private:
	CRagdollEditorGUI() = default;
	~CRagdollEditorGUI() override;

public:
	void Open()
	{
		m_bOpen = true;
	}

	void UpdateGUI();
	void SetCollisionLayerNames(
		std::vector<std::pair<uint32_t, std::string>>
			LayerNames);

public:
	static UPtr<CRagdollEditorGUI> Create();

private:
	void DrawWindow();
	void DrawModelSelector();
	void DrawHierarchyPanel();
	void DrawSkeletonHierarchy();
	void DrawBodyHierarchy();
	void DrawJointHierarchy();
	void DrawInspectorPanel();
	void DrawBodyInspector();
	void DrawShapeInspector();
	void DrawJointInspector();
	void DrawJointCreationPanel();
	void DrawLayerSelector(
		const char* pLabel,
		uint32_t& iLayer) const;
	void DrawLayerMaskSelector(
		const char* pLabel,
		uint32_t& iMask) const;
	void DrawFilePopups();
	void DrawPreviewControls();
	void DrawPreview();
	void RenderGizmo();
	_bool StartPhysicsPreview();
	void StopPhysicsPreview();
	void UpdatePhysicsPreviewPose();

	void SelectModel(const SPtr<CResModel>& pModel);
	void PlacePreviewAtCamera();
	void AddBodyForSelectedBone();
	void RemoveSelectedBody();
	void AddShapeToSelectedBody();
	void RemoveSelectedShape();
	void AddBindPoseJoint();
	void RemoveSelectedJoint();
	_bool GenerateHumanoidPreset();
	_bool Validate(std::vector<std::string>& Errors) const;
	HRESULT Save() const;
	HRESULT Load();
	void Clear();
	std::filesystem::path MakeFilePath() const;
	void QueueResultPopup(
		std::string sMessage,
		_bool bSuccess);

	PX_RAGDOLL_BODY_DESC* GetSelectedBody();
	PX_RAGDOLL_SHAPE_DESC* GetSelectedShape();
	PX_RAGDOLL_D6_JOINT_DESC* GetSelectedJoint();
	_bool BuildBodyPreviewWorldMatrix(
		size_t iBodyIndex,
		_float4x4& OutWorldMatrix) const;

private:
	PX_RAGDOLL_DESC m_Ragdoll{};
	CPxRagdollAuthoring m_Authoring{};
	SPtr<CResModel> m_pSelectedModel{};
	UPtr<CRagdollPreviewRenderer>
		m_pPreviewRenderer{};
	UPtr<CRagdollPhysicsPreviewOwner>
		m_pPhysicsPreviewOwner{};
	std::vector<_float4x4>
		m_PreviewBindPoses{};
	std::string m_sSelectedModelLabel{
		"<select model>"
	};
	std::vector<std::pair<uint32_t, std::string>>
		m_CollisionLayerNames{};
	std::vector<std::string> m_ValidationErrors{};

	SELECTION_TYPE m_eSelection{
		SELECTION_TYPE::NONE
	};
	int32_t m_iSelectedBone{ -1 };
	int32_t m_iSelectedBody{ -1 };
	int32_t m_iSelectedShape{ -1 };
	int32_t m_iSelectedJoint{ -1 };
	int32_t m_iNewJointParentBody{};
	int32_t m_iNewJointChildBody{ 1 };

	_bool m_bOpen{};
	_bool m_bPreviewVisible{ true };
	_bool m_bPreviewModel{ true };
	_bool m_bPreviewSkeleton{ true };
	_bool m_bPreviewBodies{ true };
	_bool m_bPreviewJoints{ true };
	_bool m_bPreviewDepthTest{};
	_bool m_bPhysicsPreviewActive{};
	_bool m_bOpenResultPopup{};
	_bool m_bResultPopupSuccess{};
	_bool m_bEditFileName{};
	_bool m_bDirty{};
	std::string m_sStatus{
		"Select a model resource."
	};
	std::string m_sResultPopupMessage{};
	_float3 m_vPreviewPosition{};
	_float m_fPreviewJointAxisLength{ 0.2f };
	_float3 m_vPhysicsTestLinearVelocity{};
	_float3 m_vPhysicsTestAngularVelocityDegrees{};
	ImGuizmo::OPERATION m_eGizmoOperation{
		ImGuizmo::TRANSLATE
	};
	char m_RagdollFileName[128]{
		"NewRagdoll"
	};
};

NS_END
