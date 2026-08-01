#include "pch.h"
#include "ComPxDistanceJoint.h"

#pragma push_macro("new")
#undef new

#include "PxPhysicsAPI.h"

#pragma pop_macro("new")

using namespace physx;

NS_USING(Engine)

namespace
{
	PxQuat ToDistanceJointNormalizedQuat(
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

	PxTransform ToDistanceJointPxTransform(
		const CComPxJoint::FRAME& tFrame)
	{
		return PxTransform{
			PxVec3{
				tFrame.vPosition.x,
				tFrame.vPosition.y,
				tFrame.vPosition.z },
			ToDistanceJointNormalizedQuat(tFrame.vRotation) };
	}

	_bool IsValidDistanceJointDesc(
		const CComPxDistanceJoint::DESC& tDesc)
	{
		return std::isfinite(tDesc.fMinDistance) &&
			std::isfinite(tDesc.fMaxDistance) &&
			std::isfinite(tDesc.fTolerance) &&
			std::isfinite(tDesc.fStiffness) &&
			std::isfinite(tDesc.fDamping) &&
			tDesc.fMinDistance >= 0.f &&
			tDesc.fMaxDistance >= tDesc.fMinDistance &&
			tDesc.fTolerance > 0.f &&
			tDesc.fStiffness >= 0.f &&
			tDesc.fDamping >= 0.f;
	}
}

CComPxDistanceJoint::CComPxDistanceJoint()
{
}

CComPxDistanceJoint::CComPxDistanceJoint(
	const CComPxDistanceJoint& Prototype)
	: CComPxJoint{ Prototype }
{
}

CComPxDistanceJoint::~CComPxDistanceJoint()
{
}

HRESULT CComPxDistanceJoint::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc || !IsValidDistanceJointDesc(*pDesc))
		return E_FAIL;

	if (FAILED(CComPxJoint::Initialize(pArg)))
		return E_FAIL;

	auto* pPhysics = CGameInstance::Get().PxGetPhysics();
	if (!pPhysics)
		return E_FAIL;

	PxDistanceJoint* pJoint = PxDistanceJointCreate(
		*pPhysics,
		GetActorA(),
		ToDistanceJointPxTransform(GetLocalFrameA()),
		GetActorB(),
		ToDistanceJointPxTransform(GetLocalFrameB()));
	if (!pJoint)
		return E_FAIL;

	pJoint->setMaxDistance(pDesc->fMaxDistance);
	pJoint->setMinDistance(pDesc->fMinDistance);
	pJoint->setTolerance(pDesc->fTolerance);
	pJoint->setStiffness(pDesc->fStiffness);
	pJoint->setDamping(pDesc->fDamping);

	PxDistanceJointFlags tFlags{};
	if (pDesc->bMinDistanceEnabled)
		tFlags |= PxDistanceJointFlag::eMIN_DISTANCE_ENABLED;
	if (pDesc->bMaxDistanceEnabled)
		tFlags |= PxDistanceJointFlag::eMAX_DISTANCE_ENABLED;
	if (pDesc->bSpringEnabled)
		tFlags |= PxDistanceJointFlag::eSPRING_ENABLED;
	pJoint->setDistanceJointFlags(tFlags);

	if (!AttachJoint(pJoint))
	{
		pJoint->release();
		return E_FAIL;
	}

	return S_OK;
}

PxDistanceJoint* CComPxDistanceJoint::GetDistanceJoint() const
{
	return static_cast<PxDistanceJoint*>(GetJoint());
}

_float CComPxDistanceJoint::GetDistance() const
{
	const auto* pJoint = GetDistanceJoint();
	return pJoint ? pJoint->getDistance() : 0.f;
}

_float CComPxDistanceJoint::GetMinDistance() const
{
	const auto* pJoint = GetDistanceJoint();
	return pJoint ? pJoint->getMinDistance() : 0.f;
}

_float CComPxDistanceJoint::GetMaxDistance() const
{
	const auto* pJoint = GetDistanceJoint();
	return pJoint ? pJoint->getMaxDistance() : 0.f;
}

