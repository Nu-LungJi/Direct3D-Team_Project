#include "pch.h"
#include "TestSquareStepController.h"

#include "GameInstance.h"
#include "TestSquareStep.h"

NS_USING(Client)

CTestSquareStepController::CTestSquareStepController() = default;

CTestSquareStepController::CTestSquareStepController(
	const CTestSquareStepController& prototype)
	: CGameObject{ prototype }
{
}

HRESULT CTestSquareStepController::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc || FAILED(CGameObject::Initialize(pArg)))
		return E_INVALIDARG;
	if (pDesc->fInfluenceRadius <= 0.f ||
		pDesc->fMaxRaiseHeight < 0.f ||
		pDesc->fRaiseSpeed <= 0.f ||
		pDesc->fReturnSpeed <= 0.f ||
		pDesc->fFollowDelay < 0.f)
		return E_INVALIDARG;

	m_hTarget = pDesc->hTarget;
	m_fInfluenceRadius = pDesc->fInfluenceRadius;
	m_fMaxRaiseHeight = pDesc->fMaxRaiseHeight;
	m_fRaiseSpeed = pDesc->fRaiseSpeed;
	m_fReturnSpeed = pDesc->fReturnSpeed;
	m_fFollowDelay = pDesc->fFollowDelay;
	m_bFollowPositionInitialized = false;

	uint64_t iTotalCount{};
	switch (pDesc->ePlacement)
	{
	case PLACEMENT::GRID:
		if (pDesc->iCountX == 0 || pDesc->iCountZ == 0 ||
			pDesc->fSpacingX <= 0.f || pDesc->fSpacingZ <= 0.f)
			return E_INVALIDARG;
		iTotalCount =
			static_cast<uint64_t>(pDesc->iCountX) * pDesc->iCountZ;
		break;

	case PLACEMENT::CIRCLE:
		if (pDesc->fCircleRadius <= 0.f ||
			pDesc->fCircleSpacing <= 0.f)
			return E_INVALIDARG;
		{
			const uint64_t iHalfCount = static_cast<uint64_t>(
				std::ceil(pDesc->fCircleRadius / pDesc->fCircleSpacing));
			if (iHalfCount >
				static_cast<uint64_t>(std::numeric_limits<int32_t>::max()) ||
				iHalfCount >
				(std::numeric_limits<uint64_t>::max() - 1) / 2)
				return E_INVALIDARG;

			const uint64_t iDiameterCount = iHalfCount * 2 + 1;
			if (iDiameterCount >
				std::numeric_limits<uint64_t>::max() / iDiameterCount)
				return E_INVALIDARG;

			iTotalCount = iDiameterCount * iDiameterCount;
		}
		break;

	default:
		return E_INVALIDARG;
	}

	if (iTotalCount > std::numeric_limits<size_t>::max())
		return E_INVALIDARG;

	m_SquareStepHandles.clear();
	m_SquareStepHandles.reserve(static_cast<size_t>(iTotalCount));

	const HRESULT hr = pDesc->ePlacement == PLACEMENT::CIRCLE ?
		CreateFilledCircle(*pDesc) : CreateGrid(*pDesc);
	if (FAILED(hr))
		ClearSquareSteps();

	return hr;
}

