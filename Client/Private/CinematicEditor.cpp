#include "pch.h"
#include "CinematicEditor.h"

#include "CinematicAsset.h"
#include "CinematicTypes.h"
#include "GameInstance.h"
#include "GameObject.h"

NS_USING(Client)

namespace
{
	constexpr const _char* CINEMATIC_SAVE_ROOT = "./Resources/json/Cinematics";
	constexpr const _char* CINEMATIC_JSON_ROOT = "Cinematic";

	_bool TryBuildCinematicPath(
		const _char* pCinematicID,
		std::filesystem::path& OutPath,
		std::string& OutError)
	{
		if (pCinematicID == nullptr || pCinematicID[0] == '\0')
		{
			OutError = "Cinematic ID cannot be empty.";
			return false;
		}

		const std::string CinematicID{ pCinematicID };
		if (CinematicID == "." ||
			CinematicID == ".." ||
			CinematicID.find_first_of("<>:\"/\\|?*") !=
				std::string::npos ||
			static_cast<unsigned char>(CinematicID.back()) <= 0x20 ||
			CinematicID.back() == '.')
		{
			OutError =
				"Cinematic ID contains invalid filename characters.";
			return false;
		}

		for (const unsigned char Character : CinematicID)
		{
			if (Character < 0x20)
			{
				OutError =
					"Cinematic ID contains invalid filename characters.";
				return false;
			}
		}

		OutPath =
			std::filesystem::path{ CINEMATIC_SAVE_ROOT } /
			(CinematicID + ".json");
		return true;
	}

	std::string MakeSerializeStatus(
		const _char* pOperation,
		const E::SERIALIZE_RESULT& Result)
	{
		std::string Status{ pOperation };
		Status += " failed";
		if (!Result.sMessage.empty())
		{
			Status += ": ";
			Status += Result.sMessage;
		}
		return Status;
	}

	_vector LoadSafeQuaternion(const _float4& vRotation)
	{
		const _vector vQuaternion =
			XMLoadFloat4(&vRotation);
		_float fLengthSq{};
		XMStoreFloat(
			&fLengthSq,
			XMVector4LengthSq(vQuaternion));

		return fLengthSq > FLT_EPSILON ?
			XMQuaternionNormalize(vQuaternion) :
			XMQuaternionIdentity();
	}

	_bool TryProjectToViewport(const _float3& vPosition,_fmatrix matViewProjection,const ImGuiViewport& Viewport,ImVec2& OutScreenPosition)
	{
		const _vector vClipPosition = XMVector4Transform(XMVectorSet(vPosition.x,vPosition.y,vPosition.z,1.f), matViewProjection);
		const _float fClipW = XMVectorGetW(vClipPosition);
		if (fClipW <= FLT_EPSILON)
		{
			return false;
		}

		const _vector vNdcPosition = vClipPosition / fClipW;
		const _float fNdcX = XMVectorGetX(vNdcPosition);
		const _float fNdcY = XMVectorGetY(vNdcPosition);
		const _float fNdcZ = XMVectorGetZ(vNdcPosition);
		if (fNdcX < -1.f || fNdcX > 1.f ||fNdcY < -1.f || fNdcY > 1.f || fNdcZ < 0.f || fNdcZ > 1.f)
		{
			return false;
		}

		OutScreenPosition = 
		{
			Viewport.Pos.x + (fNdcX * 0.5f + 0.5f) * Viewport.Size.x,
			Viewport.Pos.y + (-fNdcY * 0.5f + 0.5f) * Viewport.Size.y
		};
		return true;
	}
}

CCinematicEditor::CCinematicEditor()
{
}

CCinematicEditor::~CCinematicEditor()
{
}