_float CComPxDistanceJoint::GetTolerance() const
{
	const auto* pJoint = GetDistanceJoint();
	return pJoint ? pJoint->getTolerance() : 0.f;
}

_float CComPxDistanceJoint::GetStiffness() const
{
	const auto* pJoint = GetDistanceJoint();
	return pJoint ? pJoint->getStiffness() : 0.f;
}

_float CComPxDistanceJoint::GetDamping() const
{
	const auto* pJoint = GetDistanceJoint();
	return pJoint ? pJoint->getDamping() : 0.f;
}

_bool CComPxDistanceJoint::SetDistanceRange(
	_float fMinDistance,
	_float fMaxDistance)
{
	auto* pJoint = GetDistanceJoint();
	if (!pJoint ||
		!std::isfinite(fMinDistance) ||
		!std::isfinite(fMaxDistance) ||
		fMinDistance < 0.f ||
		fMaxDistance < fMinDistance)
	{
		return false;
	}

	const _float fCurrentMin = pJoint->getMinDistance();
	const _float fCurrentMax = pJoint->getMaxDistance();
	if (fMinDistance > fCurrentMax)
	{
		pJoint->setMaxDistance(fMaxDistance);
		pJoint->setMinDistance(fMinDistance);
	}
	else if (fMaxDistance < fCurrentMin)
	{
		pJoint->setMinDistance(fMinDistance);
		pJoint->setMaxDistance(fMaxDistance);
	}
	else
	{
		pJoint->setMinDistance(fMinDistance);
		pJoint->setMaxDistance(fMaxDistance);
	}

	return true;
}

_bool CComPxDistanceJoint::SetMinDistance(_float fDistance)
{
	auto* pJoint = GetDistanceJoint();
	if (!pJoint ||
		!std::isfinite(fDistance) ||
		fDistance < 0.f ||
		fDistance > pJoint->getMaxDistance())
	{
		return false;
	}

	pJoint->setMinDistance(fDistance);
	return true;
}

_bool CComPxDistanceJoint::SetMaxDistance(_float fDistance)
{
	auto* pJoint = GetDistanceJoint();
	if (!pJoint ||
		!std::isfinite(fDistance) ||
		fDistance < pJoint->getMinDistance())
	{
		return false;
	}

	pJoint->setMaxDistance(fDistance);
	return true;
}

_bool CComPxDistanceJoint::SetTolerance(_float fTolerance)
{
	auto* pJoint = GetDistanceJoint();
	if (!pJoint ||
		!std::isfinite(fTolerance) ||
		fTolerance <= 0.f)
	{
		return false;
	}

	pJoint->setTolerance(fTolerance);
	return true;
}

_bool CComPxDistanceJoint::SetSpring(
	_float fStiffness,
	_float fDamping)
{
	auto* pJoint = GetDistanceJoint();
	if (!pJoint ||
		!std::isfinite(fStiffness) ||
		!std::isfinite(fDamping) ||
		fStiffness < 0.f ||
		fDamping < 0.f)
	{
		return false;
	}

	pJoint->setStiffness(fStiffness);
	pJoint->setDamping(fDamping);
	return true;
}

_bool CComPxDistanceJoint::SetMinDistanceEnabled(
	_bool bEnabled)
{
	auto* pJoint = GetDistanceJoint();
	if (!pJoint)
		return false;

	pJoint->setDistanceJointFlag(
		PxDistanceJointFlag::eMIN_DISTANCE_ENABLED,
		bEnabled);
	return true;
}

_bool CComPxDistanceJoint::IsMinDistanceEnabled() const
{
	const auto* pJoint = GetDistanceJoint();
	return pJoint &&
		pJoint->getDistanceJointFlags().isSet(
			PxDistanceJointFlag::eMIN_DISTANCE_ENABLED);
}

_bool CComPxDistanceJoint::SetMaxDistanceEnabled(
	_bool bEnabled)
{
	auto* pJoint = GetDistanceJoint();
	if (!pJoint)
		return false;

	pJoint->setDistanceJointFlag(
		PxDistanceJointFlag::eMAX_DISTANCE_ENABLED,
		bEnabled);
	return true;
}

