#include "pch.h"
#include "ComPxD6Joint.h"
#include "ComPxCharacterController.h"
#include "ComPxRigidBody.h"

#pragma push_macro("new")
#undef new

#include "PxPhysicsAPI.h"

#pragma pop_macro("new")

using namespace physx;

NS_USING(Engine)

namespace
{
	constexpr size_t D6_AXIS_COUNT =
		static_cast<size_t>(CComPxD6Joint::AXIS::COUNT);
	constexpr size_t D6_DRIVE_COUNT =
		static_cast<size_t>(CComPxD6Joint::DRIVE::COUNT);
	constexpr _float D6_TWIST_LIMIT_DEGREES = 360.f;
	constexpr _float D6_SWING_LIMIT_DEGREES = 180.f;

	_float ToD6Radians(_float fDegrees)
	{
		return XMConvertToRadians(fDegrees);
	}

	_float ToD6Degrees(_float fRadians)
	{
		return XMConvertToDegrees(fRadians);
	}

	_bool IsD6Finite(const _float3& vValue)
	{
		return std::isfinite(vValue.x) &&
			std::isfinite(vValue.y) &&
			std::isfinite(vValue.z);
	}

	_bool IsD6Finite(const _float4& vValue)
	{
		return std::isfinite(vValue.x) &&
			std::isfinite(vValue.y) &&
			std::isfinite(vValue.z) &&
			std::isfinite(vValue.w);
	}

	PxQuat ToD6NormalizedQuat(const _float4& vRotation)
	{
		const PxQuat tRotation{
			vRotation.x,
			vRotation.y,
			vRotation.z,
			vRotation.w };

		return tRotation.magnitudeSquared() > 0.f
			? tRotation.getNormalized()
			: PxQuat{ PxIdentity };
	}

	PxTransform ToD6PxTransform(
		const CComPxJoint::FRAME& tFrame)
	{
		return PxTransform{
			PxVec3{
				tFrame.vPosition.x,
				tFrame.vPosition.y,
				tFrame.vPosition.z },
			ToD6NormalizedQuat(tFrame.vRotation) };
	}

	CComPxJoint::FRAME ToD6Frame(
		const PxTransform& tTransform)
	{
		return {
			.vPosition = {
				tTransform.p.x,
				tTransform.p.y,
				tTransform.p.z },
			.vRotation = {
				tTransform.q.x,
				tTransform.q.y,
				tTransform.q.z,
				tTransform.q.w }
		};
	}

	void PreserveD6CurrentPose(CComPxJoint::DESC& tDesc)
	{
		PxRigidActor* pActorA =
			tDesc.pRigidBodyA
			? tDesc.pRigidBodyA->GetActor()
			: (tDesc.pCharacterControllerA
				? tDesc.pCharacterControllerA->GetActor()
				: nullptr);
		PxRigidActor* pActorB =
			tDesc.pRigidBodyB
			? tDesc.pRigidBodyB->GetActor()
			: (tDesc.pCharacterControllerB
				? tDesc.pCharacterControllerB->GetActor()
				: nullptr);

		if (pActorA && pActorB)
		{
			const PxTransform tPoseA = pActorA->getGlobalPose();
			const PxTransform tPoseB = pActorB->getGlobalPose();

			tDesc.tLocalFrameA = {};
			tDesc.tLocalFrameB =
				ToD6Frame(tPoseB.getInverse() * tPoseA);
		}
		else if (pActorA)
		{
			tDesc.tLocalFrameA = {};
			tDesc.tLocalFrameB =
				ToD6Frame(pActorA->getGlobalPose());
		}
		else if (pActorB)
		{
			tDesc.tLocalFrameA =
				ToD6Frame(pActorB->getGlobalPose());
			tDesc.tLocalFrameB = {};
		}
	}

	_bool IsValidD6Axis(CComPxD6Joint::AXIS eAxis)
	{
		return static_cast<size_t>(eAxis) < D6_AXIS_COUNT;
	}

	_bool IsD6LinearAxis(CComPxD6Joint::AXIS eAxis)
	{
		return eAxis == CComPxD6Joint::AXIS::X ||
			eAxis == CComPxD6Joint::AXIS::Y ||
			eAxis == CComPxD6Joint::AXIS::Z;
	}

	_bool IsValidD6Motion(CComPxD6Joint::MOTION eMotion)
	{
		return eMotion == CComPxD6Joint::MOTION::LOCKED ||
			eMotion == CComPxD6Joint::MOTION::LIMITED ||
			eMotion == CComPxD6Joint::MOTION::FREE;
	}