void CCinematicEditor::UpdateGUI()
{
	ImGui::Begin("Cinematic Editor");

	ImGui::InputText(
		"Cinematic ID",
		m_szCinematicID,
		IM_ARRAYSIZE(m_szCinematicID));

	if (ImGui::Button("New Asset"))
	{
		CreateAsset();
	}

	ImGui::SameLine();
	if (ImGui::Button("Load"))
	{
		LoadAsset();
	}

	if (m_pEditingAsset == nullptr)
	{
		ImGui::TextWrapped("%s", m_Status.c_str());
		ImGui::End();
		return;
	}

	if (m_PreviewTargetHandle.has_value() &&
		E::CGameInstance::Get().GetGameObjectByHandle(
			*m_PreviewTargetHandle) == nullptr)
	{
		m_PreviewTargetHandle.reset();
	}

	ImGui::SameLine();
	if (ImGui::Button("Save"))
	{
		SaveAsset();
	}

	ImGui::SameLine();
	if (ImGui::Button("Register"))
	{
		m_pEditingAsset->RecalculateDuration();
		const HRESULT hr =
			E::CGameInstance::Get().RegistCinematicAsset(m_pEditingAsset);
		m_Status = hr == S_OK ?
			"Asset registered." :
			"Failed to register asset.";
	}

	ImGui::SameLine();
	if (ImGui::Button("Play"))
	{
		HRESULT hr = E_INVALIDARG;
		if (HasTargetLocalShot())
		{
			if (m_PreviewTargetHandle.has_value())
			{
				hr = E::CGameInstance::Get().PlayCinematic(
					m_pEditingAsset->GetCinematicID(),
					*m_PreviewTargetHandle);
			}
		}
		else
		{
			hr = E::CGameInstance::Get().PlayCinematic(
				m_pEditingAsset->GetCinematicID());
		}

		m_Status = hr == S_OK ?
			"Playback started." :
			"Playback failed. Register the asset and select a valid preview target.";
	}

	ImGui::SameLine();
	if (ImGui::Button("Stop"))
	{
		E::CGameInstance::Get().StopCinematic();
		m_Status = "Playback stopped.";
	}

	const _bool bPlaying =
		E::CGameInstance::Get().IsCinematicPlaying();
	ImGui::Text(
		"State: %s | Time: %.3f",
		bPlaying ? "Playing" : "Stopped",
		E::CGameInstance::Get().GetCinematicPlayTime());

	m_pEditingAsset->RecalculateDuration();
	ImGui::Text(
		"Asset: %s | Duration: %.3f",
		m_pEditingAsset->GetCinematicID().GetDbgStr(),
		m_pEditingAsset->GetDuration());

	DrawPreviewTargetSelector();

	if (m_SelectedShotIndex.has_value() &&
		m_SelectedKeyframeIndex.has_value())
	{
		ImGui::Text(
			"Selected: Shot %zu / Keyframe %zu",
			*m_SelectedShotIndex,
			*m_SelectedKeyframeIndex);

		if (ImGui::Button("Translate Gizmo (W)"))
		{
			m_GizmoOperation = ImGuizmo::TRANSLATE;
		}
		ImGui::SameLine();
		if (ImGui::Button("Rotate Gizmo (E)"))
		{
			m_GizmoOperation = ImGuizmo::ROTATE;
		}
	}

	ImGui::DragFloat(
		"Frustum Display Size",
		&m_fFrustumDisplaySize,
		0.05f,
		0.1f,
		10.f,
		"%.2f");
	m_fFrustumDisplaySize =
		std::clamp(m_fFrustumDisplaySize, 0.1f, 10.f);

	ImGui::Separator();

	auto& Track = m_pEditingAsset->GetMutableCameraTrack();
	ImGui::Text("Camera Track: %s", Track.TrackID.GetDbgStr());

	_bool bChanged = false;
	std::optional<size_t> RemoveShotIndex{};
	std::optional<size_t> StartTimeEditedShotIndex{};

	for (size_t i = 0; i < Track.Shots.size(); ++i)
	{
		ImGui::PushID(static_cast<int>(i));

		if (ImGui::Button("Remove Shot"))
		{
			RemoveShotIndex = i;
		}

		_bool bStartTimeEditFinished = false;
		DrawShot(
			Track.Shots[i],
			i,
			bChanged,
			bStartTimeEditFinished);
		if (bStartTimeEditFinished)
		{
			StartTimeEditedShotIndex = i;
		}
		ImGui::PopID();

		if (RemoveShotIndex.has_value())
		{
			break;
		}
	}

	if (StartTimeEditedShotIndex.has_value() &&
		!RemoveShotIndex.has_value())
	{
		const size_t iOldIndex =
			*StartTimeEditedShotIndex;
		const _float fEditedStartTime =
			Track.Shots[iOldIndex].fStartTime;
		size_t iNewIndex = 0;

		for (size_t i = 0; i < Track.Shots.size(); ++i)
		{
			if (i == iOldIndex)
			{
				continue;
			}

			if (Track.Shots[i].fStartTime < fEditedStartTime ||
				(Track.Shots[i].fStartTime == fEditedStartTime &&
				 i < iOldIndex))
			{
				++iNewIndex;
			}
		}

		if (m_SelectedShotIndex.has_value())
		{
			size_t& iSelectedIndex = *m_SelectedShotIndex;

			if (iSelectedIndex == iOldIndex)
			{
				iSelectedIndex = iNewIndex;
			}
			else if (iOldIndex < iNewIndex &&
				iSelectedIndex > iOldIndex &&
				iSelectedIndex <= iNewIndex)
			{
				--iSelectedIndex;
			}
			else if (iNewIndex < iOldIndex &&
				iSelectedIndex >= iNewIndex &&
				iSelectedIndex < iOldIndex)
			{
				++iSelectedIndex;
			}
		}

		Track.SortShots();
		bChanged = true;
	}

	if (RemoveShotIndex.has_value())
	{
		if (m_SelectedShotIndex == RemoveShotIndex)
		{
			m_SelectedShotIndex.reset();
			m_SelectedKeyframeIndex.reset();
		}
		else if (m_SelectedShotIndex.has_value() &&
			*m_SelectedShotIndex > *RemoveShotIndex)
		{
			--(*m_SelectedShotIndex);
		}

		Track.Shots.erase(
			Track.Shots.begin() +
			static_cast<std::ptrdiff_t>(*RemoveShotIndex));
		if (StartTimeEditedShotIndex.has_value())
		{
			Track.SortShots();
			m_SelectedShotIndex.reset();
			m_SelectedKeyframeIndex.reset();
		}
		bChanged = true;
	}

	if (ImGui::Button("Add Shot"))
	{
		AddShot();
		bChanged = true;
	}

	if (bChanged)
	{
		m_pEditingAsset->RecalculateDuration();
	}

	ImGui::Separator();
	ImGui::TextWrapped("%s", m_Status.c_str());
	ImGui::End();

	DrawViewportEditor();
}

