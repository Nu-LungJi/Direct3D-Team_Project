#include "pch.h"
#include "ComPxRevoluteJoint.h"
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
	constexpr _float REVOLUTE_LIMIT_DEGREES = 360.f;

	_float ToRadians(_float fDegrees)
	{
		return XMConvertToRadians(fDegrees);
	}

	_float ToDegrees(_float fRadians)
	{
		return XMConvertToDegrees(fRadians);
	}

	PxQuat ToRevoluteJointNormalizedQuat(
		const _float4& vRotation)
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

	PxTransform ToRevoluteJointPxTransform(
		const CComPxJoint::FRAME& tFrame)
	{
		return PxTransform{
			PxVec3{
				tFrame.vPosition.x,
				tFrame.vPosition.y,
				tFrame.vPosition.z },
			ToRevoluteJointNormalizedQuat(tFrame.vRotation) };
	}

	CComPxJoint::FRAME ToRevoluteJointFrame(
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

	void PreserveRevoluteJointCurrentPose(
		CComPxJoint::DESC& tDesc)
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
				ToRevoluteJointFrame(tPoseB.getInverse() * tPoseA);
		}
		else if (pActorA)
		{
			tDesc.tLocalFrameA = {};
			tDesc.tLocalFrameB =
				ToRevoluteJointFrame(pActorA->getGlobalPose());
		}
		else if (pActorB)
		{
			tDesc.tLocalFrameA =
				ToRevoluteJointFrame(pActorB->getGlobalPose());
			tDesc.tLocalFrameB = {};
		}
	}

	_bool IsValidLimitRange(
		_float fLowerDegrees,
		_float fUpperDegrees)
	{
		return std::isfinite(fLowerDegrees) &&
			std::isfinite(fUpperDegrees) &&
			fLowerDegrees > -REVOLUTE_LIMIT_DEGREES &&
			fUpperDegrees < REVOLUTE_LIMIT_DEGREES &&
			fLowerDegrees <= fUpperDegrees;
	}

	_bool IsValidLimitResponse(
		_float fRestitution,
		_float fBounceThresholdDegreesPerSecond,
		_float fStiffness,
		_float fDamping)
	{
		return std::isfinite(fRestitution) &&
			std::isfinite(fBounceThresholdDegreesPerSecond) &&
			std::isfinite(fStiffness) &&
			std::isfinite(fDamping) &&
			fRestitution >= 0.f &&
			fRestitution <= 1.f &&
			fBounceThresholdDegreesPerSecond >= 0.f &&
			fStiffness >= 0.f &&
			fDamping >= 0.f;
	}

	_bool IsValidRevoluteJointDesc(
		const CComPxRevoluteJoint::DESC& tDesc)
	{
		return IsValidLimitRange(
			tDesc.fLowerLimitDegrees,
			tDesc.fUpperLimitDegrees) &&
			IsValidLimitResponse(
				tDesc.fLimitRestitution,
				tDesc.fLimitBounceThresholdDegreesPerSecond,
				tDesc.fLimitStiffness,
				tDesc.fLimitDamping) &&
			std::isfinite(
				tDesc.fDriveVelocityDegreesPerSecond) &&
			std::isfinite(tDesc.fDriveForceLimit) &&
			std::isfinite(tDesc.fDriveGearRatio) &&
			tDesc.fDriveForceLimit >= 0.f &&
			tDesc.fDriveGearRatio > 0.f;
	}

	PxJointAngularLimitPair CreateAngularLimit(
		_float fLowerDegrees,
		_float fUpperDegrees,
		_float fRestitution,
		_float fBounceThresholdDegreesPerSecond,
		_float fStiffness,
		_float fDamping)
	{
		PxJointAngularLimitPair tLimit{
			ToRadians(fLowerDegrees),
			ToRadians(fUpperDegrees) };
		tLimit.restitution = fRestitution;
		tLimit.bounceThreshold =
			ToRadians(fBounceThresholdDegreesPerSecond);
		tLimit.stiffness = fStiffness;
		tLimit.damping = fDamping;
		return tLimit;
	}
}

CComPxRevoluteJoint::CComPxRevoluteJoint()
{
}

CComPxRevoluteJoint::CComPxRevoluteJoint(
	const CComPxRevoluteJoint& Prototype)
	: CComPxJoint{ Prototype }
{
}

CComPxRevoluteJoint::~CComPxRevoluteJoint()
{
}