	_bool IsValidD6Drive(CComPxD6Joint::DRIVE eDrive)
	{
		return static_cast<size_t>(eDrive) < D6_DRIVE_COUNT;
	}

	PxD6Axis::Enum ToPxD6Axis(CComPxD6Joint::AXIS eAxis)
	{
		switch (eAxis)
		{
		case CComPxD6Joint::AXIS::X:
			return PxD6Axis::eX;
		case CComPxD6Joint::AXIS::Y:
			return PxD6Axis::eY;
		case CComPxD6Joint::AXIS::Z:
			return PxD6Axis::eZ;
		case CComPxD6Joint::AXIS::TWIST:
			return PxD6Axis::eTWIST;
		case CComPxD6Joint::AXIS::SWING_Y:
			return PxD6Axis::eSWING1;
		case CComPxD6Joint::AXIS::SWING_Z:
			return PxD6Axis::eSWING2;
		default:
			return PxD6Axis::eX;
		}
	}

	PxD6Motion::Enum ToPxD6Motion(
		CComPxD6Joint::MOTION eMotion)
	{
		switch (eMotion)
		{
		case CComPxD6Joint::MOTION::LIMITED:
			return PxD6Motion::eLIMITED;
		case CComPxD6Joint::MOTION::FREE:
			return PxD6Motion::eFREE;
		case CComPxD6Joint::MOTION::LOCKED:
		default:
			return PxD6Motion::eLOCKED;
		}
	}

	CComPxD6Joint::MOTION FromPxD6Motion(
		PxD6Motion::Enum eMotion)
	{
		switch (eMotion)
		{
		case PxD6Motion::eLIMITED:
			return CComPxD6Joint::MOTION::LIMITED;
		case PxD6Motion::eFREE:
			return CComPxD6Joint::MOTION::FREE;
		case PxD6Motion::eLOCKED:
		default:
			return CComPxD6Joint::MOTION::LOCKED;
		}
	}

	PxD6Drive::Enum ToPxD6Drive(
		CComPxD6Joint::DRIVE eDrive)
	{
		switch (eDrive)
		{
		case CComPxD6Joint::DRIVE::X:
			return PxD6Drive::eX;
		case CComPxD6Joint::DRIVE::Y:
			return PxD6Drive::eY;
		case CComPxD6Joint::DRIVE::Z:
			return PxD6Drive::eZ;
		case CComPxD6Joint::DRIVE::TWIST:
			return PxD6Drive::eTWIST;
		case CComPxD6Joint::DRIVE::SWING_Y:
			return PxD6Drive::eSWING1;
		case CComPxD6Joint::DRIVE::SWING_Z:
			return PxD6Drive::eSWING2;
		case CComPxD6Joint::DRIVE::SLERP:
			return PxD6Drive::eSLERP;
		default:
			return PxD6Drive::eX;
		}
	}

	_bool IsValidD6AngularDriveMode(
		CComPxD6Joint::ANGULAR_DRIVE_MODE eMode)
	{
		return eMode ==
				CComPxD6Joint::ANGULAR_DRIVE_MODE::SWING_TWIST ||
			eMode ==
				CComPxD6Joint::ANGULAR_DRIVE_MODE::SLERP;
	}

	PxD6AngularDriveConfig::Enum ToPxD6AngularDriveMode(
		CComPxD6Joint::ANGULAR_DRIVE_MODE eMode)
	{
		return eMode ==
			CComPxD6Joint::ANGULAR_DRIVE_MODE::SLERP
			? PxD6AngularDriveConfig::eSLERP
			: PxD6AngularDriveConfig::eSWING_TWIST;
	}

	CComPxD6Joint::ANGULAR_DRIVE_MODE
	FromPxD6AngularDriveMode(
		PxD6AngularDriveConfig::Enum eMode)
	{
		return eMode == PxD6AngularDriveConfig::eSLERP
			? CComPxD6Joint::ANGULAR_DRIVE_MODE::SLERP
			: CComPxD6Joint::ANGULAR_DRIVE_MODE::SWING_TWIST;
	}

	_bool IsD6DriveCompatible(
		CComPxD6Joint::DRIVE eDrive,
		CComPxD6Joint::ANGULAR_DRIVE_MODE eMode)
	{
		if (eDrive == CComPxD6Joint::DRIVE::X ||
			eDrive == CComPxD6Joint::DRIVE::Y ||
			eDrive == CComPxD6Joint::DRIVE::Z)
		{
			return true;
		}

		if (eMode ==
			CComPxD6Joint::ANGULAR_DRIVE_MODE::SLERP)
		{
			return eDrive == CComPxD6Joint::DRIVE::SLERP;
		}

		return eDrive == CComPxD6Joint::DRIVE::TWIST ||
			eDrive == CComPxD6Joint::DRIVE::SWING_Y ||
			eDrive == CComPxD6Joint::DRIVE::SWING_Z;
	}