void CCinematicEditor::CreateAsset()
{
	if (m_szCinematicID[0] == '\0')
	{
		m_Status = "Cinematic ID cannot be empty.";
		return;
	}

	E::CGameInstance::Get().StopCinematic();

	const E::StringID CinematicID{
		std::string{ m_szCinematicID } };
	m_pEditingAsset = E::CCinematicAsset::Create(CinematicID);
	m_pEditingAsset->GetMutableCameraTrack().TrackID =
		E::StringID{ std::string{ "CameraTrack" } };
	m_SelectedShotIndex.reset();
	m_SelectedKeyframeIndex.reset();
	m_PreviewTargetHandle.reset();

	m_Status = "New asset created.";
}

void CCinematicEditor::SaveAsset()
{
	if (m_pEditingAsset == nullptr)
	{
		m_Status = "Create or load a cinematic asset first.";
		return;
	}

	const _char* pCinematicID =
		m_pEditingAsset->GetCinematicID().GetDbgStr();
	std::filesystem::path SavePath{};
	if (!TryBuildCinematicPath(
		pCinematicID,
		SavePath,
		m_Status))
	{
		return;
	}

	m_pEditingAsset->RecalculateDuration();
	E::FCinematicAssetData Data =
		m_pEditingAsset->ExportData();
	Data.CameraTrack.SortShots();
	for (auto& Shot : Data.CameraTrack.Shots)
	{
		Shot.SortKeyFrames();
	}

	const E::SERIALIZE_RESULT Result =
		E::CGameInstance::Get().JsonSerializeDetailed(
			SavePath.generic_string(),
			Data,
			CINEMATIC_JSON_ROOT);
	if (Result.Failed())
	{
		m_Status = MakeSerializeStatus("Save", Result);
		return;
	}

	m_Status =
		"Saved: " + SavePath.generic_string();
}

void CCinematicEditor::LoadAsset()
{
	std::filesystem::path LoadPath{};
	if (!TryBuildCinematicPath(
		m_szCinematicID,
		LoadPath,
		m_Status))
	{
		return;
	}

	E::FCinematicAssetData Data{};
	const E::SERIALIZE_RESULT Result =
		E::CGameInstance::Get().JsonDeSerializeDetailed(
			LoadPath.generic_string(),
			Data,
			CINEMATIC_JSON_ROOT);
	if (Result.Failed())
	{
		m_Status = MakeSerializeStatus("Load", Result);
		return;
	}

	auto pLoadedAsset = E::CCinematicAsset::Create(Data);
	if (pLoadedAsset == nullptr)
	{
		m_Status = "Load failed: Cinematic ID is empty.";
		return;
	}

	E::CGameInstance::Get().StopCinematic();
	m_pEditingAsset = std::move(pLoadedAsset);
	m_SelectedShotIndex.reset();
	m_SelectedKeyframeIndex.reset();
	m_PreviewTargetHandle.reset();

	strncpy_s(
		m_szCinematicID,
		m_pEditingAsset->GetCinematicID().GetDbgStr(),
		_TRUNCATE);
	m_Status =
		"Loaded: " + LoadPath.generic_string();
}

