#pragma once
#include "ComPxJoint.h"

namespace physx
{
	class PxRevoluteJoint;
}

NS_BEGIN(Engine)

class ENGINE_DLL CComPxRevoluteJoint final : public CComPxJoint
{
public:
	struct DESC : public CComPxJoint::DESC
	{
		// Joint Local X축을 중심으로 허용할 회전 각도와 Limit 반응.
		_float fLowerLimitDegrees{ -90.f };
		_float fUpperLimitDegrees{ 90.f };
		_float fLimitRestitution{};
		_float fLimitBounceThresholdDegreesPerSecond{ 28.6478898f };
		_float fLimitStiffness{};
		_float fLimitDamping{};

		// 목표 회전 속도, 최대 모터 출력, 기어비.
		_float fDriveVelocityDegreesPerSecond{};
		_float fDriveForceLimit{ std::numeric_limits<_float>::max() };
		_float fDriveGearRatio{ 1.f };

		// 현재 자세 보존, 각도 제한, 속도 Drive 및 Free Spin 활성화 여부.
		_bool bPreserveCurrentPose{ true };
		_bool bLimitEnabled{};
		_bool bDriveEnabled{};
		_bool bDriveFreeSpin{};
	};

public:
	DECLARE_DERIVED_TYPE(CComPxRevoluteJoint, CComPxJoint)

private:
	explicit CComPxRevoluteJoint();
	explicit CComPxRevoluteJoint(
		const CComPxRevoluteJoint& Prototype);
	~CComPxRevoluteJoint() override;

public:
	_float GetAngleDegrees() const;
	_float GetVelocityDegreesPerSecond() const;

	_float GetLowerLimitDegrees() const;
	_float GetUpperLimitDegrees() const;
	_float GetLimitRestitution() const;
	_float GetLimitBounceThresholdDegreesPerSecond() const;
	_float GetLimitStiffness() const;
	_float GetLimitDamping() const;

	_bool SetLimit(
		_float fLowerDegrees,
		_float fUpperDegrees);
	_bool SetLimitResponse(
		_float fRestitution,
		_float fBounceThresholdDegreesPerSecond,
		_float fStiffness,
		_float fDamping);

	_float GetDriveVelocityDegreesPerSecond() const;
	_float GetDriveForceLimit() const;
	_float GetDriveGearRatio() const;

	_bool SetDriveVelocity(
		_float fDegreesPerSecond,
		_bool bAutoWake = true);
	_bool SetDriveForceLimit(_float fForceLimit);
	_bool SetDriveGearRatio(_float fGearRatio);

	_bool SetLimitEnabled(_bool bEnabled);
	_bool IsLimitEnabled() const;
	_bool SetDriveEnabled(_bool bEnabled);
	_bool IsDriveEnabled() const;
	_bool SetDriveFreeSpin(_bool bEnabled);
	_bool IsDriveFreeSpin() const;

	void UpdateGUI() override;

private:
	HRESULT Initialize(void* pArg) override;
	physx::PxRevoluteJoint* GetRevoluteJoint() const;

public:
	static UPtr<CComPxRevoluteJoint> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	void Free() override;
};

NS_END
