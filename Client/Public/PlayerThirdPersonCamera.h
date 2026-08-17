

#pragma once
#include "CameraObject.h"

NS_BEGIN(Client)

struct FRequestPlayerCameraShake;

class CPlayerThirdPersonCamera final : public CCameraObject
{
public:
	struct DESC : public CCameraObject::CAMERA_DESC
	{
		CHandle hTarget{};
		_float fYaw{};
		_float fDistance{ 7.f };
		_float fPitch{ 15.f };
		_float fMinPitch{ -20.f };
		_float fMaxPitch{ 65.f };
		_float fMouseSensitivity{ 10.f };
		_float fShoulderOffset{ 2.f };
		_float fHorizontalDeadZoneRadius{ 1.f };
		_float fVerticalDeadZoneHalfHeight{ 1.f };
		_float fVerticalFollowSpeed{ 8.f };
	};

public:
	DECLARE_DERIVED_TYPE(CPlayerThirdPersonCamera, CCameraObject)

private:
	CPlayerThirdPersonCamera();
	CPlayerThirdPersonCamera(const CPlayerThirdPersonCamera& rhs);
	~CPlayerThirdPersonCamera() override;

public:
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(_float fTimeDelta) override;
	void UpdateFollow(_float fTimeDelta);
private:
	// 타겟 -> 카메라 방향으로 SphereSweep
	_bool PlayerToCameraSphereSweep(const _float3& PlayerPosition, const _float3& CameraPosition, _float fCollisionRadius, _float3& OutCameraPosition) const;

private:
	struct FSHAKE_STATE
	{
		_bool bActive{ false };

		_float fElapsed{};
		_float fDuration{};
		_float fIntensity{};
		_float fFrequency{};
		_float fSeed{};
	};
	void BeginShake(const FRequestPlayerCameraShake& Event);
	void EvaluateShake(_float fTimeDelta, _float3& OutLocalPositionOffset, _float3& OutRotationOffset);
private:
	EVENT_LISTENER_ID m_iShakeListenerID{};
	FSHAKE_STATE m_ShakeState{};
private:
	CHandle m_hTarget{};
	_float m_fYaw{};
	_float m_fPitch{ 15.f };
	_float m_fDistance{ 7.f };
	_float m_fCurrentDistance{ 7.f };
	_float m_fMinPitch{ -20.f };
	_float m_fMaxPitch{ 65.f };
	_float m_fMouseSensitivity{ 10.f };
	_float m_fShoulderOffset{ 2.f };
	_float m_fCurrentShoulderOffset{ 2.f };
	_float m_fHorizontalDeadZoneRadius{ 1.f };
	_float m_fVerticalDeadZoneHalfHeight{ 1.f };
	_float m_fVerticalFollowSpeed{ 8.f };
	_float3 m_vFollowPivot{};
	_bool m_bFollowPivotInitialized{ false };

	// 고속 상승/하강 중에도 플레이어가 화면 상하 구도 밖으로 벗어나지 않게 하는 비행 전용 값.
	_float m_fFlightHorizontalDeadZoneRadius{ 0.35f };
	_float m_fFlightHorizontalFollowSpeed{ 7.f };
	_float m_fBoostHorizontalDeadZoneRadius{ 0.05f };
	_float m_fBoostHorizontalFollowSpeed{ 13.f };
	_float m_fFlightVerticalDeadZoneHalfHeight{ 0.2f };
	_float m_fFlightVerticalFollowSpeed{ 8.f };
	_float m_fFlightMaxVerticalCompositionError{ 1.f };
	_float m_fFlightCompositionCorrectionSpeed{ 10.f };
	_float m_fFlightVerticalLookAheadTime{ 0.02f };
	_float m_fFlightMaxVerticalLookAhead{ 0.35f };
	// 캐릭터와 빗자루 전체는 유지하면서 화면을 크게 채우는 비행 기본 거리다.
	_float m_fFlightDistance{ 5.6f };
	_float m_fFlightDistanceResponse{ 5.f };
	_float m_fFlightTargetOffsetY{ 0.85f };
	_float m_fBoostTargetOffsetY{ 0.85f };
	// 비행 중 캐릭터가 화면 정중앙보다 살짝 왼쪽에 보이도록 조준점을 오른쪽으로 이동한다.
	_float m_fFlightShoulderOffset{ 0.55f };
	_float m_fFlightShoulderResponse{ 5.f };
	_float m_fFlightCameraHeightOffset{ 0.15f };
	_float m_fCurrentFlightCameraHeightOffset{};
	_float m_fFlightCameraPositionResponse{ 10.f };
	_float3 m_vSmoothedCameraPosition{};
	_bool m_bSmoothedCameraPositionInitialized{ false };

	// 실제 비행 속도에 따라 시야각, 거리, 난기류를 함께 제어한다.
	_float m_fBaseFovY{ 75.f };
	_float m_fCurrentSpeedEffectRatio{};
	_float m_fSpeedEffectStartSpeed{ 14.f };
	_float m_fSpeedEffectFullSpeed{ 36.f };
	_float m_fSpeedEffectResponse{ 3.5f };
	// 최고 속도에서 FOV를 넓혀 주변부가 빠르게 벌어지는 부스트 원근감을 만든다.
	_float m_fSpeedFovExpansion{ 8.f };
	// FOV 확장으로 캐릭터가 작아지는 만큼 고속에서 카메라를 가까이 당긴다.
	// 최고 속도에서는 약 5.2까지 가까워져 캐릭터 크기를 보정한다.
	_float m_fSpeedDistanceExtension{ -0.4f };
	_float m_fTurbulenceElapsed{};
	_float m_fTurbulenceMinFrequency{ 8.f };
	_float m_fTurbulenceMaxFrequency{ 14.f };
	_float m_fTurbulenceStartRatio{ 0.85f };
	_float m_fTurbulencePositionAmplitude{ 0.025f };
	_float m_fTurbulenceRotationAmplitude{ 0.14f };

private:
	_float CAMERA_TARGET_OFFSET_Y = 1.5f;

public:
	CHandle GetTargetHandle() const { return m_hTarget; }

private:
	_float CAMERA_COLLISION_RADIUS = 0.3f;
	_float CAMERA_COLLISION_PADDING = 0.05f;
public:
	static UPtr<CPlayerThirdPersonCamera> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
