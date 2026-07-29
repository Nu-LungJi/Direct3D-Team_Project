#pragma once
#include "CinematicTypes.h"
#include "Handle.h"

NS_BEGIN(Engine)
class CCinematicAsset;
NS_END

NS_BEGIN(Client)

class CCinematicEditor final : public E::CEngineBase
{
private:
	CCinematicEditor();
	~CCinematicEditor() override;

public:
	void UpdateGUI();

private:
	void CreateAsset();
	void SaveAsset();
	void LoadAsset();
	void AddShot();
	void DrawPreviewTargetSelector();
	void DrawShot(E::FCinematicCameraShot& Shot, size_t iShotIndex, _bool& bChanged, _bool& bStartTimeEditFinished);
	void DrawKeyframe(E::FCinematicCameraKeyframe& Keyframe,size_t iShotIndex,size_t iKeyframeIndex,_bool& bChanged,_bool& bRemove,_bool& bTimeEditFinished);
	void DrawViewportEditor();
	void DrawWorldVisualization() const;
	void HandleViewportSelection();
	void DrawSelectedKeyframeGizmo();

	E::FCinematicCameraKeyframe* GetSelectedKeyframe();
	const E::FCinematicCameraKeyframe* GetSelectedKeyframe() const;
	_bool HasTargetLocalShot() const;
	_bool TryGetPreviewTargetWorld(_matrix& OutTargetWorld) const;
	_bool TryGetKeyframeWorld(
		const E::FCinematicCameraShot& Shot,
		const E::FCinematicCameraKeyframe& Keyframe,
		_matrix& OutKeyframeWorld) const;

private:
	E::SPtr<E::CCinematicAsset> m_pEditingAsset{};
	_char m_szCinematicID[128]{ "TestCinematic" };
	std::string m_Status{ "Create a cinematic asset to begin." };
	std::optional<size_t> m_SelectedShotIndex{};
	std::optional<size_t> m_SelectedKeyframeIndex{};
	std::optional<E::CHandle> m_PreviewTargetHandle{};
	ImGuizmo::OPERATION m_GizmoOperation{ ImGuizmo::TRANSLATE };
	_float m_fFrustumDisplaySize{ 1.25f };

public:
	static E::UPtr<CCinematicEditor> Create();
};

NS_END