	_bool IsValidD6LimitResponse(
		const CComPxD6Joint::LIMIT_RESPONSE& tResponse)
	{
		return std::isfinite(tResponse.fRestitution) &&
			std::isfinite(tResponse.fBounceThreshold) &&
			std::isfinite(tResponse.fStiffness) &&
			std::isfinite(tResponse.fDamping) &&
			tResponse.fRestitution >= 0.f &&
			tResponse.fRestitution <= 1.f &&
			tResponse.fBounceThreshold >= 0.f &&
			tResponse.fStiffness >= 0.f &&
			tResponse.fDamping >= 0.f;
	}

	_bool IsValidD6LinearLimit(
		const CComPxD6Joint::LINEAR_LIMIT_DESC& tLimit)
	{
		return std::isfinite(tLimit.fLower) &&
			std::isfinite(tLimit.fUpper) &&
			tLimit.fLower <= tLimit.fUpper &&
			IsValidD6LimitResponse(tLimit.tResponse);
	}

	_bool IsValidD6TwistLimit(
		const CComPxD6Joint::TWIST_LIMIT_DESC& tLimit)
	{
		return std::isfinite(tLimit.fLowerDegrees) &&
			std::isfinite(tLimit.fUpperDegrees) &&
			tLimit.fLowerDegrees > -D6_TWIST_LIMIT_DEGREES &&
			tLimit.fUpperDegrees < D6_TWIST_LIMIT_DEGREES &&
			tLimit.fLowerDegrees <= tLimit.fUpperDegrees &&
			IsValidD6LimitResponse(tLimit.tResponse);
	}

	_bool IsValidD6SwingLimit(
		const CComPxD6Joint::SWING_LIMIT_DESC& tLimit)
	{
		return std::isfinite(tLimit.fYDegrees) &&
			std::isfinite(tLimit.fZDegrees) &&
			tLimit.fYDegrees > 0.f &&
			tLimit.fYDegrees < D6_SWING_LIMIT_DEGREES &&
			tLimit.fZDegrees > 0.f &&
			tLimit.fZDegrees < D6_SWING_LIMIT_DEGREES &&
			IsValidD6LimitResponse(tLimit.tResponse);
	}

	_bool IsValidD6DriveDesc(
		const CComPxD6Joint::DRIVE_DESC& tDrive)
	{
		return std::isfinite(tDrive.fStiffness) &&
			std::isfinite(tDrive.fDamping) &&
			std::isfinite(tDrive.fForceLimit) &&
			tDrive.fStiffness >= 0.f &&
			tDrive.fDamping >= 0.f &&
			tDrive.fForceLimit >= 0.f;
	}

	template<typename T>
	void ApplyD6LimitResponse(
		T& tLimit,
		const CComPxD6Joint::LIMIT_RESPONSE& tResponse,
		_bool bAngular)
	{
		tLimit.restitution = tResponse.fRestitution;
		tLimit.bounceThreshold = bAngular
			? ToD6Radians(tResponse.fBounceThreshold)
			: tResponse.fBounceThreshold;
		tLimit.stiffness = tResponse.fStiffness;
		tLimit.damping = tResponse.fDamping;
	}

	template<typename T>
	CComPxD6Joint::LIMIT_RESPONSE GetD6LimitResponse(
		const T& tLimit,
		_bool bAngular)
	{
		return {
			.fRestitution = tLimit.restitution,
			.fBounceThreshold = bAngular
				? ToD6Degrees(tLimit.bounceThreshold)
				: tLimit.bounceThreshold,
			.fStiffness = tLimit.stiffness,
			.fDamping = tLimit.damping
		};
	}

	PxD6JointDrive ToPxD6JointDrive(
		const CComPxD6Joint::DRIVE_DESC& tDrive)
	{
		PxD6JointDrive tPxDrive{
			tDrive.fStiffness,
			tDrive.fDamping,
			tDrive.fForceLimit,
			tDrive.bAcceleration };
		if (tDrive.bOutputForce)
		{
			tPxDrive.flags |=
				PxD6JointDriveFlag::eOUTPUT_FORCE;
		}
		return tPxDrive;
	}

	CComPxD6Joint::DRIVE_DESC FromPxD6JointDrive(
		const PxD6JointDrive& tDrive)
	{
		return {
			.fStiffness = tDrive.stiffness,
			.fDamping = tDrive.damping,
			.fForceLimit = tDrive.forceLimit,
			.bAcceleration = tDrive.flags.isSet(
				PxD6JointDriveFlag::eACCELERATION),
			.bOutputForce = tDrive.flags.isSet(
				PxD6JointDriveFlag::eOUTPUT_FORCE)
		};
	}