_bool CComPxDistanceJoint::IsMaxDistanceEnabled() const
{
	const auto* pJoint = GetDistanceJoint();
	return pJoint &&
		pJoint->getDistanceJointFlags().isSet(
			PxDistanceJointFlag::eMAX_DISTANCE_ENABLED);
}

_bool CComPxDistanceJoint::SetSpringEnabled(
	_bool bEnabled)
{
	auto* pJoint = GetDistanceJoint();
	if (!pJoint)
		return false;

	pJoint->setDistanceJointFlag(
		PxDistanceJointFlag::eSPRING_ENABLED,
		bEnabled);
	return true;
}

_bool CComPxDistanceJoint::IsSpringEnabled() const
{
	const auto* pJoint = GetDistanceJoint();
	return pJoint &&
		pJoint->getDistanceJointFlags().isSet(
			PxDistanceJointFlag::eSPRING_ENABLED);
}

void CComPxDistanceJoint::UpdateGUI()
{
	CComPxJoint::UpdateGUI();

	auto* pJoint = GetDistanceJoint();
	if (!pJoint)
		return;

	ImGui::Separator();
	ImGui::TextUnformatted("Distance Joint");
	ImGui::Text("Current Distance: %.3f", GetDistance());

	bool bMinEnabled = IsMinDistanceEnabled();
	if (ImGui::Checkbox("Min Distance Enabled", &bMinEnabled))
		SetMinDistanceEnabled(bMinEnabled);

	bool bMaxEnabled = IsMaxDistanceEnabled();
	if (ImGui::Checkbox("Max Distance Enabled", &bMaxEnabled))
		SetMaxDistanceEnabled(bMaxEnabled);

	bool bSpringEnabled = IsSpringEnabled();
	if (ImGui::Checkbox("Spring Enabled", &bSpringEnabled))
		SetSpringEnabled(bSpringEnabled);

	_float fMinDistance = GetMinDistance();
	_float fMaxDistance = GetMaxDistance();
	bool bRangeChanged{};
	bRangeChanged |= ImGui::DragFloat(
		"Min Distance",
		&fMinDistance,
		0.01f,
		0.f,
		fMaxDistance);
	bRangeChanged |= ImGui::DragFloat(
		"Max Distance",
		&fMaxDistance,
		0.01f,
		fMinDistance,
		std::numeric_limits<_float>::max());
	if (bRangeChanged)
		SetDistanceRange(fMinDistance, fMaxDistance);

	_float fTolerance = GetTolerance();
	if (ImGui::DragFloat(
		"Distance Tolerance",
		&fTolerance,
		0.001f,
		0.001f,
		std::numeric_limits<_float>::max()))
	{
		SetTolerance(fTolerance);
	}

	_float fStiffness = GetStiffness();
	_float fDamping = GetDamping();
	bool bSpringChanged{};
	bSpringChanged |= ImGui::DragFloat(
		"Spring Stiffness",
		&fStiffness,
		0.1f,
		0.f,
		std::numeric_limits<_float>::max());
	bSpringChanged |= ImGui::DragFloat(
		"Spring Damping",
		&fDamping,
		0.1f,
		0.f,
		std::numeric_limits<_float>::max());
	if (bSpringChanged)
		SetSpring(fStiffness, fDamping);
}

UPtr<CComPxDistanceJoint> CComPxDistanceJoint::Create()
{
	auto pInstance = ToUPtr(new CComPxDistanceJoint{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CComPxDistanceJoint");
		return nullptr;
	}

	return pInstance;
}

UPtr<CPrototype> CComPxDistanceJoint::Clone(void* pArg)
{
	auto pInstance = ToUPtr(
		new CComPxDistanceJoint{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CComPxDistanceJoint");
		return nullptr;
	}

	return pInstance;
}

void CComPxDistanceJoint::Free()
{
	CComPxJoint::Free();
}