void CCinematicEditor::DrawPreviewTargetSelector()
{
	if (!HasTargetLocalShot())
	{
		return;
	}

	auto& GameInstance = E::CGameInstance::Get();
	const E::CGameObject* pSelectedObject = nullptr;
	if (m_PreviewTargetHandle.has_value())
	{
		pSelectedObject = GameInstance.GetGameObjectByHandle(
			*m_PreviewTargetHandle);
	}

	std::string PreviewLabel{ "None" };
	if (pSelectedObject != nullptr)
	{
		PreviewLabel =
			std::string{ pSelectedObject->GetObjectTag() } +
			" [" +
			std::to_string(m_PreviewTargetHandle->GetIndex()) +
			":" +
			std::to_string(m_PreviewTargetHandle->GetGeneration()) +
			"]";
	}

	if (!ImGui::BeginCombo(
		"Preview Target",
		PreviewLabel.c_str()))
	{
		return;
	}

	if (ImGui::Selectable(
		"None",
		!m_PreviewTargetHandle.has_value()))
	{
		m_PreviewTargetHandle.reset();
	}

	for (const auto& [LayerName, Handles] :
		GameInstance.GetGameObjectLayers())
	{
		for (const E::CHandle& Handle : Handles)
		{
			const E::CGameObject* pObject =
				GameInstance.GetGameObjectByHandle(Handle);
			if (pObject == nullptr ||
				pObject->GetPendingDestroy())
			{
				continue;
			}

			const std::string Label =
				LayerName +
				" / " +
				std::string{ pObject->GetObjectTag() } +
				" [" +
				std::to_string(Handle.GetIndex()) +
				":" +
				std::to_string(Handle.GetGeneration()) +
				"]";
			const _bool bSelected =
				m_PreviewTargetHandle.has_value() &&
				*m_PreviewTargetHandle == Handle;

			if (ImGui::Selectable(
				Label.c_str(),
				bSelected))
			{
				m_PreviewTargetHandle = Handle;
			}

			if (bSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
	}

	ImGui::EndCombo();
}

void CCinematicEditor::AddShot()
{
	if (m_pEditingAsset == nullptr)
	{
		return;
	}

	m_pEditingAsset->RecalculateDuration();
	auto& Track = m_pEditingAsset->GetMutableCameraTrack();

	E::FCinematicCameraShot Shot{};
	Shot.ShotID = E::StringID{
		std::string{ "Shot_" } + std::to_string(Track.Shots.size()) };
	Shot.fStartTime =
		Track.Shots.empty() ? 0.f : m_pEditingAsset->GetDuration();
	Shot.eCoordinateSpace = E::ECinematicCoordinateSpace::World;

	E::FCinematicCameraKeyframe Keyframe{};
	Keyframe.fTime = 0.f;
	Shot.Keyframes.push_back(Keyframe);

	Track.Shots.push_back(std::move(Shot));
	Track.SortShots();
	m_SelectedShotIndex = Track.Shots.size() - 1;
	m_SelectedKeyframeIndex = 0;
}

void CCinematicEditor::DrawShot(
	E::FCinematicCameraShot& Shot,
	size_t iShotIndex,
	_bool& bChanged,
	_bool& bStartTimeEditFinished)
{
	const _bool bOpen = ImGui::TreeNodeEx(
		"##Shot",
		ImGuiTreeNodeFlags_DefaultOpen,
		"Shot %zu: %s",
		iShotIndex,
		Shot.ShotID.GetDbgStr());
	if (!bOpen)
	{
		return;
	}

	if (ImGui::DragFloat(
		"Start Time",
		&Shot.fStartTime,
		0.05f,
		0.f,
		FLT_MAX,
		"%.3f"))
	{
		Shot.fStartTime = std::max(0.f, Shot.fStartTime);
		bChanged = true;
	}
	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		bStartTimeEditFinished = true;
	}

	static const _char* CoordinateSpaceNames[] = {
		"World",
		"TargetLocal"
	};
	int iCoordinateSpace =
		static_cast<int>(Shot.eCoordinateSpace);
	if (ImGui::Combo(
		"Coordinate Space",
		&iCoordinateSpace,
		CoordinateSpaceNames,
		IM_ARRAYSIZE(CoordinateSpaceNames)))
	{
		Shot.eCoordinateSpace =
			static_cast<E::ECinematicCoordinateSpace>(iCoordinateSpace);
		bChanged = true;
	}

	if (Shot.eCoordinateSpace ==
		E::ECinematicCoordinateSpace::TargetLocal)
	{
		if (m_PreviewTargetHandle.has_value())
		{
			ImGui::TextColored(
				ImVec4{ 0.4f, 0.9f, 0.5f, 1.f },
				"Editing relative to the preview target.");
		}
		else
		{
			ImGui::TextColored(
				ImVec4{ 1.f, 0.75f, 0.25f, 1.f },
				"Select a Preview Target above.");
		}
	}

	std::optional<size_t> RemoveKeyframeIndex{};
	std::optional<size_t> TimeEditedKeyframeIndex{};
	for (size_t i = 0; i < Shot.Keyframes.size(); ++i)
	{
		ImGui::PushID(static_cast<int>(i));

		_bool bRemove = false;
		_bool bTimeEditFinished = false;
		DrawKeyframe(
			Shot.Keyframes[i],
			iShotIndex,
			i,
			bChanged,
			bRemove,
			bTimeEditFinished);
		if (bRemove)
		{
			RemoveKeyframeIndex = i;
		}
		if (bTimeEditFinished)
		{
			TimeEditedKeyframeIndex = i;
		}

		ImGui::PopID();
		if (RemoveKeyframeIndex.has_value())
		{
			break;
		}
	}

	if (TimeEditedKeyframeIndex.has_value() &&
		!RemoveKeyframeIndex.has_value())
	{
		const size_t iOldIndex =
			*TimeEditedKeyframeIndex;
		const _float fEditedTime =
			Shot.Keyframes[iOldIndex].fTime;
		size_t iNewIndex = 0;

		for (size_t i = 0; i < Shot.Keyframes.size(); ++i)
		{
			if (i == iOldIndex)
			{
				continue;
			}

			if (Shot.Keyframes[i].fTime < fEditedTime ||
				(Shot.Keyframes[i].fTime == fEditedTime &&
				 i < iOldIndex))
			{
				++iNewIndex;
			}
		}

		if (m_SelectedShotIndex == iShotIndex &&
			m_SelectedKeyframeIndex.has_value())
		{
			size_t& iSelectedIndex =
				*m_SelectedKeyframeIndex;

			if (iSelectedIndex == iOldIndex)
			{
				iSelectedIndex = iNewIndex;
			}
			else if (iOldIndex < iNewIndex &&
				iSelectedIndex > iOldIndex &&
				iSelectedIndex <= iNewIndex)
			{
				--iSelectedIndex;
			}
			else if (iNewIndex < iOldIndex &&
				iSelectedIndex >= iNewIndex &&
				iSelectedIndex < iOldIndex)
			{
				++iSelectedIndex;
			}
		}

		Shot.SortKeyFrames();
		bChanged = true;
	}

	if (RemoveKeyframeIndex.has_value())
	{
		if (m_SelectedShotIndex == iShotIndex &&
			m_SelectedKeyframeIndex == RemoveKeyframeIndex)
		{
			m_SelectedShotIndex.reset();
			m_SelectedKeyframeIndex.reset();
		}
		else if (m_SelectedShotIndex == iShotIndex &&
			m_SelectedKeyframeIndex.has_value() &&
			*m_SelectedKeyframeIndex > *RemoveKeyframeIndex)
		{
			--(*m_SelectedKeyframeIndex);
		}

		Shot.Keyframes.erase(
			Shot.Keyframes.begin() +
			static_cast<std::ptrdiff_t>(*RemoveKeyframeIndex));
		if (TimeEditedKeyframeIndex.has_value())
		{
			Shot.SortKeyFrames();
			if (m_SelectedShotIndex == iShotIndex)
			{
				m_SelectedShotIndex.reset();
				m_SelectedKeyframeIndex.reset();
			}
		}
		bChanged = true;
	}

	if (ImGui::Button("Add Keyframe"))
	{
		E::FCinematicCameraKeyframe NewKeyframe{};

		if (!Shot.Keyframes.empty())
		{
			const auto Iter = std::max_element(
				Shot.Keyframes.begin(),
				Shot.Keyframes.end(),
				[](const auto& Left, const auto& Right)
				{
					return Left.fTime < Right.fTime;
				});

			NewKeyframe = *Iter;
			NewKeyframe.fTime = Iter->fTime + 1.f;
			NewKeyframe.ePositionInterpolation =
				E::ECinematicInterpolation::Linear;
		}

		Shot.Keyframes.push_back(NewKeyframe);
		Shot.SortKeyFrames();
		m_SelectedShotIndex = iShotIndex;
		m_SelectedKeyframeIndex = Shot.Keyframes.size() - 1;
		bChanged = true;
	}

	ImGui::TreePop();
}

void CCinematicEditor::DrawKeyframe(
	E::FCinematicCameraKeyframe& Keyframe,
	size_t iShotIndex,
	size_t iKeyframeIndex,
	_bool& bChanged,
	_bool& bRemove,
	_bool& bTimeEditFinished)
{
	const _bool bOpen = ImGui::TreeNodeEx(
		"##Keyframe",
		ImGuiTreeNodeFlags_DefaultOpen,
		"Keyframe %zu",
		iKeyframeIndex);
	if (!bOpen)
	{
		return;
	}

	const _bool bSelected =
		m_SelectedShotIndex == iShotIndex &&
		m_SelectedKeyframeIndex == iKeyframeIndex;
	if (bSelected)
	{
		ImGui::TextColored(
			ImVec4{ 1.f, 0.85f, 0.2f, 1.f },
			"Selected in viewport");
	}
	else if (ImGui::Button("Select Keyframe"))
	{
		m_SelectedShotIndex = iShotIndex;
		m_SelectedKeyframeIndex = iKeyframeIndex;
	}

	if (ImGui::Button("Remove Keyframe"))
	{
		bRemove = true;
		ImGui::TreePop();
		return;
	}

	if (ImGui::DragFloat(
		"Time",
		&Keyframe.fTime,
		0.05f,
		0.f,
		FLT_MAX,
		"%.3f"))
	{
		Keyframe.fTime = std::max(0.f, Keyframe.fTime);
		bChanged = true;
	}
	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		bTimeEditFinished = true;
	}

	if (ImGui::DragFloat3(
		"Position",
		&Keyframe.vPosition.x,
		0.05f))
	{
		bChanged = true;
	}

	_vector vQuaternion = XMLoadFloat4(&Keyframe.vRotation);
	_float fQuaternionLengthSq{};
	XMStoreFloat(
		&fQuaternionLengthSq,
		XMVector4LengthSq(vQuaternion));
	if (fQuaternionLengthSq <= FLT_EPSILON)
	{
		vQuaternion = XMQuaternionIdentity();
	}
	else
	{
		vQuaternion = XMQuaternionNormalize(vQuaternion);
	}

	const SimpleMath::Quaternion Quaternion{ vQuaternion };
	const SimpleMath::Vector3 EulerRadians = Quaternion.ToEuler();
	_float3 vEulerDegrees{
		XMConvertToDegrees(EulerRadians.x),
		XMConvertToDegrees(EulerRadians.y),
		XMConvertToDegrees(EulerRadians.z)
	};

	if (ImGui::DragFloat3(
		"Rotation Euler",
		&vEulerDegrees.x,
		0.25f))
	{
		XMStoreFloat4(
			&Keyframe.vRotation,
			XMQuaternionRotationRollPitchYaw(
				XMConvertToRadians(vEulerDegrees.x),
				XMConvertToRadians(vEulerDegrees.y),
				XMConvertToRadians(vEulerDegrees.z)));
		bChanged = true;
	}

	if (ImGui::DragFloat(
		"FOV",
		&Keyframe.fFovY,
		0.1f,
		1.f,
		179.f,
		"%.1f"))
	{
		Keyframe.fFovY =
			std::clamp(Keyframe.fFovY, 1.f, 179.f);
		bChanged = true;
	}

	static const _char* InterpolationNames[] = {
		"Linear",
		"CatmullRom"
	};
	int iInterpolation =
		static_cast<int>(Keyframe.ePositionInterpolation);
	if (ImGui::Combo(
		"Position Interpolation",
		&iInterpolation,
		InterpolationNames,
		IM_ARRAYSIZE(InterpolationNames)))
	{
		Keyframe.ePositionInterpolation =
			static_cast<E::ECinematicInterpolation>(iInterpolation);
		bChanged = true;
	}

	ImGui::TreePop();
}