	_bool IsValidD6Desc(const CComPxD6Joint::DESC& tDesc)
	{
		for (const auto eMotion : tDesc.eMotions)
		{
			if (!IsValidD6Motion(eMotion))
				return false;
		}

		for (const auto& tLimit : tDesc.tLinearLimits)
		{
			if (!IsValidD6LinearLimit(tLimit))
				return false;
		}

		if (!IsValidD6TwistLimit(tDesc.tTwistLimit) ||
			!IsValidD6SwingLimit(tDesc.tSwingLimit) ||
			!IsValidD6AngularDriveMode(
				tDesc.eAngularDriveMode))
		{
			return false;
		}

		for (const auto& tDrive : tDesc.tDrives)
		{
			if (!IsValidD6DriveDesc(tDrive))
				return false;
		}

		return IsD6Finite(tDesc.tDrivePose.vPosition) &&
			IsD6Finite(tDesc.tDrivePose.vRotation) &&
			IsD6Finite(tDesc.vDriveLinearVelocity) &&
			IsD6Finite(
				tDesc.vDriveAngularVelocityDegreesPerSecond);
	}
}

CComPxD6Joint::CComPxD6Joint()
{
}

CComPxD6Joint::CComPxD6Joint(
	const CComPxD6Joint& Prototype)
	: CComPxJoint{ Prototype }
{
}

CComPxD6Joint::~CComPxD6Joint()
{
}

HRESULT CComPxD6Joint::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc || !IsValidD6Desc(*pDesc))
		return E_FAIL;

	DESC tJointDesc = *pDesc;
	if (tJointDesc.bPreserveCurrentPose)
		PreserveD6CurrentPose(tJointDesc);

	if (FAILED(CComPxJoint::Initialize(&tJointDesc)))
		return E_FAIL;

	auto* pPhysics = CGameInstance::Get().PxGetPhysics();
	if (!pPhysics)
		return E_FAIL;

	PxD6Joint* pJoint = PxD6JointCreate(
		*pPhysics,
		GetActorA(),
		ToD6PxTransform(GetLocalFrameA()),
		GetActorB(),
		ToD6PxTransform(GetLocalFrameB()));
	if (!pJoint)
		return E_FAIL;

	for (size_t i = 0; i < D6_AXIS_COUNT; ++i)
	{
		const AXIS eAxis = static_cast<AXIS>(i);
		pJoint->setMotion(
			ToPxD6Axis(eAxis),
			ToPxD6Motion(tJointDesc.eMotions[i]));
	}

	const PxTolerancesScale& tScale =
		pPhysics->getTolerancesScale();
	for (size_t i = 0; i < 3; ++i)
	{
		const auto& tLimitDesc =
			tJointDesc.tLinearLimits[i];
		PxJointLinearLimitPair tLimit{
			tScale,
			tLimitDesc.fLower,
			tLimitDesc.fUpper };
		ApplyD6LimitResponse(
			tLimit,
			tLimitDesc.tResponse,
			false);
		pJoint->setLinearLimit(
			ToPxD6Axis(static_cast<AXIS>(i)),
			tLimit);
	}

	PxJointAngularLimitPair tTwistLimit{
		ToD6Radians(
			tJointDesc.tTwistLimit.fLowerDegrees),
		ToD6Radians(
			tJointDesc.tTwistLimit.fUpperDegrees) };
	ApplyD6LimitResponse(
		tTwistLimit,
		tJointDesc.tTwistLimit.tResponse,
		true);
	pJoint->setTwistLimit(tTwistLimit);

	PxJointLimitCone tSwingLimit{
		ToD6Radians(
			tJointDesc.tSwingLimit.fYDegrees),
		ToD6Radians(
			tJointDesc.tSwingLimit.fZDegrees) };
	ApplyD6LimitResponse(
		tSwingLimit,
		tJointDesc.tSwingLimit.tResponse,
		true);
	pJoint->setSwingLimit(tSwingLimit);

	pJoint->setAngularDriveConfig(
		ToPxD6AngularDriveMode(
			tJointDesc.eAngularDriveMode));

	for (size_t i = 0; i < D6_DRIVE_COUNT; ++i)
	{
		const DRIVE eDrive = static_cast<DRIVE>(i);
		if (!IsD6DriveCompatible(
			eDrive,
			tJointDesc.eAngularDriveMode))
		{
			continue;
		}

		pJoint->setDrive(
			ToPxD6Drive(eDrive),
			ToPxD6JointDrive(
				tJointDesc.tDrives[i]));
	}

	pJoint->setDrivePosition(
		ToD6PxTransform(tJointDesc.tDrivePose),
		false);
	pJoint->setDriveVelocity(
		PxVec3{
			tJointDesc.vDriveLinearVelocity.x,
			tJointDesc.vDriveLinearVelocity.y,
			tJointDesc.vDriveLinearVelocity.z },
		PxVec3{
			ToD6Radians(
				tJointDesc
				.vDriveAngularVelocityDegreesPerSecond.x),
			ToD6Radians(
				tJointDesc
				.vDriveAngularVelocityDegreesPerSecond.y),
			ToD6Radians(
				tJointDesc
				.vDriveAngularVelocityDegreesPerSecond.z) },
		false);

	if (!AttachJoint(pJoint))
	{
		pJoint->release();
		return E_FAIL;
	}

	return S_OK;
}

