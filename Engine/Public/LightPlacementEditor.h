#pragma once

#include "Engine_Defines.h"
#include "Engine_Base.h"
#include "Handle.h"
#include "LightPlacementData.h"

NS_BEGIN(Engine)

class CLight;
class CLightManager;
class CDbgLineRender;

class CLightPlacementEditor final : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CLightPlacementEditor, CEngineBase)

private:
	explicit CLightPlacementEditor(CLightManager* pLightManager);
	~CLightPlacementEditor() override = default;

public:
	void UpdateGUI();
	void SetActivePlacementGroup(std::string_view sGroup);
	HRESULT Save() const;
	HRESULT Load();
	void Clear();

public:
	static UPtr<CLightPlacementEditor> Create(
		CLightManager* pLightManager);

private:
	void DrawWindow();
	void DrawLightList();
	void DrawSelectedLightInspector();
	void DrawDebugLights();
	void DrawDebugLight(
		CDbgLineRender& debug,
		CLight& light,
		_bool bSelected) const;
	void RenderGizmo();
	std::optional<CHandle> CreateLightAtCamera(LIGHT_TYPE eType);
	std::optional<CHandle> CreateLight(
		const LIGHT_PLACEMENT_ENTRY& data);
	LIGHT_PLACEMENT_FILE BuildFileData() const;
	std::string GetActivePlacementGroup() const;
	CLight* GetSelectedLight() const;
	void DeleteSelected();
	void QueueResultPopup(std::string message, _bool bSuccess);

private:
	CLightManager* m_pLightManager{};
	std::optional<CHandle> m_SelectedLight{};
	ImGuizmo::OPERATION m_eGizmoOperation{
		ImGuizmo::TRANSLATE
	};
	ImGuizmo::MODE m_eGizmoMode{ ImGuizmo::WORLD };
	_bool m_bEditMode{};
	_bool m_bVisible{ true };
	_bool m_bDepthTest{ true };
	_bool m_bShowAllLights{};
	_bool m_bShowInfluenceRange{ true };
	_bool m_bShowDirection{ true };
	_bool m_bShowEffectLights{ };
	_bool m_bEffectLightDepthTest{ true };
	_bool m_bSnapEnabled{};
	_bool m_bManualFileNameInput{};
	int32_t m_iLightFilePreset{};
	_float m_fTranslationSnap{ 0.5f };
	_float m_fRotationSnap{ 15.f };
	_float m_fSpawnDistance{ 5.f };
	std::string m_Status{};
	std::string m_ResultPopupMessage{};
	_bool m_bOpenResultPopup{};
	_bool m_bResultPopupSuccess{};
	char m_LightFileName[128]{ "Level_Lights" };
};

NS_END
