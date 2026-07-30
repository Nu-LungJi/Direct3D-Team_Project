

#pragma once
#include "CameraObject.h"

NS_BEGIN(Client)

class CPlayerThirdPersonCamera final : public CCameraObject
{
public:
	struct DESC : public CCameraObject::CAMERA_DESC
	{
		CHandle hTarget{};
		_float fDistance{ 7.2f };
		_float fTargetHeight{ 1.2f };
		_float fPitch{ 3.f };
		_float fMinPitch{ -20.f };
		_float fMaxPitch{ 65.f };
		_float fMouseSensitivity{ 10.f };

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
	void UpdateFollow();
private:
	// 타겟 -> 카메라 방향으로 SphereSweep
	_bool PlayerToCameraSphereSweep(const _float3& PlayerPosition, const _float3& CameraPosition, _float fCollisionRadius, _float3& OutCameraPosition) const;

private:
	CHandle m_hTarget{};
	_float m_fYaw{};
	_float m_fPitch{ 3.f };
	_float m_fDistance{ 7.2f };
	_float m_fTargetHeight{ 1.2f };
	_float m_fMinPitch{ -20.f };
	_float m_fMaxPitch{ 65.f };
	_float m_fMouseSensitivity{ 10.f };
	_float m_fShoulderOffset{ 1.35f };
	_float m_fLookSideOffset{ 1.35f };
	_float m_fLookHeightOffset{ 0.3f };
	_float m_fPositionSmoothSpeed{ 12.f };
	_float m_fLookSmoothSpeed{ 16.f };
	_float3 m_vSmoothedPosition{};
	_float3 m_vSmoothedLookTarget{};
	bool m_bFollowInitialized{};

private:
	_float CAMERA_COLLISION_RADIUS = 0.3f;
	_float CAMERA_COLLISION_PADDING = 0.05f;
public:
	static UPtr<CPlayerThirdPersonCamera> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
