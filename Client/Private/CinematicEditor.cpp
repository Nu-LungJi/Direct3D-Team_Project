#include "pch.h"
#include "CinematicEditor.h"

#include "CinematicAsset.h"
#include "CinematicTypes.h"
#include "GameInstance.h"

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
		const HRESULT hr = E::CGameInstance::Get().PlayCinematic(
			m_pEditingAsset->GetCinematicID());
		m_Status = hr == S_OK ?
			"Playback started." :
			"Playback failed. Register the asset and check its data.";
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

	for (size_t i = 0; i < Track.Shots.size(); ++i)
	{
		ImGui::PushID(static_cast<int>(i));

		if (ImGui::Button("Remove Shot"))
		{
			RemoveShotIndex = i;
		}

		DrawShot(Track.Shots[i], i, bChanged);
		ImGui::PopID();

		if (RemoveShotIndex.has_value())
		{
			break;
		}
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
	const E::FCinematicAssetData Data =
		m_pEditingAsset->ExportData();
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

	strncpy_s(
		m_szCinematicID,
		m_pEditingAsset->GetCinematicID().GetDbgStr(),
		_TRUNCATE);
	m_Status =
		"Loaded: " + LoadPath.generic_string();
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
	Shot.eBindingMode = E::ECinematicBindingMode::Snapshot;

	E::FCinematicCameraKeyframe Keyframe{};
	Keyframe.fTime = 0.f;
	Shot.Keyframes.push_back(Keyframe);

	Track.Shots.push_back(std::move(Shot));
	m_SelectedShotIndex = Track.Shots.size() - 1;
	m_SelectedKeyframeIndex = 0;
}

void CCinematicEditor::DrawShot(
	E::FCinematicCameraShot& Shot,
	size_t iShotIndex,
	_bool& bChanged)
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

	static const _char* CoordinateSpaceNames[] = {
		"World",
		"TargetLocal (Not Implemented)"
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

	static const _char* BindingModeNames[] = {
		"Live",
		"Snapshot"
	};
	int iBindingMode = static_cast<int>(Shot.eBindingMode);
	if (ImGui::Combo(
		"Binding Mode",
		&iBindingMode,
		BindingModeNames,
		IM_ARRAYSIZE(BindingModeNames)))
	{
		Shot.eBindingMode =
			static_cast<E::ECinematicBindingMode>(iBindingMode);
		bChanged = true;
	}

	if (Shot.eCoordinateSpace ==
		E::ECinematicCoordinateSpace::TargetLocal)
	{
		ImGui::TextColored(
			ImVec4{ 1.f, 0.75f, 0.25f, 1.f },
			"TargetLocal playback is not implemented yet.");
	}

	std::optional<size_t> RemoveKeyframeIndex{};
	for (size_t i = 0; i < Shot.Keyframes.size(); ++i)
	{
		ImGui::PushID(static_cast<int>(i));

		_bool bRemove = false;
		DrawKeyframe(
			Shot.Keyframes[i],
			iShotIndex,
			i,
			bChanged,
			bRemove);
		if (bRemove)
		{
			RemoveKeyframeIndex = i;
		}

		ImGui::PopID();
		if (RemoveKeyframeIndex.has_value())
		{
			break;
		}
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
	_bool& bRemove)
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
		"CatmullRom (Not Implemented)"
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

	if (Keyframe.ePositionInterpolation ==
		E::ECinematicInterpolation::CatmullRom)
	{
		ImGui::TextColored(
			ImVec4{ 1.f, 0.75f, 0.25f, 1.f },
			"CatmullRom playback is not implemented yet.");
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
		if (Shot.eCoordinateSpace !=
			E::ECinematicCoordinateSpace::World)
		{
			continue;
		}

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
			pLineRender->AddLine(
				SortedKeyframes[i - 1]->vPosition,
				SortedKeyframes[i]->vPosition,
				_float4{ 0.2f, 0.85f, 1.f, 1.f });
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
			const _float3& vPosition =
				Keyframe.vPosition;

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
				XMMatrixRotationQuaternion(
					LoadSafeQuaternion(Keyframe.vRotation)) *
				XMMatrixTranslation(
					vPosition.x,
					vPosition.y,
					vPosition.z);

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
		if (Shot.eCoordinateSpace !=
			E::ECinematicCoordinateSpace::World)
		{
			continue;
		}

		for (size_t iKeyframe = 0;
			iKeyframe < Shot.Keyframes.size();
			++iKeyframe)
		{
			ImVec2 vScreenPosition{};
			if (!TryProjectToViewport(
				Shot.Keyframes[iKeyframe].vPosition,
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
		XMMatrixAffineTransformation(
			XMVectorSet(1.f, 1.f, 1.f, 0.f),
			XMVectorZero(),
			LoadSafeQuaternion(pKeyframe->vRotation),
			XMLoadFloat3(&pKeyframe->vPosition)));

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
	if (XMMatrixDecompose(
		&vScale,
		&vRotation,
		&vTranslation,
		XMLoadFloat4x4(&matWorld)))
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
	if (Shot.eCoordinateSpace !=
			E::ECinematicCoordinateSpace::World ||
		*m_SelectedKeyframeIndex >=
			Shot.Keyframes.size())
	{
		return nullptr;
	}

	return &Shot.Keyframes[
		*m_SelectedKeyframeIndex];
}

E::UPtr<CCinematicEditor> CCinematicEditor::Create()
{
	return E::ToUPtr(new CCinematicEditor{});
}
