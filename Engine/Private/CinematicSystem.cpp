#include "pch.h"
#include "CinematicSystem.h"
#include "CameraManager.h"
#include "CinematicCamera.h"
#include "CinematicAsset.h"
#include "GameInstance.h"

NS_USING(Engine)

namespace
{
	constexpr const _char* CINEMATIC_LOAD_ROOT =
		"./Resources/json/Cinematics";
	constexpr const _char* CINEMATIC_JSON_ROOT =
		"Cinematic";

	_bool IsValidCinematicName(const std::string& CinematicName)
	{
		if (CinematicName.empty() ||
			CinematicName == "." ||
			CinematicName == ".." ||
			CinematicName.find_first_of("<>:\"/\\|?*") !=
				std::string::npos ||
			static_cast<unsigned char>(CinematicName.back()) <= 0x20 ||
			CinematicName.back() == '.')
		{
			return false;
		}

		return std::none_of(
			CinematicName.begin(),
			CinematicName.end(),
			[](_char Character)
			{
				return static_cast<unsigned char>(Character) < 0x20;
			});
	}
}

CCinematicSystem::CCinematicSystem(CCameraManager& CameraManager)
	: m_CameraManager{CameraManager}
{
}

CCinematicSystem::~CCinematicSystem()
{

}

HRESULT CCinematicSystem::Initialize(const StringID& CinematicCameraID)
{
	m_CinematicCameraID = CinematicCameraID;

	return S_OK;
}

void CCinematicSystem::Update(_float fTimeDelta)
{
	if (!m_bPlaying || m_pPlayingAsset == nullptr)
	{
		return;
	}

	if (fTimeDelta > 0.f)
	{
		m_fPlayTime += fTimeDelta;
	}

	const _float fDuration = m_pPlayingAsset->GetDuration();
	m_fPlayTime = std::min(m_fPlayTime, fDuration);

	FCinematicCameraPose Pose{};
	if (FAILED(EvaluateCamera(m_fPlayTime, Pose)) || FAILED(ApplyCameraPose(Pose)))
	{
		Stop();
		return;
	}

	if (m_fPlayTime >= fDuration)
	{
		Stop();
	}
}

