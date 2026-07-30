#include "pch.h"
#include "CinematicSystem.h"
#include "CameraManager.h"
#include "CinematicCamera.h"
#include "CinematicAsset.h"
#include "GameInstance.h"
#include "GameObject.h"

NS_USING(Engine)

namespace
{
	constexpr const _char* CINEMATIC_LOAD_ROOT = "./Resources/json/Cinematics";
	constexpr const _char* CINEMATIC_JSON_ROOT = "Cinematic";

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
	if (m_ePlayState == EPlayState::Stopped)
	{
		return;
	}

	if (m_ePlayState == EPlayState::BlendingIn)
	{
		if (FAILED(UpdateBlendIn(fTimeDelta)))
		{
			Stop();
		}
		return;
	}

	if (m_ePlayState == EPlayState::BlendingOut)
	{
		if (FAILED(UpdateBlendOut(fTimeDelta)))
		{
			Stop();
		}
		return;
	}

	if (m_pPlayingAsset == nullptr)
	{
		Stop();
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
		if (m_PlayOptions.eReturnMode ==
				ECinematicReturnMode::Blend &&
			m_PlayOptions.fReturnBlendDuration >
				FLT_EPSILON)
		{
			if (FAILED(BeginBlendOut(Pose)))
			{
				Stop();
			}
		}
		else
		{
			Stop();
		}
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

HRESULT CCinematicSystem::Play(const StringID& CinematicID, const FCinematicPlayOptions& Options)
{
	return Play(CinematicID, std::nullopt, Options);
}

HRESULT CCinematicSystem::Play(const StringID& CinematicID, const CHandle& TargetHandle, const FCinematicPlayOptions& Options)
{
	return Play(CinematicID, std::optional<CHandle>{ TargetHandle }, Options);
}

HRESULT CCinematicSystem::Play(const StringID& CinematicID, const std::optional<CHandle>& TargetHandle, const FCinematicPlayOptions& Options)
{
	if (m_ePlayState != EPlayState::Stopped)
	{
		return S_FALSE;
	}

	if (!std::isfinite(Options.fStartBlendDuration) || Options.fStartBlendDuration < 0.f ||
		(Options.eStartMode != ECinematicStartMode::Immediate &&
		 Options.eStartMode != ECinematicStartMode::Blend) ||
		!std::isfinite(Options.fReturnBlendDuration) || Options.fReturnBlendDuration < 0.f ||
		(Options.eReturnMode != ECinematicReturnMode::Immediate && Options.eReturnMode !=ECinematicReturnMode::Blend))
	{
		return E_INVALIDARG;
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

	const auto& Shots = iter->second->GetCameraTrack().Shots;
	const _bool bRequiresTarget = std::any_of(Shots.begin(),Shots.end(),
		[](const FCinematicCameraShot& Shot)
		{
			return Shot.eCoordinateSpace == ECinematicCoordinateSpace::TargetLocal;
		});

	if (bRequiresTarget && (!TargetHandle.has_value() || CGameInstance::Get().GetGameObjectByHandle(*TargetHandle) == nullptr))
	{
		return E_INVALIDARG;
	}

	const HRESULT hr = BeginCameraControl();
	if (hr != S_OK)
	{
		return hr;
	}

	m_pPlayingAsset = iter->second;
	m_TargetHandle = TargetHandle;
	m_PlayOptions = Options;
	m_fPlayTime = 0.f;
	m_fStartBlendTime = 0.f;
	m_fReturnBlendTime = 0.f;

	if (Options.eStartMode == ECinematicStartMode::Blend &&Options.fStartBlendDuration > FLT_EPSILON)
	{
		if (FAILED(BeginBlendIn()))
		{
			Stop();
			return E_FAIL;
		}
	}
	else
	{
		m_ePlayState = EPlayState::Playing;

		FCinematicCameraPose Pose{};
		if (FAILED(EvaluateCamera(0.f, Pose)) ||
			FAILED(ApplyCameraPose(Pose)))
		{
			Stop();
			return E_FAIL;
		}
	}

	return S_OK;
}

void CCinematicSystem::Stop()
{
	if (m_ePlayState == EPlayState::Stopped &&
		!m_PreviousCameraID.has_value())
	{
		return;
	}

	FinishPlayback();
}

void CCinematicSystem::FinishPlayback()
{
	EndCameraControl();
	m_PreviousCameraID.reset();

	m_ePlayState = EPlayState::Stopped;
	m_fPlayTime = 0.f;
	m_fStartBlendTime = 0.f;
	m_fReturnBlendTime = 0.f;
	m_pPlayingAsset.reset();
	m_TargetHandle.reset();
	m_PlayOptions = {};
	m_BlendStartPose = {};
}

_bool CCinematicSystem::IsPlaying() const
{
	return m_ePlayState != EPlayState::Stopped;
}

_float CCinematicSystem::GetPlayTime() const
{
	return m_fPlayTime;
}

HRESULT CCinematicSystem::BeginBlendIn()
{
	if (!m_PreviousCameraID.has_value())
	{
		return E_FAIL;
	}

	if (FAILED(GetCameraPose(m_CameraManager.GetCamera(*m_PreviousCameraID), m_BlendStartPose)))
	{
		return E_FAIL;
	}

	if (FAILED(ApplyCameraPose(m_BlendStartPose)))
	{
		return E_FAIL;
	}

	m_fStartBlendTime = 0.f;
	m_ePlayState = EPlayState::BlendingIn;
	return S_OK;
}

HRESULT CCinematicSystem::UpdateBlendIn(_float fTimeDelta)
{
	if (m_pPlayingAsset == nullptr || m_PlayOptions.fStartBlendDuration <= FLT_EPSILON)
	{
		return E_FAIL;
	}

	if (fTimeDelta > 0.f)
	{
		m_fStartBlendTime += fTimeDelta;
	}

	FCinematicCameraPose CinematicStartPose{};
	if (FAILED(EvaluateCamera(0.f, CinematicStartPose)))
	{
		return E_FAIL;
	}

	_float fRatio = std::clamp(m_fStartBlendTime / m_PlayOptions.fStartBlendDuration, 0.f, 1.f);
	fRatio = fRatio * fRatio * (3.f - 2.f * fRatio);

	FCinematicCameraPose BlendedPose{};
	XMStoreFloat3(&BlendedPose.vPosition, XMVectorLerp(XMLoadFloat3(&m_BlendStartPose.vPosition), XMLoadFloat3(&CinematicStartPose.vPosition), fRatio));
	XMStoreFloat4(&BlendedPose.vRotation,
		XMQuaternionSlerp(XMQuaternionNormalize(XMLoadFloat4(&m_BlendStartPose.vRotation)),XMQuaternionNormalize(XMLoadFloat4(&CinematicStartPose.vRotation)),fRatio));
	BlendedPose.fFovY = m_BlendStartPose.fFovY + (CinematicStartPose.fFovY - m_BlendStartPose.fFovY) * fRatio;

	if (FAILED(ApplyCameraPose(BlendedPose)))
	{
		return E_FAIL;
	}

	if (m_fStartBlendTime >= m_PlayOptions.fStartBlendDuration)
	{
		m_fStartBlendTime = m_PlayOptions.fStartBlendDuration;
		m_fPlayTime = 0.f;
		m_ePlayState = EPlayState::Playing;
	}

	return S_OK;
}

HRESULT CCinematicSystem::BeginBlendOut(const FCinematicCameraPose& StartPose)
{
	if (!m_PreviousCameraID.has_value() ||m_CameraManager.GetCamera(*m_PreviousCameraID) == nullptr)
	{
		return E_FAIL;
	}

	m_BlendStartPose = StartPose;
	m_fReturnBlendTime = 0.f;
	m_ePlayState = EPlayState::BlendingOut;
	return S_OK;
}

HRESULT CCinematicSystem::UpdateBlendOut(_float fTimeDelta)
{
	if (!m_PreviousCameraID.has_value() || m_PlayOptions.fReturnBlendDuration <= FLT_EPSILON)
	{
		return E_FAIL;
	}

	if (fTimeDelta > 0.f)
	{
		m_fReturnBlendTime += fTimeDelta;
	}

	FCinematicCameraPose ReturnPose{};
	if (FAILED(GetCameraPose(m_CameraManager.GetCamera(*m_PreviousCameraID), ReturnPose)))
	{
		return E_FAIL;
	}

	_float fRatio = std::clamp(m_fReturnBlendTime / m_PlayOptions.fReturnBlendDuration, 0.f, 1.f);
	fRatio = fRatio * fRatio * (3.f - 2.f * fRatio);

	FCinematicCameraPose BlendedPose{};
	XMStoreFloat3(&BlendedPose.vPosition, XMVectorLerp(
			XMLoadFloat3(&m_BlendStartPose.vPosition),
			XMLoadFloat3(&ReturnPose.vPosition),
			fRatio));
	XMStoreFloat4(&BlendedPose.vRotation, XMQuaternionSlerp(
			XMQuaternionNormalize(XMLoadFloat4(&m_BlendStartPose.vRotation)),
			XMQuaternionNormalize(XMLoadFloat4(&ReturnPose.vRotation)),
		fRatio));
	BlendedPose.fFovY = m_BlendStartPose.fFovY + (ReturnPose.fFovY - m_BlendStartPose.fFovY) * fRatio;

	if (FAILED(ApplyCameraPose(BlendedPose)))
	{
		return E_FAIL;
	}

	if (m_fReturnBlendTime >= m_PlayOptions.fReturnBlendDuration)
	{
		FinishPlayback();
	}

	return S_OK;
}

HRESULT CCinematicSystem::EvaluateCamera(_float fPlayTime, FCinematicCameraPose& OutPose)
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

	FCinematicCameraPose LocalPose{};
	const HRESULT hr = EvaluateShot(*pShot, fPlayTime - pShot->fStartTime, LocalPose);
	if (FAILED(hr))
	{
		return hr;
	}

	if (pShot->eCoordinateSpace == ECinematicCoordinateSpace::World)
	{
		OutPose = LocalPose;
		return S_OK;
	}

	if (pShot->eCoordinateSpace != ECinematicCoordinateSpace::TargetLocal)
	{
		return E_INVALIDARG;
	}

	_matrix TargetWorld{};
	if (FAILED(GetTargetWorldMatrix(TargetWorld)))
	{
		return E_FAIL;
	}

	return ConvertTargetLocalPose(LocalPose, TargetWorld, OutPose);
}

HRESULT CCinematicSystem::GetTargetWorldMatrix(_matrix& OutTargetWorld) const
{
	if (!m_TargetHandle.has_value())
	{
		return E_FAIL;
	}

	const CGameObject* pTarget = CGameInstance::Get().GetGameObjectByHandle(*m_TargetHandle);
	if (pTarget == nullptr)
	{
		return E_FAIL;
	}

	_vector vScale{};
	_vector vRotation{};
	_vector vTranslation{};
	if (!XMMatrixDecompose(&vScale, &vRotation, &vTranslation, pTarget->GetTransform().GetLoadedCombinedWorldMatrix()))
	{
		return E_FAIL;
	}

	OutTargetWorld = XMMatrixRotationQuaternion(XMQuaternionNormalize(vRotation)) * XMMatrixTranslationFromVector(vTranslation);

	return S_OK;
}

HRESULT CCinematicSystem::ConvertTargetLocalPose(const FCinematicCameraPose& LocalPose, _fmatrix TargetWorld, FCinematicCameraPose& OutWorldPose) const
{
	const _vector vLocalRotation = XMLoadFloat4(&LocalPose.vRotation);

	_float fRotationLengthSq{};
	XMStoreFloat(&fRotationLengthSq, XMVector4LengthSq(vLocalRotation));
	if (fRotationLengthSq <= FLT_EPSILON)
	{
		return E_INVALIDARG;
	}

	const _matrix LocalWorld = XMMatrixRotationQuaternion(XMQuaternionNormalize(vLocalRotation)) *
		XMMatrixTranslationFromVector(XMLoadFloat3(&LocalPose.vPosition));
	const _matrix CameraWorld = LocalWorld * TargetWorld;

	_vector vScale{};
	_vector vRotation{};
	_vector vTranslation{};
	if (!XMMatrixDecompose(&vScale, &vRotation, &vTranslation, CameraWorld))
	{
		return E_FAIL;
	}

	XMStoreFloat3(&OutWorldPose.vPosition, vTranslation);
	XMStoreFloat4(&OutWorldPose.vRotation, XMQuaternionNormalize(vRotation));
	OutWorldPose.fFovY = LocalPose.fFovY;

	return S_OK;
}

HRESULT CCinematicSystem::GetCameraPose(const CCameraObject* pCamera, FCinematicCameraPose& OutPose) const
{
	if (pCamera == nullptr ||
		!std::isfinite(pCamera->GetFovY()) ||
		pCamera->GetFovY() <= 0.f ||
		pCamera->GetFovY() >= 180.f)
	{
		return E_FAIL;
	}

	_vector vScale{};
	_vector vRotation{};
	_vector vTranslation{};
	if (!XMMatrixDecompose(&vScale, &vRotation, &vTranslation, pCamera->GetTransform().GetLoadedCombinedWorldMatrix()))
	{
		return E_FAIL;
	}

	_float fRotationLengthSq{};
	XMStoreFloat(&fRotationLengthSq, XMVector4LengthSq(vRotation));
	if (fRotationLengthSq <= FLT_EPSILON)
	{
		return E_FAIL;
	}

	XMStoreFloat3(&OutPose.vPosition, vTranslation);
	XMStoreFloat4(&OutPose.vRotation, XMQuaternionNormalize(vRotation));
	OutPose.fFovY = pCamera->GetFovY();
	return S_OK;
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
