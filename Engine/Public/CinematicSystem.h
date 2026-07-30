#pragma once
#include "Engine_Base.h"
#include "CinematicTypes.h"
#include "Handle.h"

NS_BEGIN(Engine)

class CCameraManager;
class CCameraObject;
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
	HRESULT Play(const StringID& CinematicID, const FCinematicPlayOptions& Options = {});
	HRESULT Play(const StringID& CinematicID, const CHandle& TargetHandle, const FCinematicPlayOptions& Options = {});
	void Stop();
	_bool IsPlaying() const;
	_float GetPlayTime() const;

private:
	enum class EPlayState
	{
		Stopped,
		BlendingIn,
		Playing,
		BlendingOut
	};

	HRESULT Play(const StringID& CinematicID, const std::optional<CHandle>& TargetHandle, const FCinematicPlayOptions& Options);
	HRESULT BeginBlendIn();
	HRESULT UpdateBlendIn(_float fTimeDelta);
	HRESULT BeginBlendOut(const FCinematicCameraPose& StartPose);
	HRESULT UpdateBlendOut(_float fTimeDelta);
	void FinishPlayback();
	HRESULT EvaluateCamera(_float fPlayTime, FCinematicCameraPose& OutPose);
	const FCinematicCameraShot* FindActiveShot(_float fPlayTime) const;
	HRESULT EvaluateShot(const FCinematicCameraShot& Shot, _float fShotTime, FCinematicCameraPose& OutPose) const;
	HRESULT GetTargetWorldMatrix(_matrix& OutTargetWorld) const;
	HRESULT ConvertTargetLocalPose(const FCinematicCameraPose& LocalPose, _fmatrix TargetWorld, FCinematicCameraPose& OutWorldPose) const;
	HRESULT GetCameraPose(const CCameraObject* pCamera, FCinematicCameraPose& OutPose) const;
	HRESULT ApplyCameraPose(const FCinematicCameraPose& Pose);

private:
	// 선형보간 or CatMullRom
	HRESULT LinearInterpolate(const FCinematicCameraKeyframe* prevKeyFrame, const FCinematicCameraKeyframe* nextKeyFrame, const _float fRatio, FCinematicCameraPose& OutPose) const;
	HRESULT CatMullRomInterpolate(const FCinematicCameraKeyframe& p0,
		const FCinematicCameraKeyframe& p1,
		const FCinematicCameraKeyframe& p2,
		const FCinematicCameraKeyframe& p3,
		const _float fRatio, FCinematicCameraPose& OutPose) const;

private:
	// 타겟 -> 카메라 방향으로 SphereSweep
	_bool TargetToCameraSphereSweep(const _float3& TargetPosition, const _float3& CameraPosition, _float fCollisionRadius, _float3& OutCameraPosition) const;

private:
	// CGameInstance 반복호출보단 그냥 가져와 씀
	CCameraManager& m_CameraManager;

	// 시네마틱 재생해서 보여주는 카메라
	StringID m_CinematicCameraID { "CinematicCamera" };
	// 시네마틱 재생 전 카메라
	std::optional<StringID> m_PreviousCameraID {};

	SPtr<CCinematicAsset> m_pPlayingAsset{};
	std::unordered_map<StringID, SPtr<CCinematicAsset>> m_Assets{};

	// 공전 할 타겟의 핸들
	std::optional<CHandle> m_TargetHandle{};
	const FCinematicCameraShot* m_pActiveShot{};
	_float4x4 m_SnapshotTargetWorld{};
	_bool m_bHasSnapshotTargetWorld{ false };

	// 복귀 옵션
	FCinematicPlayOptions m_PlayOptions{};
	FCinematicCameraPose m_BlendStartPose{};
	_float3 m_vBlendInStartTargetOffset{};
	_bool m_bUseTargetOrbitBlendIn{ false };

private:
	_float m_fPlayTime { 0.f };
	_float m_fStartBlendTime{ 0.f };
	_float m_fReturnBlendTime{ 0.f };
	// 현재 컷신의 플레이 상태
	EPlayState m_ePlayState{ EPlayState::Stopped };

private:
	_float CINEMATIC_CAMERA_COLLISION_RADIUS = 0.3f;
	_float CINEMATIC_CAMERA_COLLISION_PADDING = 0.05f;
public:
	static UPtr<CCinematicSystem> Create(CCameraManager& CameraManager, const StringID& CinematicCameraID);
};

NS_END