PxD6Joint* CComPxD6Joint::GetD6Joint() const
{
	return static_cast<PxD6Joint*>(GetJoint());
}

_bool CComPxD6Joint::SetMotion(
	AXIS eAxis,
	MOTION eMotion)
{
	auto* pJoint = GetD6Joint();
	if (!pJoint ||
		!IsValidD6Axis(eAxis) ||
		!IsValidD6Motion(eMotion))
	{
		return false;
	}

	pJoint->setMotion(
		ToPxD6Axis(eAxis),
		ToPxD6Motion(eMotion));
	return true;
}

CComPxD6Joint::MOTION CComPxD6Joint::GetMotion(
	AXIS eAxis) const
{
	const auto* pJoint = GetD6Joint();
	if (!pJoint || !IsValidD6Axis(eAxis))
		return MOTION::LOCKED;

	return FromPxD6Motion(
		pJoint->getMotion(ToPxD6Axis(eAxis)));
}

_bool CComPxD6Joint::SetLinearLimit(
	AXIS eAxis,
	const LINEAR_LIMIT_DESC& tLimitDesc)
{
	auto* pJoint = GetD6Joint();
	if (!pJoint ||
		!IsD6LinearAxis(eAxis) ||
		!IsValidD6LinearLimit(tLimitDesc))
	{
		return false;
	}

	PxJointLinearLimitPair tLimit =
		pJoint->getLinearLimit(ToPxD6Axis(eAxis));
	tLimit.lower = tLimitDesc.fLower;
	tLimit.upper = tLimitDesc.fUpper;
	ApplyD6LimitResponse(
		tLimit,
		tLimitDesc.tResponse,
		false);
	pJoint->setLinearLimit(
		ToPxD6Axis(eAxis),
		tLimit);
	return true;
}

CComPxD6Joint::LINEAR_LIMIT_DESC
CComPxD6Joint::GetLinearLimit(AXIS eAxis) const
{
	const auto* pJoint = GetD6Joint();
	if (!pJoint || !IsD6LinearAxis(eAxis))
		return {};

	const PxJointLinearLimitPair tLimit =
		pJoint->getLinearLimit(ToPxD6Axis(eAxis));
	return {
		.fLower = tLimit.lower,
		.fUpper = tLimit.upper,
		.tResponse = GetD6LimitResponse(
			tLimit,
			false)
	};
}

_bool CComPxD6Joint::SetTwistLimit(
	const TWIST_LIMIT_DESC& tLimitDesc)
{
	auto* pJoint = GetD6Joint();
	if (!pJoint ||
		!IsValidD6TwistLimit(tLimitDesc))
	{
		return false;
	}

	PxJointAngularLimitPair tLimit{
		ToD6Radians(tLimitDesc.fLowerDegrees),
		ToD6Radians(tLimitDesc.fUpperDegrees) };
	ApplyD6LimitResponse(
		tLimit,
		tLimitDesc.tResponse,
		true);
	pJoint->setTwistLimit(tLimit);
	return true;
}

CComPxD6Joint::TWIST_LIMIT_DESC
CComPxD6Joint::GetTwistLimit() const
{
	const auto* pJoint = GetD6Joint();
	if (!pJoint)
		return {};

	const PxJointAngularLimitPair tLimit =
		pJoint->getTwistLimit();
	return {
		.fLowerDegrees = ToD6Degrees(tLimit.lower),
		.fUpperDegrees = ToD6Degrees(tLimit.upper),
		.tResponse = GetD6LimitResponse(
			tLimit,
			true)
	};
}

