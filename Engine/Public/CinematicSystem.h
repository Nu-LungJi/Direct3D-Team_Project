#pragma once
#include "Engine_Base.h"
#include "CinematicTypes.h"
#include "Handle.h"

NS_BEGIN(Engine)

class CCameraManager;
class CCinematicAsset;


class ENGINE_DLL CCinematicSystem final : public CEngineBase
{
private:
	CCinematicSystem(CCameraManager& CameraManager);
	~CCinematicSystem() override;
private:
	HRESULT Initialize(const StringID& CinematicCameraID);

public:
	void Update(_float fTimeDelta);

public:
	HRESULT BeginCameraControl();
	HRESULT EndCameraControl();
	_bool HasCameraControl() const;

public:
	HRESULT RegistAsset(const SPtr<CCinematicAsset>& pAsset);
	HRESULT Load(const std::string& CinematicName);
	HRESULT Play(const StringID& CinematicID);
	HRESULT Play(const StringID& CinematicID, const CHandle& TargetHandle);
	void Stop();
	_bool IsPlaying() const;
	_float GetPlayTime() const;

private:
	HRESULT Play(const StringID& CinematicID, const std::optional<CHandle>& TargetHandle);
	HRESULT EvaluateCamera(_float fPlayTime, FCinematicCameraPose& OutPose);
	const FCinematicCameraShot* FindActiveShot(_float fPlayTime) const;
	HRESULT EvaluateShot(const FCinematicCameraShot& Shot, _float fShotTime, FCinematicCameraPose& OutPose) const;
	HRESULT GetTargetWorldMatrix(_matrix& OutTargetWorld) const;
	HRESULT ConvertTargetLocalPose(const FCinematicCameraPose& LocalPose, _fmatrix TargetWorld, FCinematicCameraPose& OutWorldPose) const;
	HRESULT ApplyCameraPose(const FCinematicCameraPose& Pose);

private:
	HRESULT LinearInterpolate(const FCinematicCameraKeyframe* prevKeyFrame, const FCinematicCameraKeyframe* nextKeyFrame, const _float fRatio, FCinematicCameraPose& OutPose) const;
	HRESULT CatMullRomInterpolate(const FCinematicCameraKeyframe& p0,
		const FCinematicCameraKeyframe& p1,
		const FCinematicCameraKeyframe& p2,
		const FCinematicCameraKeyframe& p3,
		const _float fRatio, FCinematicCameraPose& OutPose) const;

private:
	// CGameInstance 반복호출보단 그냥 가져와 씀
	CCameraManager& m_CameraManager;

	// 시네마틱 재생해서 보여주는 카메라
	StringID m_CinematicCameraID { "CinematicCamera" };
	// 시네마틱 재생 전 카메라
	std::optional<StringID> m_PreviousCameraID {};

	SPtr<CCinematicAsset> m_pPlayingAsset{};
	std::unordered_map<StringID, SPtr<CCinematicAsset>> m_Assets{};
	std::optional<CHandle> m_TargetHandle{};
	const FCinematicCameraShot* m_pActiveShot{};
	_float4x4 m_SnapshotTargetWorld{};
	_bool m_bHasSnapshotTargetWorld{ false };

private:
	_float m_fPlayTime { 0.f };
	_bool m_bPlaying { false };
public:
	static UPtr<CCinematicSystem> Create(CCameraManager& CameraManager, const StringID& CinematicCameraID);
};

NS_END
