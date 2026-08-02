#pragma once

#include "Engine_Base.h"
#include "PathPlaybackDefines.h"

#include <filesystem>
#include <string>
#include <vector>

NS_BEGIN(Engine)

class CDbgLineRender;

class CPathPlaybackEditor final : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CPathPlaybackEditor, CEngineBase)

private:
	CPathPlaybackEditor();
	~CPathPlaybackEditor() override = default;

public:
	void UpdateGUI();
	static UPtr<CPathPlaybackEditor> Create();

private:
	void DrawWindow();
	void DrawFileControls();
	void DrawClipList();
	void DrawClipInspector();
	void DrawKeyframeList();
	void DrawKeyframeInspector();
	void DrawPreviewControls();
	void DrawValidationErrors();
	void DrawResultPopup();

	void NewData();
	void AddClip();
	void DuplicateSelectedClip();
	void DeleteSelectedClip();
	void AddKeyframe();
	void DuplicateSelectedKeyframe();
	void DeleteSelectedKeyframe();

	HRESULT Save(_bool bBinary);
	HRESULT Load(_bool bBinary);
	std::filesystem::path MakeFilePath(_bool bBinary) const;
	_bool ValidateWorkingData();
	void QueueResultPopup(std::string Message, _bool bSuccess);

	void ResetPreview();
	void UpdatePreview(_float fTimeDelta);
	_bool EvaluatePreviewPose(_float fElapsedTime);
	void DrawDebugPath();
	void RenderGizmo();

	PATH_PLAYBACK_CLIP* GetSelectedClip();
	const PATH_PLAYBACK_CLIP* GetSelectedClip() const;
	PATH_PLAYBACK_KEYFRAME* GetSelectedKeyframe();
	const PATH_PLAYBACK_KEYFRAME* GetSelectedKeyframe() const;

	_matrix MakeAnchorWorld() const;
	_matrix MakeKeyframeWorld(
		const PATH_PLAYBACK_CLIP& Clip,
		const PATH_PLAYBACK_KEYFRAME& Keyframe) const;
	void ApplyKeyframeWorld(
		const PATH_PLAYBACK_CLIP& Clip,
		PATH_PLAYBACK_KEYFRAME& Keyframe,
		FXMMATRIX World);

	static _float3 QuaternionToEulerDegrees(const _float4& Rotation);
	static _float4 EulerDegreesToQuaternion(const _float3& EulerDegrees);
	static std::string MakeUniqueClipName(
		const PATH_PLAYBACK_DATA& Data,
		std::string BaseName);

private:
	PATH_PLAYBACK_DATA m_WorkingData{};
	int32_t m_iSelectedClip{ -1 };
	int32_t m_iSelectedKeyframe{ -1 };

	PATH_PLAYBACK_POSE m_tPreviewAnchor{};
	PATH_PLAYBACK_POSE m_tPreviewPose{};
	_float m_fPreviewTime{};
	_float m_fPreviewRate{ 1.f };
	_bool m_bPreviewPlaying{};
	_bool m_bPreviewForward{ true };
	_bool m_bPreviewPoseValid{};

	ImGuizmo::OPERATION m_eGizmoOperation{ ImGuizmo::TRANSLATE };
	ImGuizmo::MODE m_eGizmoMode{ ImGuizmo::WORLD };
	_bool m_bEditAnchor{};
	_bool m_bEditMode{ true };
	_bool m_bShowPath{ true };
	_bool m_bDepthTest{};
	_bool m_bSnapEnabled{};
	_float m_fTranslationSnap{ 0.25f };
	_float m_fRotationSnap{ 15.f };
	int32_t m_iSamplesPerSegment{ 8 };

	_bool m_bWindowVisible{};
	_bool m_bDirty{};
	_bool m_bManualFileNameInput{};
	_bool m_bOpenResultPopup{};
	_bool m_bResultPopupSuccess{};
	std::string m_sStatus{ "Create or load PathPlayback data." };
	std::string m_sResultPopupMessage{};
	std::vector<std::string> m_ValidationErrors{};
	char m_FileName[128]{ "NewPath" };
};

NS_END
