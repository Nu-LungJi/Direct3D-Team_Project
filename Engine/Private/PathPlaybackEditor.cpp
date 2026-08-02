#include "pch.h"
#include "PathPlaybackEditor.h"

#include "CameraObject.h"
#include "DbgLineRender.h"
#include "GameInstance.h"
#include "PathPlaybackEvaluator.h"
#include "ResPathPlayback.h"

NS_USING(Engine)

static constexpr const char* PATH_PLAYBACK_EDITOR_ROOT =
	"./Resources/json/PathPlayback";
static constexpr const char* PATH_PLAYBACK_EDITOR_SERIALIZE_ROOT =
	"PathPlayback";
static constexpr int PATH_PLAYBACK_EDITOR_GIZMO_ID = 0x50544845;
static constexpr _float PATH_PLAYBACK_EDITOR_TIME_EPSILON = 0.0001f;

template <typename TEnum>
static _bool PathPlaybackEditorDrawEnumCombo(
	const char* pLabel,
	TEnum& Value)
{
	const auto Values = magic_enum::enum_values<TEnum>();
	const std::string_view Preview = magic_enum::enum_name(Value);
	_bool bChanged = false;
	if (ImGui::BeginCombo(pLabel, Preview.empty() ? "Invalid" : Preview.data()))
	{
		for (const TEnum Candidate : Values)
		{
			const _bool bSelected = Candidate == Value;
			const std::string_view Name = magic_enum::enum_name(Candidate);
			if (ImGui::Selectable(Name.data(), bSelected))
			{
				Value = Candidate;
				bChanged = true;
			}
			if (bSelected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	return bChanged;
}

static void PathPlaybackEditorCopyText(
	char* pDestination,
	size_t iCapacity,
	std::string_view Source)
{
	if (!pDestination || iCapacity == 0)
		return;
	const size_t iCopyLength = std::min(iCapacity - 1, Source.size());
	std::memcpy(pDestination, Source.data(), iCopyLength);
	pDestination[iCopyLength] = '\0';
}

static void PathPlaybackEditorBeginDisabled(_bool bDisabled)
{
	ImGui::PushItemFlag(ImGuiItemFlags_Disabled, bDisabled);
	ImGui::PushStyleVar(
		ImGuiStyleVar_Alpha,
		bDisabled ? ImGui::GetStyle().Alpha * 0.5f :
		ImGui::GetStyle().Alpha);
}

static void PathPlaybackEditorEndDisabled()
{
	ImGui::PopStyleVar();
	ImGui::PopItemFlag();
}

CPathPlaybackEditor::CPathPlaybackEditor()
{
	NewData();
}

void CPathPlaybackEditor::UpdateGUI()
{
	DrawWindow();
	if (!m_bWindowVisible)
		return;

	UpdatePreview(ImGui::GetIO().DeltaTime);
	if (m_bShowPath)
		DrawDebugPath();
	if (m_bEditMode)
		RenderGizmo();
}

void CPathPlaybackEditor::DrawWindow()
{
	ImGui::SetNextWindowSize(ImVec2(920.f, 720.f), ImGuiCond_FirstUseEver);
	m_bWindowVisible = ImGui::Begin("Path Playback Editor");
	if (!m_bWindowVisible)
	{
		ImGui::End();
		return;
	}

	DrawFileControls();
	DrawPreviewControls();
	ImGui::Separator();

	const _float fLeftWidth = 270.f;
	if (ImGui::BeginChild("PathPlaybackClipPanel", ImVec2(fLeftWidth, 0.f), true))
		DrawClipList();
	ImGui::EndChild();
	ImGui::SameLine();
	if (ImGui::BeginChild("PathPlaybackInspectorPanel", ImVec2(0.f, 0.f), true))
	{
		DrawClipInspector();
		ImGui::Separator();
		DrawKeyframeList();
		ImGui::Separator();
		DrawKeyframeInspector();
		DrawValidationErrors();
	}
	ImGui::EndChild();

	DrawResultPopup();
	ImGui::End();
}

void CPathPlaybackEditor::DrawFileControls()
{
	ImGui::Checkbox("Edit File Name", &m_bManualFileNameInput);
	ImGui::SameLine();
	PathPlaybackEditorBeginDisabled(!m_bManualFileNameInput);
	ImGui::SetNextItemWidth(220.f);
	ImGui::InputText("##PathPlaybackFileName", m_FileName, sizeof(m_FileName));
	PathPlaybackEditorEndDisabled();

	if (ImGui::Button("New"))
		NewData();
	ImGui::SameLine();
	if (ImGui::Button("Validate"))
	{
		const _bool bValid = ValidateWorkingData();
		QueueResultPopup(
			bValid ? "PathPlayback data is valid." :
			"Validation failed. Check the error list.",
			bValid);
	}
	ImGui::SameLine();
	if (ImGui::Button("Save JSON"))
		Save(false);
	ImGui::SameLine();
	if (ImGui::Button("Load JSON"))
		Load(false);
	ImGui::SameLine();
	if (ImGui::Button("Export BIN"))
		Save(true);
	ImGui::SameLine();
	if (ImGui::Button("Load BIN"))
		Load(true);

	ImGui::Text("Path: %s", MakeFilePath(false).generic_string().c_str());
	ImGui::SameLine();
	ImGui::TextColored(
		m_bDirty ? ImVec4(1.f, 0.7f, 0.2f, 1.f) :
		ImVec4(0.4f, 1.f, 0.5f, 1.f),
		m_bDirty ? "[Modified]" : "[Saved]");
	ImGui::TextUnformatted(m_sStatus.c_str());
}

void CPathPlaybackEditor::DrawClipList()
{
	ImGui::Text("Clips (%zu)", m_WorkingData.Clips.size());
	if (ImGui::Button("Add Clip"))
		AddClip();
	ImGui::SameLine();
	PathPlaybackEditorBeginDisabled(GetSelectedClip() == nullptr);
	if (ImGui::Button("Duplicate"))
		DuplicateSelectedClip();
	ImGui::SameLine();
	if (ImGui::Button("Delete"))
		DeleteSelectedClip();
	PathPlaybackEditorEndDisabled();

	for (size_t i = 0; i < m_WorkingData.Clips.size(); ++i)
	{
		const auto& Clip = m_WorkingData.Clips[i];
		ImGui::PushID(static_cast<int>(i));
		const _bool bSelected = m_iSelectedClip == static_cast<int32_t>(i);
		if (ImGui::Selectable(Clip.sClipID.GetDbgStr(), bSelected))
		{
			m_iSelectedClip = static_cast<int32_t>(i);
			m_iSelectedKeyframe = Clip.Keyframes.empty() ? -1 : 0;
			ResetPreview();
		}
		ImGui::PopID();
	}
}

void CPathPlaybackEditor::DrawClipInspector()
{
	auto* pClip = GetSelectedClip();
	if (!pClip)
	{
		ImGui::TextDisabled("Select a clip.");
		return;
	}

	ImGui::TextUnformatted("Clip");
	char ClipID[128]{};
	PathPlaybackEditorCopyText(
		ClipID, sizeof(ClipID), pClip->sClipID.GetDbgStr());
	if (ImGui::InputText("Clip ID", ClipID, sizeof(ClipID)))
	{
		pClip->sClipID = StringID{ ClipID };
		m_bDirty = true;
	}

	_bool bChanged = false;
	bChanged |= PathPlaybackEditorDrawEnumCombo(
		"Coordinate Space", pClip->eCoordinateSpace);
	bChanged |= PathPlaybackEditorDrawEnumCombo(
		"Rotation Mode", pClip->eRotationMode);
	bChanged |= PathPlaybackEditorDrawEnumCombo(
		"Play Mode", pClip->ePlayMode);
	bChanged |= PathPlaybackEditorDrawEnumCombo(
		"Finish Behavior", pClip->eFinishBehavior);
	if (bChanged)
	{
		m_bDirty = true;
		ResetPreview();
	}
}

void CPathPlaybackEditor::DrawKeyframeList()
{
	auto* pClip = GetSelectedClip();
	if (!pClip)
		return;

	ImGui::Text("Keyframes (%zu)", pClip->Keyframes.size());
	if (ImGui::Button("Add Key"))
		AddKeyframe();
	ImGui::SameLine();
	PathPlaybackEditorBeginDisabled(GetSelectedKeyframe() == nullptr);
	if (ImGui::Button("Duplicate Key"))
		DuplicateSelectedKeyframe();
	ImGui::SameLine();
	if (ImGui::Button("Delete Key"))
		DeleteSelectedKeyframe();
	PathPlaybackEditorEndDisabled();

	const _float fListHeight = std::min(
		180.f, 28.f + static_cast<_float>(pClip->Keyframes.size()) * 23.f);
	if (ImGui::BeginChild("PathPlaybackKeyList", ImVec2(0.f, fListHeight), true))
	{
		for (size_t i = 0; i < pClip->Keyframes.size(); ++i)
		{
			const auto& Key = pClip->Keyframes[i];
			char Label[96]{};
			std::snprintf(
				Label, sizeof(Label), "[%zu]  %.3f sec%s%s",
				i, Key.fTime,
				Key.sEventTag.hash != 0 ? "  |  " : "",
				Key.sEventTag.hash != 0 ? Key.sEventTag.GetDbgStr() : "");
			if (ImGui::Selectable(
				Label, m_iSelectedKeyframe == static_cast<int32_t>(i)))
			{
				m_iSelectedKeyframe = static_cast<int32_t>(i);
				m_fPreviewTime = std::max(
					0.f, Key.fTime - pClip->Keyframes.front().fTime);
				EvaluatePreviewPose(m_fPreviewTime);
			}
		}
	}
	ImGui::EndChild();
}

void CPathPlaybackEditor::DrawKeyframeInspector()
{
	auto* pClip = GetSelectedClip();
	auto* pKey = GetSelectedKeyframe();
	if (!pClip || !pKey)
	{
		ImGui::TextDisabled("Select a keyframe.");
		return;
	}

	ImGui::Text("Keyframe %d", m_iSelectedKeyframe);
	_float fTime = pKey->fTime;
	if (ImGui::DragFloat("Time", &fTime, 0.01f, 0.f, FLT_MAX, "%.3f"))
	{
		pKey->fTime = std::max(0.f, fTime);
		const _float fSelectedTime = pKey->fTime;
		pClip->SortKeyframes();
		for (size_t i = 0; i < pClip->Keyframes.size(); ++i)
		{
			if (std::abs(pClip->Keyframes[i].fTime - fSelectedTime) <=
				PATH_PLAYBACK_EDITOR_TIME_EPSILON)
			{
				m_iSelectedKeyframe = static_cast<int32_t>(i);
				break;
			}
		}
		m_bDirty = true;
		ResetPreview();
		pKey = GetSelectedKeyframe();
	}

	if (!pKey)
		return;
	if (ImGui::DragFloat3("Position", &pKey->vPosition.x, 0.01f))
		m_bDirty = true;
	_float3 Euler = QuaternionToEulerDegrees(pKey->vRotation);
	if (ImGui::DragFloat3("Rotation", &Euler.x, 0.25f))
	{
		pKey->vRotation = EulerDegreesToQuaternion(Euler);
		m_bDirty = true;
	}
	if (PathPlaybackEditorDrawEnumCombo(
		"Interpolation", pKey->ePositionInterpolation))
		m_bDirty = true;
	if (PathPlaybackEditorDrawEnumCombo("Easing", pKey->eEasing))
		m_bDirty = true;

	char EventTag[128]{};
	PathPlaybackEditorCopyText(
		EventTag, sizeof(EventTag),
		pKey->sEventTag.hash != 0 ? pKey->sEventTag.GetDbgStr() : "");
	if (ImGui::InputText("Event Tag", EventTag, sizeof(EventTag)))
	{
		pKey->sEventTag = EventTag[0] == '\0' ? StringID{} : StringID{ EventTag };
		m_bDirty = true;
	}
}

void CPathPlaybackEditor::DrawPreviewControls()
{
	auto* pClip = GetSelectedClip();
	const _float fDuration = pClip && pClip->Keyframes.size() >= 2
		? pClip->Keyframes.back().fTime - pClip->Keyframes.front().fTime
		: 0.f;

	ImGui::Checkbox("Edit Gizmo", &m_bEditMode);
	ImGui::SameLine();
	ImGui::Checkbox("Show Path", &m_bShowPath);
	ImGui::SameLine();
	ImGui::Checkbox("Depth Test", &m_bDepthTest);
	ImGui::SameLine();
	ImGui::Checkbox("Snap", &m_bSnapEnabled);
	ImGui::SameLine();
	ImGui::TextUnformatted("Gizmo Target:");
	ImGui::SameLine();
	if (ImGui::RadioButton("Keyframe", !m_bEditAnchor))
		m_bEditAnchor = false;
	ImGui::SameLine();
	if (ImGui::RadioButton("Anchor", m_bEditAnchor))
		m_bEditAnchor = true;

	if (ImGui::RadioButton(
		"Move", m_eGizmoOperation == ImGuizmo::TRANSLATE))
		m_eGizmoOperation = ImGuizmo::TRANSLATE;
	ImGui::SameLine();
	if (ImGui::RadioButton(
		"Rotate", m_eGizmoOperation == ImGuizmo::ROTATE))
		m_eGizmoOperation = ImGuizmo::ROTATE;
	ImGui::SameLine();
	if (ImGui::RadioButton("World", m_eGizmoMode == ImGuizmo::WORLD))
		m_eGizmoMode = ImGuizmo::WORLD;
	ImGui::SameLine();
	if (ImGui::RadioButton("Local", m_eGizmoMode == ImGuizmo::LOCAL))
		m_eGizmoMode = ImGuizmo::LOCAL;
	ImGui::SameLine();
	ImGui::SetNextItemWidth(90.f);
	ImGui::DragFloat("Move Snap", &m_fTranslationSnap, 0.05f, 0.01f, 100.f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(90.f);
	ImGui::DragFloat("Rotate Snap", &m_fRotationSnap, 1.f, 1.f, 180.f);

	if (ImGui::Button("Reset Anchor"))
	{
		m_tPreviewAnchor = {};
		EvaluatePreviewPose(m_fPreviewTime);
	}
	if (ImGui::CollapsingHeader("Anchor Numeric Values"))
	{
		_bool bAnchorChanged = ImGui::InputFloat3(
			"Preview Anchor Position", &m_tPreviewAnchor.vPosition.x);
		_float3 AnchorEuler = QuaternionToEulerDegrees(m_tPreviewAnchor.vRotation);
		if (ImGui::InputFloat3("Preview Anchor Rotation", &AnchorEuler.x))
		{
			m_tPreviewAnchor.vRotation = EulerDegreesToQuaternion(AnchorEuler);
			bAnchorChanged = true;
		}
		if (bAnchorChanged)
			EvaluatePreviewPose(m_fPreviewTime);
	}

	PathPlaybackEditorBeginDisabled(!pClip || fDuration <= 0.f);
	if (ImGui::Button(m_bPreviewPlaying ? "Pause" : "Play"))
		m_bPreviewPlaying = !m_bPreviewPlaying;
	ImGui::SameLine();
	if (ImGui::Button("Stop"))
		ResetPreview();
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	ImGui::DragFloat("Rate", &m_fPreviewRate, 0.05f, 0.05f, 10.f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(240.f);
	if (ImGui::SliderFloat("Time", &m_fPreviewTime, 0.f, fDuration, "%.3f"))
	{
		m_bPreviewPlaying = false;
		EvaluatePreviewPose(m_fPreviewTime);
	}
	PathPlaybackEditorEndDisabled();
	ImGui::SetNextItemWidth(120.f);
	ImGui::SliderInt(
		"Debug Lines Per Key Span", &m_iSamplesPerSegment, 2, 32);
}

void CPathPlaybackEditor::DrawValidationErrors()
{
	if (m_ValidationErrors.empty())
		return;
	ImGui::Separator();
	ImGui::TextColored(ImVec4(1.f, 0.35f, 0.25f, 1.f), "Validation Errors");
	for (const auto& Error : m_ValidationErrors)
		ImGui::BulletText("%s", Error.c_str());
}

void CPathPlaybackEditor::DrawResultPopup()
{
	if (m_bOpenResultPopup)
	{
		ImGui::OpenPopup("Path Playback Result");
		m_bOpenResultPopup = false;
	}
	if (ImGui::BeginPopupModal(
		"Path Playback Result", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextColored(
			m_bResultPopupSuccess ? ImVec4(0.4f, 1.f, 0.5f, 1.f) :
			ImVec4(1.f, 0.35f, 0.25f, 1.f),
			"%s", m_sResultPopupMessage.c_str());
		if (ImGui::Button("OK", ImVec2(120.f, 0.f)))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}
}

void CPathPlaybackEditor::NewData()
{
	m_WorkingData = {};
	m_WorkingData.iVersion = PATH_PLAYBACK_DATA_VERSION;
	m_iSelectedClip = -1;
	m_iSelectedKeyframe = -1;
	m_ValidationErrors.clear();
	m_bDirty = true;
	m_sStatus = "Created empty PathPlayback data.";
	ResetPreview();
}

void CPathPlaybackEditor::AddClip()
{
	PATH_PLAYBACK_CLIP Clip{};
	Clip.sClipID = StringID{ MakeUniqueClipName(m_WorkingData, "Clip") };
	Clip.Keyframes.push_back({});
	PATH_PLAYBACK_KEYFRAME End{};
	End.fTime = 1.f;
	End.vPosition = { 0.f, 0.f, 1.f };
	Clip.Keyframes.push_back(End);
	m_WorkingData.Clips.push_back(std::move(Clip));
	m_iSelectedClip = static_cast<int32_t>(m_WorkingData.Clips.size() - 1);
	m_iSelectedKeyframe = 0;
	m_bDirty = true;
	ResetPreview();
}

void CPathPlaybackEditor::DuplicateSelectedClip()
{
	const auto* pClip = GetSelectedClip();
	if (!pClip)
		return;
	PATH_PLAYBACK_CLIP Copy = *pClip;
	Copy.sClipID = StringID{ MakeUniqueClipName(
		m_WorkingData, std::string{ pClip->sClipID.GetDbgStr() } + "_Copy") };
	m_WorkingData.Clips.push_back(std::move(Copy));
	m_iSelectedClip = static_cast<int32_t>(m_WorkingData.Clips.size() - 1);
	m_iSelectedKeyframe = 0;
	m_bDirty = true;
	ResetPreview();
}

void CPathPlaybackEditor::DeleteSelectedClip()
{
	if (!GetSelectedClip())
		return;
	m_WorkingData.Clips.erase(
		m_WorkingData.Clips.begin() + m_iSelectedClip);
	m_iSelectedClip = m_WorkingData.Clips.empty() ? -1 :
		std::min(m_iSelectedClip,
			static_cast<int32_t>(m_WorkingData.Clips.size() - 1));
	const auto* pClip = GetSelectedClip();
	m_iSelectedKeyframe = pClip && !pClip->Keyframes.empty() ? 0 : -1;
	m_bDirty = true;
	ResetPreview();
}

void CPathPlaybackEditor::AddKeyframe()
{
	auto* pClip = GetSelectedClip();
	if (!pClip)
		return;
	PATH_PLAYBACK_KEYFRAME Key{};
	if (!pClip->Keyframes.empty())
	{
		Key = pClip->Keyframes.back();
		Key.fTime += 1.f;
		Key.vPosition.z += 1.f;
		Key.sEventTag = {};
	}
	pClip->Keyframes.push_back(Key);
	pClip->SortKeyframes();
	m_iSelectedKeyframe = static_cast<int32_t>(pClip->Keyframes.size() - 1);
	m_bDirty = true;
	ResetPreview();
}

void CPathPlaybackEditor::DuplicateSelectedKeyframe()
{
	auto* pClip = GetSelectedClip();
	const auto* pKey = GetSelectedKeyframe();
	if (!pClip || !pKey)
		return;
	PATH_PLAYBACK_KEYFRAME Copy = *pKey;
	Copy.fTime += 0.1f;
	pClip->Keyframes.push_back(Copy);
	pClip->SortKeyframes();
	for (size_t i = 0; i < pClip->Keyframes.size(); ++i)
	{
		if (std::abs(pClip->Keyframes[i].fTime - Copy.fTime) <=
			PATH_PLAYBACK_EDITOR_TIME_EPSILON)
		{
			m_iSelectedKeyframe = static_cast<int32_t>(i);
			break;
		}
	}
	m_bDirty = true;
	ResetPreview();
}

void CPathPlaybackEditor::DeleteSelectedKeyframe()
{
	auto* pClip = GetSelectedClip();
	if (!pClip || !GetSelectedKeyframe())
		return;
	pClip->Keyframes.erase(pClip->Keyframes.begin() + m_iSelectedKeyframe);
	m_iSelectedKeyframe = pClip->Keyframes.empty() ? -1 :
		std::min(m_iSelectedKeyframe,
			static_cast<int32_t>(pClip->Keyframes.size() - 1));
	m_bDirty = true;
	ResetPreview();
}

HRESULT CPathPlaybackEditor::Save(_bool bBinary)
{
	if (!ValidateWorkingData())
	{
		QueueResultPopup("Save failed: validation errors exist.", false);
		return E_INVALIDARG;
	}

	const auto FilePath = MakeFilePath(bBinary);
	std::error_code Error{};
	std::filesystem::create_directories(FilePath.parent_path(), Error);
	if (Error)
	{
		QueueResultPopup("Save failed: could not create the output folder.", false);
		return E_FAIL;
	}

	const HRESULT hResult = bBinary
		? CGameInstance::Get().BinSerialize(
			FilePath.generic_string(), m_WorkingData,
			PATH_PLAYBACK_EDITOR_SERIALIZE_ROOT)
		: CGameInstance::Get().JsonSerialize(
			FilePath.generic_string(), m_WorkingData,
			PATH_PLAYBACK_EDITOR_SERIALIZE_ROOT);
	if (FAILED(hResult))
	{
		QueueResultPopup("PathPlayback save failed.", false);
		return hResult;
	}

	m_bDirty = false;
	m_sStatus = "Saved: " + FilePath.generic_string();
	QueueResultPopup(m_sStatus, true);
	return S_OK;
}

HRESULT CPathPlaybackEditor::Load(_bool bBinary)
{
	const auto FilePath = MakeFilePath(bBinary);
	PATH_PLAYBACK_DATA Loaded{};
	const HRESULT hResult = bBinary
		? CGameInstance::Get().BinDeSerialize(
			FilePath.generic_string(), Loaded,
			PATH_PLAYBACK_EDITOR_SERIALIZE_ROOT)
		: CGameInstance::Get().JsonDeSerialize(
			FilePath.generic_string(), Loaded,
			PATH_PLAYBACK_EDITOR_SERIALIZE_ROOT);
	if (FAILED(hResult))
	{
		QueueResultPopup("PathPlayback load failed: " +
			FilePath.generic_string(), false);
		return hResult;
	}

	std::vector<std::string> Errors{};
	if (!CResPathPlayback::ValidateAndNormalizeData(Loaded, &Errors))
	{
		m_ValidationErrors = std::move(Errors);
		QueueResultPopup("Loaded data failed validation.", false);
		return E_INVALIDARG;
	}

	m_WorkingData = std::move(Loaded);
	m_iSelectedClip = m_WorkingData.Clips.empty() ? -1 : 0;
	m_iSelectedKeyframe = m_iSelectedClip >= 0 &&
		!m_WorkingData.Clips[0].Keyframes.empty() ? 0 : -1;
	m_ValidationErrors.clear();
	m_bDirty = false;
	m_sStatus = "Loaded: " + FilePath.generic_string();
	ResetPreview();
	QueueResultPopup(m_sStatus, true);
	return S_OK;
}

std::filesystem::path CPathPlaybackEditor::MakeFilePath(_bool bBinary) const
{
	std::string Stem = m_FileName[0] == '\0' ? "NewPath" : m_FileName;
	const std::array<std::string_view, 4> Suffixes{
		".path.json", ".path.bin", ".json", ".bin" };
	for (const auto Suffix : Suffixes)
	{
		if (Stem.size() >= Suffix.size() &&
			Stem.compare(Stem.size() - Suffix.size(), Suffix.size(), Suffix) == 0)
		{
			Stem.erase(Stem.size() - Suffix.size());
			break;
		}
	}
	return std::filesystem::path{ PATH_PLAYBACK_EDITOR_ROOT } /
		(Stem + (bBinary ? ".path.bin" : ".path.json"));
}

_bool CPathPlaybackEditor::ValidateWorkingData()
{
	const _bool bValid = CResPathPlayback::ValidateAndNormalizeData(
		m_WorkingData, &m_ValidationErrors);
	if (bValid)
	{
		m_sStatus = "Validation succeeded.";
		ResetPreview();
	}
	else
	{
		m_sStatus = "Validation failed with " +
			std::to_string(m_ValidationErrors.size()) + " error(s).";
	}
	return bValid;
}

void CPathPlaybackEditor::QueueResultPopup(
	std::string Message,
	_bool bSuccess)
{
	m_sResultPopupMessage = std::move(Message);
	m_bResultPopupSuccess = bSuccess;
	m_bOpenResultPopup = true;
}

void CPathPlaybackEditor::ResetPreview()
{
	m_fPreviewTime = 0.f;
	m_bPreviewPlaying = false;
	m_bPreviewForward = true;
	m_tPreviewPose = m_tPreviewAnchor;
	m_bPreviewPoseValid = EvaluatePreviewPose(0.f);
}

void CPathPlaybackEditor::UpdatePreview(_float fTimeDelta)
{
	const auto* pClip = GetSelectedClip();
	if (!m_bPreviewPlaying || !pClip || pClip->Keyframes.size() < 2)
		return;

	const _float fDuration = pClip->Keyframes.back().fTime -
		pClip->Keyframes.front().fTime;
	if (fDuration <= 0.f)
		return;

	const _float fStep = std::max(0.f, fTimeDelta) *
		std::max(0.01f, m_fPreviewRate);
	m_fPreviewTime += m_bPreviewForward ? fStep : -fStep;

	switch (pClip->ePlayMode)
	{
	case PATH_PLAYBACK_MODE::LOOP:
		while (m_fPreviewTime > fDuration)
			m_fPreviewTime -= fDuration;
		while (m_fPreviewTime < 0.f)
			m_fPreviewTime += fDuration;
		break;

	case PATH_PLAYBACK_MODE::PING_PONG:
		if (m_fPreviewTime >= fDuration)
		{
			m_fPreviewTime = fDuration - (m_fPreviewTime - fDuration);
			m_bPreviewForward = false;
		}
		else if (m_fPreviewTime <= 0.f)
		{
			m_fPreviewTime = -m_fPreviewTime;
			m_bPreviewForward = true;
		}
		m_fPreviewTime = std::clamp(m_fPreviewTime, 0.f, fDuration);
		break;

	case PATH_PLAYBACK_MODE::ONCE:
	default:
		if (m_fPreviewTime >= fDuration)
		{
			m_fPreviewTime = fDuration;
			m_bPreviewPlaying = false;
		}
		break;
	}

	EvaluatePreviewPose(m_fPreviewTime);
}

_bool CPathPlaybackEditor::EvaluatePreviewPose(_float fElapsedTime)
{
	const auto* pClip = GetSelectedClip();
	if (!pClip)
	{
		m_bPreviewPoseValid = false;
		return false;
	}

	CPathPlaybackEvaluator::CONTEXT Context{};
	Context.tStartAnchorPose = m_tPreviewAnchor;
	Context.tCurrentObjectPose = m_tPreviewPose;
	Context.bHasCurrentObjectPose = m_bPreviewPoseValid;
	m_bPreviewPoseValid = CPathPlaybackEvaluator::EvaluatePose(
		*pClip, fElapsedTime, Context, m_tPreviewPose);
	return m_bPreviewPoseValid;
}

void CPathPlaybackEditor::DrawDebugPath()
{
	const auto* pClip = GetSelectedClip();
	if (!pClip || pClip->Keyframes.empty())
		return;
	auto* pDebug = CGameInstance::Get().GetDbgLineRender();
	if (!pDebug)
		return;

	const _float4 PreviousColor = pDebug->GetColor();
	const DBG_LINE_DEPTH_MODE PreviousDepth = pDebug->GetDepthMode();
	pDebug->SetDepthTest(m_bDepthTest);

	PATH_PLAYBACK_POSE PreviousPose{};
	_bool bHasPrevious = false;
	if (pClip->Keyframes.size() >= 2)
	{
		const _float fDuration = pClip->Keyframes.back().fTime -
			pClip->Keyframes.front().fTime;
		const int32_t iSampleCount = std::max(
			2, static_cast<int32_t>(pClip->Keyframes.size() - 1) *
				std::max(2, m_iSamplesPerSegment));
		for (int32_t i = 0; i <= iSampleCount; ++i)
		{
			const _float fTime = fDuration *
				static_cast<_float>(i) / static_cast<_float>(iSampleCount);
			PATH_PLAYBACK_POSE Pose{};
			CPathPlaybackEvaluator::CONTEXT Context{};
			Context.tStartAnchorPose = m_tPreviewAnchor;
			Context.tCurrentObjectPose = PreviousPose;
			Context.bHasCurrentObjectPose = bHasPrevious;
			if (!CPathPlaybackEvaluator::EvaluatePose(
				*pClip, fTime, Context, Pose))
				continue;
			if (bHasPrevious)
				pDebug->AddLine(
					PreviousPose.vPosition, Pose.vPosition,
					{ 0.15f, 0.85f, 1.f, 1.f });
			PreviousPose = Pose;
			bHasPrevious = true;
		}
	}

	for (size_t i = 0; i < pClip->Keyframes.size(); ++i)
	{
		const _matrix World = MakeKeyframeWorld(*pClip, pClip->Keyframes[i]);
		_float3 Position{};
		XMStoreFloat3(&Position, World.r[3]);
		if (m_iSelectedKeyframe == static_cast<int32_t>(i))
		{
			pDebug->SetColor({ 1.f, 0.85f, 0.1f, 1.f });
			pDebug->AddSphere(0.15f, XMMatrixTranslationFromVector(World.r[3]));
		}
		else
		{
			pDebug->SetColor({ 0.2f, 1.f, 0.35f, 1.f });
			pDebug->AddCross(Position, 0.12f);
		}
	}

	if (m_bPreviewPoseValid)
	{
		const _matrix PreviewWorld =
			XMMatrixRotationQuaternion(XMLoadFloat4(&m_tPreviewPose.vRotation)) *
			XMMatrixTranslationFromVector(XMLoadFloat3(&m_tPreviewPose.vPosition));
		pDebug->SetColor({ 1.f, 0.2f, 0.9f, 1.f });
		pDebug->AddCross(m_tPreviewPose.vPosition, 0.25f);
		_float3 Forward{};
		XMStoreFloat3(&Forward, XMVector3Normalize(PreviewWorld.r[2]));
		pDebug->AddArrow(m_tPreviewPose.vPosition, Forward, 0.75f);
	}

	pDebug->SetColor(PreviousColor);
	pDebug->SetDepthMode(PreviousDepth);
}

void CPathPlaybackEditor::RenderGizmo()
{
	auto* pClip = GetSelectedClip();
	auto* pKey = GetSelectedKeyframe();
	auto* pCamera = CGameInstance::Get().GetActiveCamera();
	if (!pCamera || (!m_bEditAnchor && (!pClip || !pKey)))
		return;

	_float4x4 View{};
	_float4x4 Projection{};
	_float4x4 World{};
	XMStoreFloat4x4(&View, pCamera->GetView());
	XMStoreFloat4x4(&Projection, pCamera->GetProj());
	XMStoreFloat4x4(
		&World,
		m_bEditAnchor ? MakeAnchorWorld() :
		MakeKeyframeWorld(*pClip, *pKey));

	ImGuiViewport* pViewport = ImGui::GetMainViewport();
	if (!pViewport)
		return;
	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList(pViewport));
	ImGuizmo::SetRect(
		pViewport->Pos.x, pViewport->Pos.y,
		pViewport->Size.x, pViewport->Size.y);
	ImGuizmo::SetID(PATH_PLAYBACK_EDITOR_GIZMO_ID);

	_float3 Snap{};
	const _float* pSnap = nullptr;
	if (m_bSnapEnabled)
	{
		const _float fValue = m_eGizmoOperation == ImGuizmo::TRANSLATE
			? std::max(m_fTranslationSnap, 0.01f)
			: std::max(m_fRotationSnap, 1.f);
		Snap = { fValue, fValue, fValue };
		pSnap = &Snap.x;
	}

	if (!ImGuizmo::Manipulate(
		&View._11, &Projection._11,
		m_eGizmoOperation, m_eGizmoMode,
		&World._11, nullptr, pSnap))
		return;

	if (m_bEditAnchor)
	{
		_vector Scale{};
		_vector Rotation{};
		_vector Translation{};
		if (!XMMatrixDecompose(
			&Scale, &Rotation, &Translation,
			XMLoadFloat4x4(&World)))
		{
			return;
		}
		XMStoreFloat3(&m_tPreviewAnchor.vPosition, Translation);
		XMStoreFloat4(
			&m_tPreviewAnchor.vRotation,
			XMQuaternionNormalize(Rotation));
		EvaluatePreviewPose(m_fPreviewTime);
		return;
	}

	ApplyKeyframeWorld(*pClip, *pKey, XMLoadFloat4x4(&World));
	m_bDirty = true;
	m_bPreviewPlaying = false;
	m_fPreviewTime = std::max(
		0.f, pKey->fTime - pClip->Keyframes.front().fTime);
	EvaluatePreviewPose(m_fPreviewTime);
}

PATH_PLAYBACK_CLIP* CPathPlaybackEditor::GetSelectedClip()
{
	if (m_iSelectedClip < 0 ||
		static_cast<size_t>(m_iSelectedClip) >= m_WorkingData.Clips.size())
		return nullptr;
	return &m_WorkingData.Clips[m_iSelectedClip];
}

const PATH_PLAYBACK_CLIP* CPathPlaybackEditor::GetSelectedClip() const
{
	if (m_iSelectedClip < 0 ||
		static_cast<size_t>(m_iSelectedClip) >= m_WorkingData.Clips.size())
		return nullptr;
	return &m_WorkingData.Clips[m_iSelectedClip];
}

PATH_PLAYBACK_KEYFRAME* CPathPlaybackEditor::GetSelectedKeyframe()
{
	auto* pClip = GetSelectedClip();
	if (!pClip || m_iSelectedKeyframe < 0 ||
		static_cast<size_t>(m_iSelectedKeyframe) >= pClip->Keyframes.size())
		return nullptr;
	return &pClip->Keyframes[m_iSelectedKeyframe];
}

const PATH_PLAYBACK_KEYFRAME* CPathPlaybackEditor::GetSelectedKeyframe() const
{
	const auto* pClip = GetSelectedClip();
	if (!pClip || m_iSelectedKeyframe < 0 ||
		static_cast<size_t>(m_iSelectedKeyframe) >= pClip->Keyframes.size())
		return nullptr;
	return &pClip->Keyframes[m_iSelectedKeyframe];
}

_matrix CPathPlaybackEditor::MakeAnchorWorld() const
{
	return XMMatrixRotationQuaternion(
		XMQuaternionNormalize(XMLoadFloat4(&m_tPreviewAnchor.vRotation))) *
		XMMatrixTranslationFromVector(XMLoadFloat3(&m_tPreviewAnchor.vPosition));
}

_matrix CPathPlaybackEditor::MakeKeyframeWorld(
	const PATH_PLAYBACK_CLIP& Clip,
	const PATH_PLAYBACK_KEYFRAME& Keyframe) const
{
	_matrix World = XMMatrixRotationQuaternion(
		XMQuaternionNormalize(XMLoadFloat4(&Keyframe.vRotation))) *
		XMMatrixTranslationFromVector(XMLoadFloat3(&Keyframe.vPosition));
	if (Clip.eCoordinateSpace == PATH_PLAYBACK_COORDINATE_SPACE::START_LOCAL)
		World *= MakeAnchorWorld();
	return World;
}

void CPathPlaybackEditor::ApplyKeyframeWorld(
	const PATH_PLAYBACK_CLIP& Clip,
	PATH_PLAYBACK_KEYFRAME& Keyframe,
	FXMMATRIX World)
{
	_matrix Stored = World;
	if (Clip.eCoordinateSpace == PATH_PLAYBACK_COORDINATE_SPACE::START_LOCAL)
	{
		_vector Determinant{};
		const _matrix InverseAnchor = XMMatrixInverse(&Determinant, MakeAnchorWorld());
		Stored = World * InverseAnchor;
	}

	_vector Scale{};
	_vector Rotation{};
	_vector Translation{};
	if (!XMMatrixDecompose(&Scale, &Rotation, &Translation, Stored))
		return;
	XMStoreFloat3(&Keyframe.vPosition, Translation);
	XMStoreFloat4(&Keyframe.vRotation, XMQuaternionNormalize(Rotation));
}

_float3 CPathPlaybackEditor::QuaternionToEulerDegrees(
	const _float4& Rotation)
{
	_float4x4 Matrix{};
	XMStoreFloat4x4(
		&Matrix,
		XMMatrixRotationQuaternion(
			XMQuaternionNormalize(XMLoadFloat4(&Rotation))));
	_float Translation[3]{};
	_float Euler[3]{};
	_float Scale[3]{};
	ImGuizmo::DecomposeMatrixToComponents(
		&Matrix._11, Translation, Euler, Scale);
	return { Euler[0], Euler[1], Euler[2] };
}

_float4 CPathPlaybackEditor::EulerDegreesToQuaternion(
	const _float3& EulerDegrees)
{
	_float4 Result{};
	XMStoreFloat4(
		&Result,
		XMQuaternionNormalize(
			XMQuaternionRotationRollPitchYaw(
				XMConvertToRadians(EulerDegrees.x),
				XMConvertToRadians(EulerDegrees.y),
				XMConvertToRadians(EulerDegrees.z))));
	return Result;
}

std::string CPathPlaybackEditor::MakeUniqueClipName(
	const PATH_PLAYBACK_DATA& Data,
	std::string BaseName)
{
	for (uint32_t i = 0;; ++i)
	{
		const std::string Candidate = BaseName + "_" + std::to_string(i);
		const _bool bExists = std::ranges::any_of(
			Data.Clips,
			[&Candidate](const PATH_PLAYBACK_CLIP& Clip)
			{
				return Candidate == Clip.sClipID.GetDbgStr();
			});
		if (!bExists)
			return Candidate;
	}
}

UPtr<CPathPlaybackEditor> CPathPlaybackEditor::Create()
{
	return ToUPtr(new CPathPlaybackEditor{});
}