void CCinematicEditor::DrawViewportEditor()
{
	if (m_pEditingAsset == nullptr)
	{
		return;
	}

	DrawWorldVisualization();

	if (E::CGameInstance::Get().IsCinematicPlaying())
	{
		return;
	}

	HandleViewportSelection();

	DrawSelectedKeyframeGizmo();
}

void CCinematicEditor::DrawWorldVisualization() const
{
	if (m_pEditingAsset == nullptr)
	{
		return;
	}

	auto* pLineRender =
		E::CGameInstance::Get().GetDbgLineRender();
	auto* pCamera =
		E::CGameInstance::Get().GetActiveCamera();
	ImGuiViewport* pViewport = ImGui::GetMainViewport();
	if (pLineRender == nullptr ||
		pCamera == nullptr ||
		pViewport == nullptr)
	{
		return;
	}

	const auto& Track =
		m_pEditingAsset->GetCameraTrack();
	ImDrawList* pDrawList =
		ImGui::GetBackgroundDrawList(pViewport);
	const _matrix matViewProjection =
		pCamera->GetView() * pCamera->GetProj();
	const _float fViewportAspect =
		pViewport->Size.y > FLT_EPSILON ?
		pViewport->Size.x / pViewport->Size.y :
		1.f;
	const _float4 vPreviousLineColor =
		pLineRender->GetColor();

	for (size_t iShot = 0;
		iShot < Track.Shots.size();
		++iShot)
	{
		const auto& Shot = Track.Shots[iShot];
		_matrix matCoordinateWorld = XMMatrixIdentity();
		if (Shot.eCoordinateSpace ==
			E::ECinematicCoordinateSpace::TargetLocal)
		{
			if (!TryGetPreviewTargetWorld(matCoordinateWorld))
			{
				continue;
			}
		}
		else if (Shot.eCoordinateSpace !=
			E::ECinematicCoordinateSpace::World)
		{
			continue;
		}

		const auto ToWorldPosition =
			[&matCoordinateWorld](const _float3& vLocalPosition)
			{
				_float3 vWorldPosition{};
				XMStoreFloat3(
					&vWorldPosition,
					XMVector3TransformCoord(
						XMLoadFloat3(&vLocalPosition),
						matCoordinateWorld));
				return vWorldPosition;
			};

		std::vector<const E::FCinematicCameraKeyframe*>
			SortedKeyframes{};
		SortedKeyframes.reserve(Shot.Keyframes.size());
		for (const auto& Keyframe : Shot.Keyframes)
		{
			SortedKeyframes.push_back(&Keyframe);
		}
		std::stable_sort(
			SortedKeyframes.begin(),
			SortedKeyframes.end(),
			[](const auto* pLeft, const auto* pRight)
			{
				return pLeft->fTime < pRight->fTime;
			});

		for (size_t i = 1;
			i < SortedKeyframes.size();
			++i)
		{
			const auto* pPreviousKeyframe =
				SortedKeyframes[i - 1];
			const auto* pNextKeyframe =
				SortedKeyframes[i];

			if (pPreviousKeyframe->ePositionInterpolation ==
				E::ECinematicInterpolation::Linear)
			{
				pLineRender->AddLine(
					ToWorldPosition(
						pPreviousKeyframe->vPosition),
					ToWorldPosition(
						pNextKeyframe->vPosition),
					_float4{ 0.2f, 0.85f, 1.f, 1.f });
				continue;
			}

			const auto* pP0 = i > 1 ?
				SortedKeyframes[i - 2] :
				pPreviousKeyframe;
			const auto* pP3 =
				i + 1 < SortedKeyframes.size() ?
				SortedKeyframes[i + 1] :
				pNextKeyframe;
			constexpr size_t iCurveSegmentCount = 16;
			_float3 vPreviousPosition =
				ToWorldPosition(
					pPreviousKeyframe->vPosition);

			for (size_t iSegment = 1;
				iSegment <= iCurveSegmentCount;
				++iSegment)
			{
				const _float fRatio =
					static_cast<_float>(iSegment) /
					static_cast<_float>(iCurveSegmentCount);
				_float3 vCurrentPosition{};
				XMStoreFloat3(
					&vCurrentPosition,
					XMVectorCatmullRom(
						XMLoadFloat3(&pP0->vPosition),
						XMLoadFloat3(
							&pPreviousKeyframe->vPosition),
						XMLoadFloat3(
							&pNextKeyframe->vPosition),
						XMLoadFloat3(&pP3->vPosition),
						fRatio));
				vCurrentPosition =
					ToWorldPosition(vCurrentPosition);

				pLineRender->AddLine(
					vPreviousPosition,
					vCurrentPosition,
					_float4{ 0.2f, 0.85f, 1.f, 1.f });
				vPreviousPosition = vCurrentPosition;
			}
		}

		for (size_t iKeyframe = 0;
			iKeyframe < Shot.Keyframes.size();
			++iKeyframe)
		{
			const auto& Keyframe =
				Shot.Keyframes[iKeyframe];
			const _bool bSelected =
				m_SelectedShotIndex == iShot &&
				m_SelectedKeyframeIndex == iKeyframe;
			const _float4 vColor = bSelected ?
				_float4{ 1.f, 0.85f, 0.1f, 1.f } :
				_float4{ 0.15f, 0.9f, 1.f, 1.f };
			const _float fCrossSize =
				bSelected ? 0.3f : 0.18f;
			const _float3 vPosition =
				ToWorldPosition(Keyframe.vPosition);

			pLineRender->AddLine(
				_float3{
					vPosition.x - fCrossSize,
					vPosition.y,
					vPosition.z },
				_float3{
					vPosition.x + fCrossSize,
					vPosition.y,
					vPosition.z },
				vColor);
			pLineRender->AddLine(
				_float3{
					vPosition.x,
					vPosition.y - fCrossSize,
					vPosition.z },
				_float3{
					vPosition.x,
					vPosition.y + fCrossSize,
					vPosition.z },
				vColor);
			pLineRender->AddLine(
				_float3{
					vPosition.x,
					vPosition.y,
					vPosition.z - fCrossSize },
				_float3{
					vPosition.x,
					vPosition.y,
					vPosition.z + fCrossSize },
				vColor);

			const _float fDisplayFar =
				m_fFrustumDisplaySize *
				(bSelected ? 1.35f : 1.f);
			const _float fDisplayNear =
				std::max(0.02f, fDisplayFar * 0.08f);
			const _float fDisplayFovY = std::clamp(
				Keyframe.fFovY,
				1.f,
				150.f);
			const _matrix matCameraWorld =
				(XMMatrixRotationQuaternion(
					LoadSafeQuaternion(Keyframe.vRotation)) *
				 XMMatrixTranslationFromVector(
					XMLoadFloat3(&Keyframe.vPosition))) *
				matCoordinateWorld;

			pLineRender->SetColor(vColor);
			pLineRender->AddFrustum(
				XMConvertToRadians(fDisplayFovY),
				fViewportAspect,
				fDisplayNear,
				fDisplayFar,
				matCameraWorld);

			ImVec2 vScreenPosition{};
			if (TryProjectToViewport(
				vPosition,
				matViewProjection,
				*pViewport,
				vScreenPosition))
			{
				pDrawList->AddCircleFilled(
					vScreenPosition,
					bSelected ? 7.f : 5.f,
					bSelected ?
						IM_COL32(255, 215, 30, 255) :
						IM_COL32(40, 220, 255, 230));
			}
		}
	}

	pLineRender->SetColor(vPreviousLineColor);
}