HRESULT CComPxRevoluteJoint::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc || !IsValidRevoluteJointDesc(*pDesc))
		return E_FAIL;

	DESC tJointDesc = *pDesc;
	if (tJointDesc.bPreserveCurrentPose)
		PreserveRevoluteJointCurrentPose(tJointDesc);

	if (FAILED(CComPxJoint::Initialize(&tJointDesc)))
		return E_FAIL;

	auto* pPhysics = CGameInstance::Get().PxGetPhysics();
	if (!pPhysics)
		return E_FAIL;

	PxRevoluteJoint* pJoint = PxRevoluteJointCreate(
		*pPhysics,
		GetActorA(),
		ToRevoluteJointPxTransform(GetLocalFrameA()),
		GetActorB(),
		ToRevoluteJointPxTransform(GetLocalFrameB()));
	if (!pJoint)
		return E_FAIL;

	pJoint->setLimit(CreateAngularLimit(
		tJointDesc.fLowerLimitDegrees,
		tJointDesc.fUpperLimitDegrees,
		tJointDesc.fLimitRestitution,
		tJointDesc.fLimitBounceThresholdDegreesPerSecond,
		tJointDesc.fLimitStiffness,
		tJointDesc.fLimitDamping));
	pJoint->setDriveVelocity(
		ToRadians(tJointDesc.fDriveVelocityDegreesPerSecond));
	pJoint->setDriveForceLimit(
		tJointDesc.fDriveForceLimit);
	pJoint->setDriveGearRatio(
		tJointDesc.fDriveGearRatio);

	PxRevoluteJointFlags tFlags{};
	if (tJointDesc.bLimitEnabled)
		tFlags |= PxRevoluteJointFlag::eLIMIT_ENABLED;
	if (tJointDesc.bDriveEnabled)
		tFlags |= PxRevoluteJointFlag::eDRIVE_ENABLED;
	if (tJointDesc.bDriveFreeSpin)
		tFlags |= PxRevoluteJointFlag::eDRIVE_FREESPIN;
	pJoint->setRevoluteJointFlags(tFlags);

	if (!AttachJoint(pJoint))
	{
		pJoint->release();
		return E_FAIL;
	}

	return S_OK;
}

PxRevoluteJoint* CComPxRevoluteJoint::GetRevoluteJoint() const
{
	return static_cast<PxRevoluteJoint*>(GetJoint());
}

_float CComPxRevoluteJoint::GetAngleDegrees() const
{
	const auto* pJoint = GetRevoluteJoint();
	return pJoint ? ToDegrees(pJoint->getAngle()) : 0.f;
}

_float CComPxRevoluteJoint::GetVelocityDegreesPerSecond() const
{
	const auto* pJoint = GetRevoluteJoint();
	return pJoint ? ToDegrees(pJoint->getVelocity()) : 0.f;
}

_float CComPxRevoluteJoint::GetLowerLimitDegrees() const
{
	const auto* pJoint = GetRevoluteJoint();
	return pJoint ? ToDegrees(pJoint->getLimit().lower) : 0.f;
}

_float CComPxRevoluteJoint::GetUpperLimitDegrees() const
{
	const auto* pJoint = GetRevoluteJoint();
	return pJoint ? ToDegrees(pJoint->getLimit().upper) : 0.f;
}

_float CComPxRevoluteJoint::GetLimitRestitution() const
{
	const auto* pJoint = GetRevoluteJoint();
	return pJoint ? pJoint->getLimit().restitution : 0.f;
}

_float CComPxRevoluteJoint::
GetLimitBounceThresholdDegreesPerSecond() const
{
	const auto* pJoint = GetRevoluteJoint();
	return pJoint
		? ToDegrees(pJoint->getLimit().bounceThreshold)
		: 0.f;
}

_float CComPxRevoluteJoint::GetLimitStiffness() const
{
	const auto* pJoint = GetRevoluteJoint();
	return pJoint ? pJoint->getLimit().stiffness : 0.f;
}

_float CComPxRevoluteJoint::GetLimitDamping() const
{
	const auto* pJoint = GetRevoluteJoint();
	return pJoint ? pJoint->getLimit().damping : 0.f;
}

_bool CComPxRevoluteJoint::SetLimit(
	_float fLowerDegrees,
	_float fUpperDegrees)
{
	auto* pJoint = GetRevoluteJoint();
	if (!pJoint ||
		!IsValidLimitRange(fLowerDegrees, fUpperDegrees))
	{
		return false;
	}

	PxJointAngularLimitPair tLimit = pJoint->getLimit();
	tLimit.lower = ToRadians(fLowerDegrees);
	tLimit.upper = ToRadians(fUpperDegrees);
	pJoint->setLimit(tLimit);
	return true;
}

