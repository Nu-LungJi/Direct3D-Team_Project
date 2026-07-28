#pragma once
#include "CinematicTypes.h"

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
	void AddShot();
	void DrawShot(E::FCinematicCameraShot& Shot, size_t iShotIndex, _bool& bChanged);
	void DrawKeyframe(E::FCinematicCameraKeyframe& Keyframe,size_t iShotIndex,size_t iKeyframeIndex,_bool& bChanged,_bool& bRemove);
	void DrawViewportEditor();
	void DrawWorldVisualization() const;
	void HandleViewportSelection();
	void DrawSelectedKeyframeGizmo();

	E::FCinematicCameraKeyframe* GetSelectedKeyframe();
	const E::FCinematicCameraKeyframe* GetSelectedKeyframe() const;

private:
	E::SPtr<E::CCinematicAsset> m_pEditingAsset{};
	_char m_szCinematicID[128]{ "TestCinematic" };
	std::string m_Status{ "Create a cinematic asset to begin." };
	std::optional<size_t> m_SelectedShotIndex{};
	std::optional<size_t> m_SelectedKeyframeIndex{};
	ImGuizmo::OPERATION m_GizmoOperation{ ImGuizmo::TRANSLATE };
	_float m_fFrustumDisplaySize{ 1.25f };

public:
	static E::UPtr<CCinematicEditor> Create();
};

NS_END