_bool CComPxD6Joint::SetSwingLimit(
	const SWING_LIMIT_DESC& tLimitDesc)
{
	auto* pJoint = GetD6Joint();
	if (!pJoint ||
		!IsValidD6SwingLimit(tLimitDesc))
	{
		return false;
	}

	PxJointLimitCone tLimit{
		ToD6Radians(tLimitDesc.fYDegrees),
		ToD6Radians(tLimitDesc.fZDegrees) };
	ApplyD6LimitResponse(
		tLimit,
		tLimitDesc.tResponse,
		true);
	pJoint->setSwingLimit(tLimit);
	return true;
}

CComPxD6Joint::SWING_LIMIT_DESC
CComPxD6Joint::GetSwingLimit() const
{
	const auto* pJoint = GetD6Joint();
	if (!pJoint)
		return {};

	const PxJointLimitCone tLimit =
		pJoint->getSwingLimit();
	return {
		.fYDegrees = ToD6Degrees(tLimit.yAngle),
		.fZDegrees = ToD6Degrees(tLimit.zAngle),
		.tResponse = GetD6LimitResponse(
			tLimit,
			true)
	};
}

_float CComPxD6Joint::GetTwistAngleDegrees() const
{
	const auto* pJoint = GetD6Joint();
	return pJoint
		? ToD6Degrees(pJoint->getTwistAngle())
		: 0.f;
}

_float CComPxD6Joint::GetSwingYAngleDegrees() const
{
	const auto* pJoint = GetD6Joint();
	return pJoint
		? ToD6Degrees(pJoint->getSwingYAngle())
		: 0.f;
}

_float CComPxD6Joint::GetSwingZAngleDegrees() const
{
	const auto* pJoint = GetD6Joint();
	return pJoint
		? ToD6Degrees(pJoint->getSwingZAngle())
		: 0.f;
}

_bool CComPxD6Joint::SetAngularDriveMode(
	ANGULAR_DRIVE_MODE eMode)
{
	auto* pJoint = GetD6Joint();
	if (!pJoint ||
		!IsValidD6AngularDriveMode(eMode))
	{
		return false;
	}

	pJoint->setAngularDriveConfig(
		ToPxD6AngularDriveMode(eMode));
	return true;
}

CComPxD6Joint::ANGULAR_DRIVE_MODE
CComPxD6Joint::GetAngularDriveMode() const
{
	const auto* pJoint = GetD6Joint();
	return pJoint
		? FromPxD6AngularDriveMode(
			pJoint->getAngularDriveConfig())
		: ANGULAR_DRIVE_MODE::SWING_TWIST;
}

_bool CComPxD6Joint::SetDrive(
	DRIVE eDrive,
	const DRIVE_DESC& tDrive)
{
	auto* pJoint = GetD6Joint();
	if (!pJoint ||
		!IsValidD6Drive(eDrive) ||
		!IsValidD6DriveDesc(tDrive) ||
		!IsD6DriveCompatible(
			eDrive,
			GetAngularDriveMode()))
	{
		return false;
	}

	pJoint->setDrive(
		ToPxD6Drive(eDrive),
		ToPxD6JointDrive(tDrive));
	return true;
}

CComPxD6Joint::DRIVE_DESC CComPxD6Joint::GetDrive(
	DRIVE eDrive) const
{
	const auto* pJoint = GetD6Joint();
	if (!pJoint ||
		!IsValidD6Drive(eDrive) ||
		!IsD6DriveCompatible(
			eDrive,
			GetAngularDriveMode()))
	{
		return {};
	}

	return FromPxD6JointDrive(
		pJoint->getDrive(ToPxD6Drive(eDrive)));
}

_bool CComPxD6Joint::SetDrivePose(
	const FRAME& tPose,
	_bool bAutoWake)
{
	auto* pJoint = GetD6Joint();
	if (!pJoint ||
		!IsD6Finite(tPose.vPosition) ||
		!IsD6Finite(tPose.vRotation))
	{
		return false;
	}

	pJoint->setDrivePosition(
		ToD6PxTransform(tPose),
		bAutoWake);
	return true;
}

CComPxJoint::FRAME CComPxD6Joint::GetDrivePose() const
{
	const auto* pJoint = GetD6Joint();
	return pJoint
		? ToD6Frame(pJoint->getDrivePosition())
		: FRAME{};
}

_bool CComPxD6Joint::SetDriveVelocity(
	const _float3& vLinearVelocity,
	const _float3& vAngularVelocityDegreesPerSecond,
	_bool bAutoWake)
{
	auto* pJoint = GetD6Joint();
	if (!pJoint ||
		!IsD6Finite(vLinearVelocity) ||
		!IsD6Finite(
			vAngularVelocityDegreesPerSecond))
	{
		return false;
	}

	pJoint->setDriveVelocity(
		PxVec3{
			vLinearVelocity.x,
			vLinearVelocity.y,
			vLinearVelocity.z },
		PxVec3{
			ToD6Radians(
				vAngularVelocityDegreesPerSecond.x),
			ToD6Radians(
				vAngularVelocityDegreesPerSecond.y),
			ToD6Radians(
				vAngularVelocityDegreesPerSecond.z) },
		bAutoWake);
	return true;
}