_bool CComPxRevoluteJoint::SetLimitResponse(
	_float fRestitution,
	_float fBounceThresholdDegreesPerSecond,
	_float fStiffness,
	_float fDamping)
{
	auto* pJoint = GetRevoluteJoint();
	if (!pJoint ||
		!IsValidLimitResponse(
			fRestitution,
			fBounceThresholdDegreesPerSecond,
			fStiffness,
			fDamping))
	{
		return false;
	}

	PxJointAngularLimitPair tLimit = pJoint->getLimit();
	tLimit.restitution = fRestitution;
	tLimit.bounceThreshold =
		ToRadians(fBounceThresholdDegreesPerSecond);
	tLimit.stiffness = fStiffness;
	tLimit.damping = fDamping;
	pJoint->setLimit(tLimit);
	return true;
}

_float CComPxRevoluteJoint::
GetDriveVelocityDegreesPerSecond() const
{
	const auto* pJoint = GetRevoluteJoint();
	return pJoint
		? ToDegrees(pJoint->getDriveVelocity())
		: 0.f;
}

_float CComPxRevoluteJoint::GetDriveForceLimit() const
{
	const auto* pJoint = GetRevoluteJoint();
	return pJoint ? pJoint->getDriveForceLimit() : 0.f;
}

_float CComPxRevoluteJoint::GetDriveGearRatio() const
{
	const auto* pJoint = GetRevoluteJoint();
	return pJoint ? pJoint->getDriveGearRatio() : 0.f;
}

_bool CComPxRevoluteJoint::SetDriveVelocity(
	_float fDegreesPerSecond,
	_bool bAutoWake)
{
	auto* pJoint = GetRevoluteJoint();
	if (!pJoint || !std::isfinite(fDegreesPerSecond))
		return false;

	pJoint->setDriveVelocity(
		ToRadians(fDegreesPerSecond),
		bAutoWake);
	return true;
}

_bool CComPxRevoluteJoint::SetDriveForceLimit(
	_float fForceLimit)
{
	auto* pJoint = GetRevoluteJoint();
	if (!pJoint ||
		!std::isfinite(fForceLimit) ||
		fForceLimit < 0.f)
	{
		return false;
	}

	pJoint->setDriveForceLimit(fForceLimit);
	return true;
}

_bool CComPxRevoluteJoint::SetDriveGearRatio(
	_float fGearRatio)
{
	auto* pJoint = GetRevoluteJoint();
	if (!pJoint ||
		!std::isfinite(fGearRatio) ||
		fGearRatio <= 0.f)
	{
		return false;
	}

	pJoint->setDriveGearRatio(fGearRatio);
	return true;
}

_bool CComPxRevoluteJoint::SetLimitEnabled(
	_bool bEnabled)
{
	auto* pJoint = GetRevoluteJoint();
	if (!pJoint)
		return false;

	pJoint->setRevoluteJointFlag(
		PxRevoluteJointFlag::eLIMIT_ENABLED,
		bEnabled);
	return true;
}

_bool CComPxRevoluteJoint::IsLimitEnabled() const
{
	const auto* pJoint = GetRevoluteJoint();
	return pJoint &&
		pJoint->getRevoluteJointFlags().isSet(
			PxRevoluteJointFlag::eLIMIT_ENABLED);
}

_bool CComPxRevoluteJoint::SetDriveEnabled(
	_bool bEnabled)
{
	auto* pJoint = GetRevoluteJoint();
	if (!pJoint)
		return false;

	pJoint->setRevoluteJointFlag(
		PxRevoluteJointFlag::eDRIVE_ENABLED,
		bEnabled);
	return true;
}

_bool CComPxRevoluteJoint::IsDriveEnabled() const
{
	const auto* pJoint = GetRevoluteJoint();
	return pJoint &&
		pJoint->getRevoluteJointFlags().isSet(
			PxRevoluteJointFlag::eDRIVE_ENABLED);
}

_bool CComPxRevoluteJoint::SetDriveFreeSpin(
	_bool bEnabled)
{
	auto* pJoint = GetRevoluteJoint();
	if (!pJoint)
		return false;

	pJoint->setRevoluteJointFlag(
		PxRevoluteJointFlag::eDRIVE_FREESPIN,
		bEnabled);
	return true;
}