HRESULT CCinematicSystem::BeginCameraControl()
{
	if (m_PreviousCameraID.has_value())
	{
		return S_FALSE;
	}

	const auto ActiveCameraID = m_CameraManager.GetActiveCameraID();
	if (!ActiveCameraID.has_value() || *ActiveCameraID == m_CinematicCameraID)
	{
		return E_FAIL;
	}

	if (m_CameraManager.GetCamera(m_CinematicCameraID) == nullptr)
	{
		return E_FAIL;
	}

	m_PreviousCameraID = *ActiveCameraID;
	if (FAILED(m_CameraManager.SetActiveCamera(m_CinematicCameraID)))
	{
		m_PreviousCameraID.reset();
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CCinematicSystem::EndCameraControl()
{
	if (!m_PreviousCameraID.has_value())
	{
		return S_FALSE;
	}

	if (FAILED(m_CameraManager.SetActiveCamera(*m_PreviousCameraID)))
	{
		return E_FAIL;
	}

	m_PreviousCameraID.reset();
	return S_OK;
}

_bool CCinematicSystem::HasCameraControl() const
{
	if (!m_PreviousCameraID.has_value())
	{
		return false;
	}

	const auto ActiveCameraID = m_CameraManager.GetActiveCameraID();
	return ActiveCameraID.has_value() && *ActiveCameraID == m_CinematicCameraID;
}

HRESULT CCinematicSystem::RegistAsset(const SPtr<CCinematicAsset>& pAsset)
{
	if (pAsset == nullptr)
	{
		return E_INVALIDARG;
	}

	m_Assets.insert_or_assign(pAsset->GetCinematicID(), pAsset);
	return S_OK;
}

HRESULT CCinematicSystem::Load(const std::string& CinematicName)
{
	if (!IsValidCinematicName(CinematicName))
	{
		return E_INVALIDARG;
	}

	const std::filesystem::path FilePath = std::filesystem::path{ CINEMATIC_LOAD_ROOT } / (CinematicName + ".json");

	FCinematicAssetData Data{};
	const SERIALIZE_RESULT Result = CGameInstance::Get().JsonDeSerializeDetailed(FilePath.generic_string(), Data, CINEMATIC_JSON_ROOT);
	if (Result.Failed())
	{
		return Result.hResult;
	}

	const StringID RequestedID{ CinematicName };
	if (Data.CinematicID.hash == 0 || Data.CinematicID != RequestedID)
	{
		return E_FAIL;
	}

	auto pAsset = CCinematicAsset::Create(Data);
	if (pAsset == nullptr)
	{
		return E_FAIL;
	}

	return RegistAsset(pAsset);
}

HRESULT CCinematicSystem::Play(const StringID& CinematicID)
{
	if (m_bPlaying)
	{
		return S_FALSE;
	}

	auto iter = m_Assets.find(CinematicID);
	if (iter == m_Assets.end() || iter->second == nullptr)
	{
		MSG_BOX("No such cinematic ID exists");
		return E_FAIL;
	}

	iter->second->RecalculateDuration();
	if (iter->second->GetDuration() <= 0.f)
	{
		return E_FAIL;
	}

	const HRESULT hr = BeginCameraControl();
	if (hr != S_OK)
	{
		return hr;
	}

	m_pPlayingAsset = iter->second;
	m_bPlaying = true;
	m_fPlayTime = 0.f;

	FCinematicCameraPose Pose{};
	if (FAILED(EvaluateCamera(0.f, Pose)) || FAILED(ApplyCameraPose(Pose)))
	{
		Stop();
		return E_FAIL;
	}

	return S_OK;
}

void CCinematicSystem::Stop()
{
	if (!m_bPlaying)
	{
		return;
	}

	EndCameraControl();

	m_bPlaying = false;
	m_fPlayTime = 0.f;
	m_pPlayingAsset.reset();
}

_bool CCinematicSystem::IsPlaying() const
{
	return m_bPlaying;
}

_float CCinematicSystem::GetPlayTime() const
{
	return m_fPlayTime;
}

HRESULT CCinematicSystem::EvaluateCamera(_float fPlayTime, FCinematicCameraPose& OutPose) const
{
	if (m_pPlayingAsset == nullptr || !std::isfinite(fPlayTime))
	{
		return E_FAIL;
	}

	const FCinematicCameraShot* pShot = FindActiveShot(fPlayTime);
	if (pShot == nullptr)
	{
		return E_FAIL;
	}

	if (pShot->eCoordinateSpace != ECinematicCoordinateSpace::World)
	{
		return E_NOTIMPL;
	}

	return EvaluateShot(*pShot, fPlayTime - pShot->fStartTime, OutPose);
}

const FCinematicCameraShot* CCinematicSystem::FindActiveShot(_float fPlayTime) const
{
	if (m_pPlayingAsset == nullptr)
	{
		return nullptr;
	}

	const FCinematicCameraShot* pActiveShot = nullptr;
	const auto& Track = m_pPlayingAsset->GetCameraTrack();

	for (const auto& Shot : Track.Shots)
	{
		if (Shot.fStartTime <= fPlayTime &&
			(pActiveShot == nullptr || Shot.fStartTime >= pActiveShot->fStartTime))
		{
			pActiveShot = &Shot;
		}
	}

	return pActiveShot;
}

HRESULT CCinematicSystem::EvaluateShot(const FCinematicCameraShot& Shot, _float fShotTime, FCinematicCameraPose& OutPose) const
{
	if (Shot.Keyframes.empty() || !std::isfinite(fShotTime))
	{
		return E_FAIL;
	}

	const FCinematicCameraKeyframe* pPreviousKeyframe = nullptr;
	const FCinematicCameraKeyframe* pNextKeyframe = nullptr;

	for (const auto& Keyframe : Shot.Keyframes)
	{
		if (Keyframe.fTime <= fShotTime &&
			(pPreviousKeyframe == nullptr ||
			 Keyframe.fTime >= pPreviousKeyframe->fTime))
		{
			pPreviousKeyframe = &Keyframe;
		}

		if (Keyframe.fTime > fShotTime &&
			(pNextKeyframe == nullptr ||
			 Keyframe.fTime < pNextKeyframe->fTime))
		{
			pNextKeyframe = &Keyframe;
		}
	}

	const FCinematicCameraKeyframe* pKeyframe = pPreviousKeyframe;
	if (pKeyframe == nullptr)
	{
		pKeyframe = pNextKeyframe;
	}

	if (pKeyframe == nullptr)
	{
		return E_FAIL;
	}

	if (pPreviousKeyframe == nullptr || pNextKeyframe == nullptr)
	{
		OutPose.vPosition = pKeyframe->vPosition;
		OutPose.vRotation = pKeyframe->vRotation;
		OutPose.fFovY = pKeyframe->fFovY;
		return S_OK;
	}

	const _float fKeyframeDuration = pNextKeyframe->fTime - pPreviousKeyframe->fTime;
	if (fKeyframeDuration <= FLT_EPSILON)
	{
		return E_FAIL;
	}

	const _float fRatio = std::clamp((fShotTime - pPreviousKeyframe->fTime) / fKeyframeDuration, 0.f, 1.f);

	switch (pPreviousKeyframe->ePositionInterpolation)
	{
		case ECinematicInterpolation::Linear:
			return LinearInterpolate(pPreviousKeyframe, pNextKeyframe, fRatio, OutPose);

		case ECinematicInterpolation::CatmullRom:
		{
			const size_t iPreviousIndex = static_cast<size_t>(pPreviousKeyframe - Shot.Keyframes.data());
			const size_t iNextIndex = static_cast<size_t>(pNextKeyframe - Shot.Keyframes.data());

			const auto& P0 = iPreviousIndex > 0 ? Shot.Keyframes[iPreviousIndex - 1] : *pPreviousKeyframe;
			const auto& P3 = iNextIndex + 1 < Shot.Keyframes.size() ? Shot.Keyframes[iNextIndex + 1] : *pNextKeyframe;

			return CatMullRomInterpolate(P0, *pPreviousKeyframe, *pNextKeyframe, P3, fRatio, OutPose);
		}
		default:
			return E_INVALIDARG;
	}
}

HRESULT CCinematicSystem::LinearInterpolate(const FCinematicCameraKeyframe* prevKeyFrame, const FCinematicCameraKeyframe* nextKeyFrame, const _float fRatio, FCinematicCameraPose& OutPose) const
{
	XMStoreFloat3(&OutPose.vPosition, XMVectorLerp(XMLoadFloat3(&prevKeyFrame->vPosition), XMLoadFloat3(&nextKeyFrame->vPosition), fRatio));

	const _vector vPreviousRotation = XMLoadFloat4(&prevKeyFrame->vRotation);
	const _vector vNextRotation = XMLoadFloat4(&nextKeyFrame->vRotation);

	_float fPreviousRotationLengthSq{};
	_float fNextRotationLengthSq{};

	XMStoreFloat(&fPreviousRotationLengthSq, XMVector4LengthSq(vPreviousRotation));
	XMStoreFloat(&fNextRotationLengthSq, XMVector4LengthSq(vNextRotation));

	if (fPreviousRotationLengthSq <= FLT_EPSILON || fNextRotationLengthSq <= FLT_EPSILON)
	{
		return E_INVALIDARG;
	}

	XMStoreFloat4(&OutPose.vRotation, XMQuaternionSlerp(XMQuaternionNormalize(vPreviousRotation), XMQuaternionNormalize(vNextRotation), fRatio));

	OutPose.fFovY = prevKeyFrame->fFovY + (nextKeyFrame->fFovY - prevKeyFrame->fFovY) * fRatio;

	return S_OK;
}

HRESULT CCinematicSystem::CatMullRomInterpolate(const FCinematicCameraKeyframe& p0, const FCinematicCameraKeyframe& p1, const FCinematicCameraKeyframe& p2, const FCinematicCameraKeyframe& p3, const _float fRatio, FCinematicCameraPose& OutPose) const
{
	const HRESULT hr = LinearInterpolate(&p1, &p2, fRatio, OutPose);
	if (FAILED(hr))
	{
		return hr;
	}

	XMStoreFloat3(&OutPose.vPosition, XMVectorCatmullRom(
			XMLoadFloat3(&p0.vPosition),
			XMLoadFloat3(&p1.vPosition),
			XMLoadFloat3(&p2.vPosition),
			XMLoadFloat3(&p3.vPosition),
			fRatio));

	return S_OK;
}


HRESULT CCinematicSystem::ApplyCameraPose(const FCinematicCameraPose& Pose)
{
	CCinematicCamera* pCinematicCam =Cast<CCinematicCamera>(m_CameraManager.GetActiveCamera(m_CinematicCameraID));
	if (pCinematicCam == nullptr)
	{
		return E_FAIL;
	}

	return pCinematicCam->ApplyPose(Pose.vPosition, Pose.vRotation, Pose.fFovY);
}


UPtr<CCinematicSystem> CCinematicSystem::Create(CCameraManager& CameraManager, const StringID& CinematicCameraID)
{
	auto pInstance = ToUPtr(new CCinematicSystem(CameraManager));
	if (FAILED(pInstance->Initialize(CinematicCameraID)))
	{
		return nullptr;
	}
	return pInstance;
}