void CComPxD6Joint::GetDriveVelocity(
	_float3& vOutLinearVelocity,
	_float3& vOutAngularVelocityDegreesPerSecond) const
{
	vOutLinearVelocity = {};
	vOutAngularVelocityDegreesPerSecond = {};

	const auto* pJoint = GetD6Joint();
	if (!pJoint)
		return;

	PxVec3 vLinear{};
	PxVec3 vAngular{};
	pJoint->getDriveVelocity(vLinear, vAngular);
	vOutLinearVelocity = {
		vLinear.x,
		vLinear.y,
		vLinear.z
	};
	vOutAngularVelocityDegreesPerSecond = {
		ToD6Degrees(vAngular.x),
		ToD6Degrees(vAngular.y),
		ToD6Degrees(vAngular.z)
	};
}

void CComPxD6Joint::UpdateGUI()
{
	CComPxJoint::UpdateGUI();

	if (!GetD6Joint())
		return;

	ImGui::Separator();
	ImGui::TextUnformatted("D6 Joint");
	ImGui::Text(
		"Twist: %.2f deg",
		GetTwistAngleDegrees());
	ImGui::Text(
		"Swing Y/Z: %.2f / %.2f deg",
		GetSwingYAngleDegrees(),
		GetSwingZAngleDegrees());

	constexpr const char* pAxisLabels[D6_AXIS_COUNT]{
		"X Motion",
		"Y Motion",
		"Z Motion",
		"Twist Motion",
		"Swing Y Motion",
		"Swing Z Motion"
	};
	constexpr const char* pMotionLabels[]{
		"Locked",
		"Limited",
		"Free"
	};

	for (size_t i = 0; i < D6_AXIS_COUNT; ++i)
	{
		const AXIS eAxis = static_cast<AXIS>(i);
		int32_t iMotion =
			static_cast<int32_t>(GetMotion(eAxis));
		if (ImGui::Combo(
			pAxisLabels[i],
			&iMotion,
			pMotionLabels,
			static_cast<int32_t>(
				std::size(pMotionLabels))))
		{
			SetMotion(
				eAxis,
				static_cast<MOTION>(iMotion));
		}
	}

	if (ImGui::CollapsingHeader("D6 Limits"))
	{
		constexpr const char* pLinearLabels[]{
			"X Linear Limit",
			"Y Linear Limit",
			"Z Linear Limit"
		};
		for (size_t i = 0; i < 3; ++i)
		{
			const AXIS eAxis = static_cast<AXIS>(i);
			LINEAR_LIMIT_DESC tLimit =
				GetLinearLimit(eAxis);
			_float vRange[2]{
				tLimit.fLower,
				tLimit.fUpper
			};
			if (ImGui::DragFloat2(
				pLinearLabels[i],
				vRange,
				0.01f))
			{
				tLimit.fLower = vRange[0];
				tLimit.fUpper = vRange[1];
				SetLinearLimit(eAxis, tLimit);
			}
		}

		TWIST_LIMIT_DESC tTwistLimit =
			GetTwistLimit();
		if (ImGui::DragFloatRange2(
			"Twist Limit (Deg)",
			&tTwistLimit.fLowerDegrees,
			&tTwistLimit.fUpperDegrees,
			1.f,
			-359.999f,
			359.999f))
		{
			SetTwistLimit(tTwistLimit);
		}

		SWING_LIMIT_DESC tSwingLimit =
			GetSwingLimit();
		_float vSwing[2]{
			tSwingLimit.fYDegrees,
			tSwingLimit.fZDegrees
		};
		if (ImGui::DragFloat2(
			"Swing Y/Z Limit (Deg)",
			vSwing,
			1.f,
			0.001f,
			179.999f))
		{
			tSwingLimit.fYDegrees = vSwing[0];
			tSwingLimit.fZDegrees = vSwing[1];
			SetSwingLimit(tSwingLimit);
		}
	}

	int32_t iAngularDriveMode =
		static_cast<int32_t>(GetAngularDriveMode());
	constexpr const char* pAngularDriveModes[]{
		"Swing / Twist",
		"SLERP"
	};
	if (ImGui::Combo(
		"Angular Drive Mode",
		&iAngularDriveMode,
		pAngularDriveModes,
		static_cast<int32_t>(
			std::size(pAngularDriveModes))))
	{
		SetAngularDriveMode(
			static_cast<ANGULAR_DRIVE_MODE>(
				iAngularDriveMode));
	}

	if (ImGui::CollapsingHeader("D6 Drives"))
	{
		constexpr const char* pDriveLabels[D6_DRIVE_COUNT]{
			"X Drive",
			"Y Drive",
			"Z Drive",
			"Twist Drive",
			"Swing Y Drive",
			"Swing Z Drive",
			"SLERP Drive"
		};
		const ANGULAR_DRIVE_MODE eMode =
			GetAngularDriveMode();

		for (size_t i = 0; i < D6_DRIVE_COUNT; ++i)
		{
			const DRIVE eDrive = static_cast<DRIVE>(i);
			if (!IsD6DriveCompatible(eDrive, eMode))
				continue;

			if (!ImGui::TreeNode(pDriveLabels[i]))
				continue;

			DRIVE_DESC tDrive = GetDrive(eDrive);
			bool bChanged{};
			bChanged |= ImGui::DragFloat(
				"Stiffness",
				&tDrive.fStiffness,
				0.1f,
				0.f,
				std::numeric_limits<_float>::max());
			bChanged |= ImGui::DragFloat(
				"Damping",
				&tDrive.fDamping,
				0.1f,
				0.f,
				std::numeric_limits<_float>::max());
			bChanged |= ImGui::DragFloat(
				"Force Limit",
				&tDrive.fForceLimit,
				1.f,
				0.f,
				std::numeric_limits<_float>::max());
			bChanged |= ImGui::Checkbox(
				"Acceleration Drive",
				&tDrive.bAcceleration);
			bChanged |= ImGui::Checkbox(
				"Output Force",
				&tDrive.bOutputForce);
			if (bChanged)
				SetDrive(eDrive, tDrive);

			ImGui::TreePop();
		}
	}

	if (ImGui::CollapsingHeader("D6 Drive Target"))
	{
		FRAME tDrivePose = GetDrivePose();
		_float vDrivePosition[3]{
			tDrivePose.vPosition.x,
			tDrivePose.vPosition.y,
			tDrivePose.vPosition.z
		};
		_float vDriveRotation[4]{
			tDrivePose.vRotation.x,
			tDrivePose.vRotation.y,
			tDrivePose.vRotation.z,
			tDrivePose.vRotation.w
		};
		bool bPoseChanged{};
		bPoseChanged |= ImGui::DragFloat3(
			"Target Position",
			vDrivePosition,
			0.01f);
		bPoseChanged |= ImGui::DragFloat4(
			"Target Rotation (Quaternion)",
			vDriveRotation,
			0.01f);
		if (bPoseChanged)
		{
			tDrivePose.vPosition = {
				vDrivePosition[0],
				vDrivePosition[1],
				vDrivePosition[2]
			};
			tDrivePose.vRotation = {
				vDriveRotation[0],
				vDriveRotation[1],
				vDriveRotation[2],
				vDriveRotation[3]
			};
			SetDrivePose(tDrivePose);
		}

		_float3 vLinearVelocity{};
		_float3 vAngularVelocity{};
		GetDriveVelocity(
			vLinearVelocity,
			vAngularVelocity);
		_float vLinear[3]{
			vLinearVelocity.x,
			vLinearVelocity.y,
			vLinearVelocity.z
		};
		_float vAngular[3]{
			vAngularVelocity.x,
			vAngularVelocity.y,
			vAngularVelocity.z
		};
		bool bVelocityChanged{};
		bVelocityChanged |= ImGui::DragFloat3(
			"Target Linear Velocity",
			vLinear,
			0.01f);
		bVelocityChanged |= ImGui::DragFloat3(
			"Target Angular Velocity (Deg/s)",
			vAngular,
			1.f);
		if (bVelocityChanged)
		{
			SetDriveVelocity(
				{ vLinear[0], vLinear[1], vLinear[2] },
				{ vAngular[0], vAngular[1], vAngular[2] });
		}
	}
}

UPtr<CComPxD6Joint> CComPxD6Joint::Create()
{
	auto pInstance = ToUPtr(new CComPxD6Joint{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CComPxD6Joint");
		return nullptr;
	}

	return pInstance;
}

UPtr<CPrototype> CComPxD6Joint::Clone(void* pArg)
{
	auto pInstance = ToUPtr(
		new CComPxD6Joint{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CComPxD6Joint");
		return nullptr;
	}

	return pInstance;
}

void CComPxD6Joint::Free()
{
	CComPxJoint::Free();
}
