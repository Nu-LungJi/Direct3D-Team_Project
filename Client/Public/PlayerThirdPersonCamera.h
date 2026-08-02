

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
	_float m_fMinPitch{ -20.f };
	_float m_fMaxPitch{ 65.f };
	_float m_fMouseSensitivity{ 10.f };
	_float m_fShoulderOffset{ 2.f };
	_float m_fHorizontalDeadZoneRadius{ 1.f };
	_float m_fVerticalDeadZoneHalfHeight{ 1.f };
	_float m_fVerticalFollowSpeed{ 8.f };
	_float3 m_vFollowPivot{};
	_bool m_bFollowPivotInitialized{ false };

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