void CCinematicEditor::HandleViewportSelection()
{
	if (m_pEditingAsset == nullptr ||
		ImGui::GetIO().WantCaptureMouse ||
		!ImGui::IsMouseClicked(ImGuiMouseButton_Left) ||
		ImGuizmo::IsOver() ||
		ImGuizmo::IsUsing())
	{
		return;
	}

	auto* pCamera =
		E::CGameInstance::Get().GetActiveCamera();
	ImGuiViewport* pViewport =
		ImGui::GetMainViewport();
	if (pCamera == nullptr || pViewport == nullptr)
	{
		return;
	}

	const _matrix matViewProjection =
		pCamera->GetView() * pCamera->GetProj();
	const ImVec2 vMousePosition =
		ImGui::GetMousePos();

	_float fNearestDistanceSq = 12.f * 12.f;
	std::optional<size_t> SelectedShotIndex{};
	std::optional<size_t> SelectedKeyframeIndex{};

	const auto& Track =
		m_pEditingAsset->GetCameraTrack();
	for (size_t iShot = 0;
		iShot < Track.Shots.size();
		++iShot)
	{
		const auto& Shot = Track.Shots[iShot];

		for (size_t iKeyframe = 0;
			iKeyframe < Shot.Keyframes.size();
			++iKeyframe)
		{
			_matrix matKeyframeWorld{};
			if (!TryGetKeyframeWorld(
				Shot,
				Shot.Keyframes[iKeyframe],
				matKeyframeWorld))
			{
				continue;
			}

			_float3 vWorldPosition{};
			XMStoreFloat3(
				&vWorldPosition,
				matKeyframeWorld.r[3]);

			ImVec2 vScreenPosition{};
			if (!TryProjectToViewport(
				vWorldPosition,
				matViewProjection,
				*pViewport,
				vScreenPosition))
			{
				continue;
			}

			const _float fDeltaX =
				vMousePosition.x - vScreenPosition.x;
			const _float fDeltaY =
				vMousePosition.y - vScreenPosition.y;
			const _float fDistanceSq =
				fDeltaX * fDeltaX +
				fDeltaY * fDeltaY;
			if (fDistanceSq <= fNearestDistanceSq)
			{
				fNearestDistanceSq = fDistanceSq;
				SelectedShotIndex = iShot;
				SelectedKeyframeIndex = iKeyframe;
			}
		}
	}

	if (SelectedShotIndex.has_value())
	{
		m_SelectedShotIndex = SelectedShotIndex;
		m_SelectedKeyframeIndex =
			SelectedKeyframeIndex;
	}
}

