#pragma once
#include "ComPxJoint.h"

namespace physx
{
	class PxD6Joint;
}

NS_BEGIN(Engine)

class ENGINE_DLL CComPxD6Joint final : public CComPxJoint
{
public:
	enum class AXIS : uint8_t
	{
		X,
		Y,
		Z,
		TWIST,
		SWING_Y,
		SWING_Z,
		COUNT
	};

	enum class MOTION : uint8_t
	{
		LOCKED,
		LIMITED,
		FREE
	};

	enum class ANGULAR_DRIVE_MODE : uint8_t
	{
		SWING_TWIST,
		SLERP
	};

	enum class DRIVE : uint8_t
	{
		X,
		Y,
		Z,
		TWIST,
		SWING_Y,
		SWING_Z,
		SLERP,
		COUNT
	};

	struct LIMIT_RESPONSE
	{
		// Limit 도달 시 반발, 반발 적용 최소 속도, Soft Limit의 강성/감쇠.
		_float fRestitution{};
		_float fBounceThreshold{};
		_float fStiffness{};
		_float fDamping{};
	};

	struct LINEAR_LIMIT_DESC
	{
		// 해당 로컬 선형축에서 허용할 비대칭 이동 범위.
		_float fLower{ -1.f };
		_float fUpper{ 1.f };
		LIMIT_RESPONSE tResponse{};
	};

	struct TWIST_LIMIT_DESC
	{
		// Joint Local X축을 중심으로 허용할 비틀림 각도 범위.
		_float fLowerDegrees{ -45.f };
		_float fUpperDegrees{ 45.f };
		LIMIT_RESPONSE tResponse{
			.fBounceThreshold = 28.6478898f
		};
	};

	struct SWING_LIMIT_DESC
	{
		// Joint Local X축이 Y/Z 방향으로 기울어질 수 있는 타원형 Cone 각도.
		_float fYDegrees{ 45.f };
		_float fZDegrees{ 45.f };
		LIMIT_RESPONSE tResponse{
			.fBounceThreshold = 28.6478898f
		};
	};

	struct DRIVE_DESC
	{
		// 목표 Pose/Velocity를 추종하는 Drive의 강성, 감쇠, 최대 출력.
		_float fStiffness{};
		_float fDamping{};
		_float fForceLimit{
			std::numeric_limits<_float>::max()
		};

		// 질량 독립 가속도 Drive 여부와 Drive 출력을 파괴 힘에 포함할지 여부.
		_bool bAcceleration{};
		_bool bOutputForce{};
	};

	struct DESC : public CComPxJoint::DESC
	{
		// X/Y/Z/Twist/Swing Y/Swing Z 축별 잠금·제한·자유 상태.
		std::array<MOTION, static_cast<size_t>(AXIS::COUNT)>
			eMotions{};

		// LIMITED로 설정한 선형축과 회전축에서 사용할 범위.
		std::array<LINEAR_LIMIT_DESC, 3> tLinearLimits{};
		TWIST_LIMIT_DESC tTwistLimit{};
		SWING_LIMIT_DESC tSwingLimit{};

		// 회전 Drive를 축별 Swing/Twist 또는 통합 SLERP 방식으로 구성한다.
		ANGULAR_DRIVE_MODE eAngularDriveMode{
			ANGULAR_DRIVE_MODE::SWING_TWIST
		};
		std::array<DRIVE_DESC, static_cast<size_t>(DRIVE::COUNT)>
			tDrives{};

		// Joint A Frame 기준 상대 목표 Pose와 목표 선속도/각속도.
		FRAME tDrivePose{};
		_float3 vDriveLinearVelocity{};
		_float3 vDriveAngularVelocityDegreesPerSecond{};

		// 생성 시 현재 상대 자세를 Joint 기준 자세로 자동 계산한다.
		_bool bPreserveCurrentPose{ true };
	};

public:
	DECLARE_DERIVED_TYPE(CComPxD6Joint, CComPxJoint)

private:
	explicit CComPxD6Joint();
	explicit CComPxD6Joint(
		const CComPxD6Joint& Prototype);
	~CComPxD6Joint() override;

public:
	_bool SetMotion(AXIS eAxis, MOTION eMotion);
	MOTION GetMotion(AXIS eAxis) const;

	_bool SetLinearLimit(
		AXIS eAxis,
		const LINEAR_LIMIT_DESC& tLimit);
	LINEAR_LIMIT_DESC GetLinearLimit(AXIS eAxis) const;

	_bool SetTwistLimit(
		const TWIST_LIMIT_DESC& tLimit);
	TWIST_LIMIT_DESC GetTwistLimit() const;

	_bool SetSwingLimit(
		const SWING_LIMIT_DESC& tLimit);
	SWING_LIMIT_DESC GetSwingLimit() const;

	_float GetTwistAngleDegrees() const;
	_float GetSwingYAngleDegrees() const;
	_float GetSwingZAngleDegrees() const;

	_bool SetAngularDriveMode(
		ANGULAR_DRIVE_MODE eMode);
	ANGULAR_DRIVE_MODE GetAngularDriveMode() const;

	_bool SetDrive(
		DRIVE eDrive,
		const DRIVE_DESC& tDrive);
	DRIVE_DESC GetDrive(DRIVE eDrive) const;

	_bool SetDrivePose(
		const FRAME& tPose,
		_bool bAutoWake = true);
	FRAME GetDrivePose() const;

	_bool SetDriveVelocity(
		const _float3& vLinearVelocity,
		const _float3& vAngularVelocityDegreesPerSecond,
		_bool bAutoWake = true);
	void GetDriveVelocity(
		_float3& vOutLinearVelocity,
		_float3& vOutAngularVelocityDegreesPerSecond) const;

	void UpdateGUI() override;

private:
	HRESULT Initialize(void* pArg) override;
	physx::PxD6Joint* GetD6Joint() const;

public:
	static UPtr<CComPxD6Joint> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	void Free() override;
};

NS_END