void CTestSquareStepController::PriorityUpdate(_float fTimeDelta)
{
	const auto* pTarget =
		CGameInstance::Get().GetGameObjectByHandle(m_hTarget);
	const _float3* pTargetPosition{};
	if (pTarget)
	{
		const _float3& vTargetPosition =
			pTarget->GetTransform().GetPosition();
		if (!m_bFollowPositionInitialized)
		{
			m_vFollowPosition = vTargetPosition;
			m_bFollowPositionInitialized = true;
		}
		else if (m_fFollowDelay <= FLT_EPSILON)
		{
			m_vFollowPosition = vTargetPosition;
		}
		else
		{
			const _float fFollowAlpha = 1.f - std::exp(
				-std::max(fTimeDelta, 0.f) / m_fFollowDelay);
			m_vFollowPosition.x +=
				(vTargetPosition.x - m_vFollowPosition.x) * fFollowAlpha;
			m_vFollowPosition.z +=
				(vTargetPosition.z - m_vFollowPosition.z) * fFollowAlpha;
			m_vFollowPosition.y = vTargetPosition.y;
		}
		pTargetPosition = &m_vFollowPosition;
	}
	else
	{
		m_bFollowPositionInitialized = false;
	}

	const _float fInfluenceRadiusSq =
		m_fInfluenceRadius * m_fInfluenceRadius;

	for (const CHandle& hSquareStep : m_SquareStepHandles)
	{
		auto* pSquareStep = CGameInstance::Get()
			.GetGameObjectByHandleT<CTestSquareStep>(hSquareStep);
		if (!pSquareStep)
			continue;

		const _float3& vBasePosition =
			pSquareStep->GetBasePosition();
		_float fTargetY = vBasePosition.y;

		if (pTargetPosition)
		{
			const _float fDeltaX =
				pTargetPosition->x - vBasePosition.x;
			const _float fDeltaZ =
				pTargetPosition->z - vBasePosition.z;
			const _float fDistanceSq =
				fDeltaX * fDeltaX + fDeltaZ * fDeltaZ;

			if (fDistanceSq < fInfluenceRadiusSq)
			{
				const _float fWeight = 1.f -
					std::sqrt(fDistanceSq) / m_fInfluenceRadius;
				const _float fSmoothWeight =
					fWeight * fWeight * (3.f - 2.f * fWeight);
				fTargetY +=
					m_fMaxRaiseHeight * fSmoothWeight;
			}
		}

		const _float fCurrentY =
			pSquareStep->GetTransform().GetPosition().y;
		const _float fMoveSpeed =
			fTargetY >= fCurrentY ?
			m_fRaiseSpeed : m_fReturnSpeed;
		pSquareStep->SetHeightTarget(
			fTargetY, fMoveSpeed);
	}
}

HRESULT CTestSquareStepController::CreateGrid(const DESC& Desc)
{
	for (uint32_t z = 0; z < Desc.iCountZ; ++z)
	{
		for (uint32_t x = 0; x < Desc.iCountX; ++x)
		{
			const _float3 vPosition{
				Desc.vOrigin.x + static_cast<_float>(x) * Desc.fSpacingX,
				Desc.vOrigin.y,
				Desc.vOrigin.z + static_cast<_float>(z) * Desc.fSpacingZ
			};
			if (FAILED(CreateSquareStep(
				vPosition, Desc.bEnablePhysics)))
				return E_FAIL;
		}
	}

	return S_OK;
}

HRESULT CTestSquareStepController::CreateFilledCircle(const DESC& Desc)
{
	const int32_t iHalfCount = static_cast<int32_t>(
		std::ceil(Desc.fCircleRadius / Desc.fCircleSpacing));
	const _float fRadiusSq =
		Desc.fCircleRadius * Desc.fCircleRadius;

	for (int32_t z = -iHalfCount; z <= iHalfCount; ++z)
	{
		const _float fZ =
			static_cast<_float>(z) * Desc.fCircleSpacing;

		for (int32_t x = -iHalfCount; x <= iHalfCount; ++x)
		{
			const _float fX =
				static_cast<_float>(x) * Desc.fCircleSpacing;
			if (fX * fX + fZ * fZ > fRadiusSq)
				continue;

			const _float3 vPosition{
				Desc.vOrigin.x + fX,
				Desc.vOrigin.y,
				Desc.vOrigin.z + fZ
			};
			if (FAILED(CreateSquareStep(
				vPosition, Desc.bEnablePhysics)))
				return E_FAIL;
		}
	}

	return S_OK;
}

HRESULT CTestSquareStepController::CreateSquareStep(
	const _float3& vPosition, _bool bEnablePhysics)
{
	CTestSquareStep::DESC Desc{};
	Desc.sObjectTag = "TestSquareStep";
	Desc.vInitialPosition = vPosition;
	Desc.bEnablePhysics = bEnablePhysics;

	const auto hSquareStep = CGameInstance::Get().AddGameObjectToLayer(
		"LEVEL_CREATURE",
		"Prototype_GameObject_TestSquareStep",
		"03_PhysXTest",
		&Desc);
	if (!hSquareStep)
		return E_FAIL;

	m_SquareStepHandles.push_back(*hSquareStep);
	return S_OK;
}

void CTestSquareStepController::ClearSquareSteps()
{
	for (const CHandle& hSpawned : m_SquareStepHandles)
	{
		if (auto* pObject =
			CGameInstance::Get().GetGameObjectByHandle(hSpawned))
			pObject->SetPendingDestroyCascade();
	}
	m_SquareStepHandles.clear();
}

UPtr<CTestSquareStepController> CTestSquareStepController::Create()
{
	auto pInstance = ToUPtr(new CTestSquareStepController{});
	if (FAILED(pInstance->InitializePrototype()))
		return nullptr;
	return pInstance;
}

UPtr<CPrototype> CTestSquareStepController::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CTestSquareStepController{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
		return nullptr;
	return pInstance;
}