void CCinematicEditor::DrawSelectedKeyframeGizmo()
{
	E::FCinematicCameraKeyframe* pKeyframe =
		GetSelectedKeyframe();
	auto* pCamera =
		E::CGameInstance::Get().GetActiveCamera();
	ImGuiViewport* pViewport =
		ImGui::GetMainViewport();
	if (pKeyframe == nullptr ||
		pCamera == nullptr ||
		pViewport == nullptr)
	{
		return;
	}

	const auto& Shot =
		m_pEditingAsset->GetCameraTrack().Shots[
			*m_SelectedShotIndex];
	_matrix matKeyframeWorld{};
	if (!TryGetKeyframeWorld(
		Shot,
		*pKeyframe,
		matKeyframeWorld))
	{
		return;
	}

	_float4x4 matView{};
	_float4x4 matProjection{};
	_float4x4 matWorld{};
	XMStoreFloat4x4(
		&matView,
		pCamera->GetView());
	XMStoreFloat4x4(
		&matProjection,
		pCamera->GetProj());
	XMStoreFloat4x4(
		&matWorld,
		matKeyframeWorld);

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist(
		ImGui::GetForegroundDrawList(pViewport));
	ImGuizmo::SetRect(
		pViewport->Pos.x,
		pViewport->Pos.y,
		pViewport->Size.x,
		pViewport->Size.y);
	ImGuizmo::SetID(0x43494E45);

	if (!ImGuizmo::Manipulate(
		&matView._11,
		&matProjection._11,
		m_GizmoOperation,
		m_GizmoOperation == ImGuizmo::ROTATE ?
			ImGuizmo::LOCAL :
			ImGuizmo::WORLD,
		&matWorld._11))
	{
		return;
	}

	_vector vScale{};
	_vector vRotation{};
	_vector vTranslation{};
	_matrix matStoredKeyframe =
		XMLoadFloat4x4(&matWorld);
	if (Shot.eCoordinateSpace ==
		E::ECinematicCoordinateSpace::TargetLocal)
	{
		_matrix matTargetWorld{};
		if (!TryGetPreviewTargetWorld(matTargetWorld))
		{
			return;
		}

		_vector vDeterminant{};
		const _matrix matTargetInverse =
			XMMatrixInverse(
				&vDeterminant,
				matTargetWorld);
		if (std::abs(XMVectorGetX(vDeterminant)) <=
			FLT_EPSILON)
		{
			return;
		}

		matStoredKeyframe *= matTargetInverse;
	}

	if (XMMatrixDecompose(
		&vScale,
		&vRotation,
		&vTranslation,
		matStoredKeyframe))
	{
		XMStoreFloat3(
			&pKeyframe->vPosition,
			vTranslation);
		XMStoreFloat4(
			&pKeyframe->vRotation,
			XMQuaternionNormalize(vRotation));
		m_pEditingAsset->RecalculateDuration();
	}
}