_bool CComPxRevoluteJoint::IsDriveFreeSpin() const
{
	const auto* pJoint = GetRevoluteJoint();
	return pJoint &&
		pJoint->getRevoluteJointFlags().isSet(
			PxRevoluteJointFlag::eDRIVE_FREESPIN);
}

void CComPxRevoluteJoint::UpdateGUI()
{
	CComPxJoint::UpdateGUI();

	if (!GetRevoluteJoint())
		return;

	ImGui::Separator();
	ImGui::TextUnformatted("Revolute Joint");
	ImGui::Text(
		"Angle: %.3f deg",
		GetAngleDegrees());
	ImGui::Text(
		"Velocity: %.3f deg/s",
		GetVelocityDegreesPerSecond());
	ImGui::TextUnformatted(
		"Hinge Axis: Local Joint Frame +X");

	bool bLimitEnabled = IsLimitEnabled();
	if (ImGui::Checkbox("Limit Enabled", &bLimitEnabled))
		SetLimitEnabled(bLimitEnabled);

	_float fLowerDegrees = GetLowerLimitDegrees();
	_float fUpperDegrees = GetUpperLimitDegrees();
	if (ImGui::DragFloatRange2(
		"Angular Limit (Deg)",
		&fLowerDegrees,
		&fUpperDegrees,
		1.f,
		-359.999f,
		359.999f))
	{
		SetLimit(fLowerDegrees, fUpperDegrees);
	}

	_float fRestitution = GetLimitRestitution();
	_float fBounceThreshold =
		GetLimitBounceThresholdDegreesPerSecond();
	_float fStiffness = GetLimitStiffness();
	_float fDamping = GetLimitDamping();
	bool bLimitResponseChanged{};
	bLimitResponseChanged |= ImGui::DragFloat(
		"Limit Restitution",
		&fRestitution,
		0.01f,
		0.f,
		1.f);
	bLimitResponseChanged |= ImGui::DragFloat(
		"Limit Bounce Threshold (Deg/s)",
		&fBounceThreshold,
		1.f,
		0.f,
		std::numeric_limits<_float>::max());
	bLimitResponseChanged |= ImGui::DragFloat(
		"Limit Stiffness",
		&fStiffness,
		0.1f,
		0.f,
		std::numeric_limits<_float>::max());
	bLimitResponseChanged |= ImGui::DragFloat(
		"Limit Damping",
		&fDamping,
		0.1f,
		0.f,
		std::numeric_limits<_float>::max());
	if (bLimitResponseChanged)
	{
		SetLimitResponse(
			fRestitution,
			fBounceThreshold,
			fStiffness,
			fDamping);
	}

	bool bDriveEnabled = IsDriveEnabled();
	if (ImGui::Checkbox("Drive Enabled", &bDriveEnabled))
		SetDriveEnabled(bDriveEnabled);

	bool bDriveFreeSpin = IsDriveFreeSpin();
	if (ImGui::Checkbox("Drive Free Spin", &bDriveFreeSpin))
		SetDriveFreeSpin(bDriveFreeSpin);

	_float fDriveVelocity =
		GetDriveVelocityDegreesPerSecond();
	if (ImGui::DragFloat(
		"Drive Velocity (Deg/s)",
		&fDriveVelocity,
		1.f))
	{
		SetDriveVelocity(fDriveVelocity);
	}

	_float fDriveForceLimit = GetDriveForceLimit();
	if (ImGui::DragFloat(
		"Drive Force Limit",
		&fDriveForceLimit,
		1.f,
		0.f,
		std::numeric_limits<_float>::max()))
	{
		SetDriveForceLimit(fDriveForceLimit);
	}

	_float fDriveGearRatio = GetDriveGearRatio();
	if (ImGui::DragFloat(
		"Drive Gear Ratio",
		&fDriveGearRatio,
		0.01f,
		0.01f,
		std::numeric_limits<_float>::max()))
	{
		SetDriveGearRatio(fDriveGearRatio);
	}
}

UPtr<CComPxRevoluteJoint> CComPxRevoluteJoint::Create()
{
	auto pInstance = ToUPtr(new CComPxRevoluteJoint{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CComPxRevoluteJoint");
		return nullptr;
	}

	return pInstance;
}

UPtr<CPrototype> CComPxRevoluteJoint::Clone(void* pArg)
{
	auto pInstance = ToUPtr(
		new CComPxRevoluteJoint{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CComPxRevoluteJoint");
		return nullptr;
	}

	return pInstance;
}

void CComPxRevoluteJoint::Free()
{
	CComPxJoint::Free();
}