E::FCinematicCameraKeyframe* CCinematicEditor::GetSelectedKeyframe()
{
	return const_cast<E::FCinematicCameraKeyframe*>(
		static_cast<const CCinematicEditor*>(this)
			->GetSelectedKeyframe());
}

const E::FCinematicCameraKeyframe* CCinematicEditor::GetSelectedKeyframe() const
{
	if (m_pEditingAsset == nullptr ||
		!m_SelectedShotIndex.has_value() ||
		!m_SelectedKeyframeIndex.has_value())
	{
		return nullptr;
	}

	const auto& Track =
		m_pEditingAsset->GetCameraTrack();
	if (*m_SelectedShotIndex >= Track.Shots.size())
	{
		return nullptr;
	}

	const auto& Shot =
		Track.Shots[*m_SelectedShotIndex];
	if (*m_SelectedKeyframeIndex >=
			Shot.Keyframes.size())
	{
		return nullptr;
	}

	return &Shot.Keyframes[
		*m_SelectedKeyframeIndex];
}

_bool CCinematicEditor::HasTargetLocalShot() const
{
	if (m_pEditingAsset == nullptr)
	{
		return false;
	}

	const auto& Shots =
		m_pEditingAsset->GetCameraTrack().Shots;
	return std::any_of(
		Shots.begin(),
		Shots.end(),
		[](const E::FCinematicCameraShot& Shot)
		{
			return Shot.eCoordinateSpace ==
				E::ECinematicCoordinateSpace::TargetLocal;
		});
}

_bool CCinematicEditor::TryGetPreviewTargetWorld(
	_matrix& OutTargetWorld) const
{
	if (!m_PreviewTargetHandle.has_value())
	{
		return false;
	}

	const E::CGameObject* pTarget =
		E::CGameInstance::Get().GetGameObjectByHandle(
			*m_PreviewTargetHandle);
	if (pTarget == nullptr ||
		pTarget->GetPendingDestroy())
	{
		return false;
	}

	_vector vScale{};
	_vector vRotation{};
	_vector vTranslation{};
	if (!XMMatrixDecompose(
		&vScale,
		&vRotation,
		&vTranslation,
		pTarget->GetTransform()
			.GetLoadedCombinedWorldMatrix()))
	{
		return false;
	}

	OutTargetWorld =
		XMMatrixRotationQuaternion(
			XMQuaternionNormalize(vRotation)) *
		XMMatrixTranslationFromVector(vTranslation);
	return true;
}

_bool CCinematicEditor::TryGetKeyframeWorld(
	const E::FCinematicCameraShot& Shot,
	const E::FCinematicCameraKeyframe& Keyframe,
	_matrix& OutKeyframeWorld) const
{
	OutKeyframeWorld =
		XMMatrixRotationQuaternion(
			LoadSafeQuaternion(Keyframe.vRotation)) *
		XMMatrixTranslationFromVector(
			XMLoadFloat3(&Keyframe.vPosition));

	if (Shot.eCoordinateSpace ==
		E::ECinematicCoordinateSpace::World)
	{
		return true;
	}

	if (Shot.eCoordinateSpace !=
		E::ECinematicCoordinateSpace::TargetLocal)
	{
		return false;
	}

	_matrix matTargetWorld{};
	if (!TryGetPreviewTargetWorld(matTargetWorld))
	{
		return false;
	}

	OutKeyframeWorld *= matTargetWorld;
	return true;
}

E::UPtr<CCinematicEditor> CCinematicEditor::Create()
{
	return E::ToUPtr(new CCinematicEditor{});
}
