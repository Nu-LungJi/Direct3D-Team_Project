#include "pch.h"
#include "MagicSquareStepController.h"

#include "GameInstance.h"
#include "MagicSquareStep.h"

NS_USING(Client)

CMagicSquareStepController::CMagicSquareStepController() = default;

CMagicSquareStepController::CMagicSquareStepController(
	const CMagicSquareStepController& prototype)
	: CGameObject{ prototype }
{
}

HRESULT CMagicSquareStepController::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc || FAILED(ValidateDesc(*pDesc)) ||
		FAILED(CGameObject::Initialize(pArg)))
		return E_INVALIDARG;

	m_ePattern = pDesc->ePattern;
	m_tCommon = pDesc->tCommon;
	m_tCombatCircle = pDesc->tCombatCircle;
	m_tTargetBridge = pDesc->tTargetBridge;
	m_tPlayerWaveBridge = pDesc->tPlayerWaveBridge;

	if (FAILED(CreatePatternSteps(*pDesc)))
	{
		ClearSteps();
		return E_FAIL;
	}

	return S_OK;
}

void CMagicSquareStepController::PriorityUpdate(
	_float fTimeDelta)
{
	m_fStateTime += std::max(fTimeDelta, 0.f);

	switch (m_eState)
	{
	case STATE::SUMMONING:
		UpdateSummoning();
		break;
	case STATE::PATTERN_TRANSITION:
		UpdatePatternTransition();
		break;
	case STATE::RUNNING:
		UpdateDynamicPattern(fTimeDelta);
		break;
	case STATE::DISAPPEARING:
		UpdateDisappearing();
		break;
	default:
		break;
	}
}

void CMagicSquareStepController::UpdateGUI()
{
	CGameObject::UpdateGUI();
	ImGui::Text("Pattern: %u", ETOUI(m_ePattern));
	ImGui::Text("State: %u", ETOUI(m_eState));
	ImGui::Text("Steps: %zu", m_Steps.size());
	if (ImGui::Button("Activate"))
		Activate();
	ImGui::SameLine();
	if (ImGui::Button("Deactivate"))
		Deactivate();
}

void CMagicSquareStepController::Activate()
{
	if (m_eState == STATE::SUMMONING ||
		m_eState == STATE::PATTERN_TRANSITION ||
		m_eState == STATE::RUNNING)
		return;

	m_eState = STATE::SUMMONING;
	m_fStateTime = 0.f;
	m_fStateDuration =
		m_tCommon.fSummonWaveDelay +
		m_tCommon.fHiddenDepth / m_tCommon.fSummonSpeed;
}

void CMagicSquareStepController::Deactivate()
{
	if (m_eState == STATE::DORMANT ||
		m_eState == STATE::DISAPPEARING)
		return;

	m_eState = STATE::DISAPPEARING;
	m_fStateTime = 0.f;
	_float fMaxDistance = m_tCommon.fHiddenDepth;
	for (const STEP_DATA& Step : m_Steps)
	{
		const auto* pStep = CGameInstance::Get()
			.GetGameObjectByHandleT<CMagicSquareStep>(Step.hStep);
		if (!pStep)
			continue;
		const _vector vDelta =
			XMLoadFloat3(&Step.vHiddenPosition) -
			XMLoadFloat3(&pStep->GetCurrentPosition());
		fMaxDistance = std::max(
			fMaxDistance,
			XMVectorGetX(XMVector3Length(vDelta)));
	}
	m_fStateDuration =
		m_tCommon.fSummonWaveDelay +
		fMaxDistance / m_tCommon.fSummonSpeed;
}

HRESULT CMagicSquareStepController::ValidateDesc(
	const DESC& Desc) const
{
	if (Desc.tCommon.fSpacing <= 0.f ||
		Desc.tCommon.fHiddenDepth <= 0.f ||
		Desc.tCommon.fSummonSpeed <= 0.f ||
		Desc.tCommon.fSummonWaveDelay < 0.f ||
		Desc.tCommon.fPatternMoveSpeed <= 0.f)
		return E_INVALIDARG;

	switch (Desc.ePattern)
	{
	case PATTERN::COMBAT_CIRCLE:
		return Desc.tCombatCircle.fRadius > 0.f &&
			Desc.tCombatCircle.fInfluenceRadius > 0.f &&
			Desc.tCombatCircle.fRaiseHeight >= 0.f &&
			Desc.tCombatCircle.fFollowDelay >= 0.f ?
			S_OK : E_INVALIDARG;
	case PATTERN::TARGET_BRIDGE:
		return Desc.tTargetBridge.iWidthCount > 0 &&
			Desc.tTargetBridge.fWidthSpacing > 0.f &&
			Desc.tTargetBridge.fWaveDelay >= 0.f ?
			S_OK : E_INVALIDARG;
	case PATTERN::PLAYER_WAVE_BRIDGE:
		return Desc.tPlayerWaveBridge.iWidthCount > 0 &&
			Desc.tPlayerWaveBridge.fWidthSpacing > 0.f &&
			Desc.tPlayerWaveBridge.fInfluenceRadius > 0.f &&
			Desc.tPlayerWaveBridge.fRaiseHeight >= 0.f &&
			Desc.tPlayerWaveBridge.fFollowDelay >= 0.f ?
			S_OK : E_INVALIDARG;
	default:
		return E_INVALIDARG;
	}
}

HRESULT CMagicSquareStepController::CreatePatternSteps(
	const DESC& Desc)
{
	switch (Desc.ePattern)
	{
	case PATTERN::COMBAT_CIRCLE:
		return CreateCombatCircle(Desc);
	case PATTERN::TARGET_BRIDGE:
		return CreateTargetBridge(Desc);
	case PATTERN::PLAYER_WAVE_BRIDGE:
		return CreatePlayerWaveBridge(Desc);
	default:
		return E_INVALIDARG;
	}
}

HRESULT CMagicSquareStepController::CreateCombatCircle(
	const DESC& Desc)
{
	const auto& Pattern = Desc.tCombatCircle;
	const int32_t iHalfCount = static_cast<int32_t>(
		std::ceil(Pattern.fRadius / Desc.tCommon.fSpacing));
	const _float fRadiusSq = Pattern.fRadius * Pattern.fRadius;

	for (int32_t z = -iHalfCount; z <= iHalfCount; ++z)
	{
		for (int32_t x = -iHalfCount; x <= iHalfCount; ++x)
		{
			const _float fX = x * Desc.tCommon.fSpacing;
			const _float fZ = z * Desc.tCommon.fSpacing;
			const _float fDistanceSq = fX * fX + fZ * fZ;
			if (fDistanceSq > fRadiusSq)
				continue;

			const _float fProgress =
				std::sqrt(fDistanceSq) / Pattern.fRadius;
			const _float3 vPosition{
				Pattern.vCenter.x + fX,
				Pattern.vCenter.y,
				Pattern.vCenter.z + fZ
			};
			if (FAILED(CreateStep(
				vPosition,
				vPosition,
				fProgress,
				fProgress * Desc.tCommon.fSummonWaveDelay)))
				return E_FAIL;
		}
	}
	return S_OK;
}

HRESULT CMagicSquareStepController::CreateTargetBridge(
	const DESC& Desc)
{
	const auto& Pattern = Desc.tTargetBridge;
	return CreateRectSteps(
		Pattern.vSourceA,
		Pattern.vSourceB,
		Pattern.vTargetA,
		Pattern.vTargetB,
		Pattern.iWidthCount,
		Pattern.fWidthSpacing,
		Pattern.fWaveDelay);
}

HRESULT CMagicSquareStepController::CreatePlayerWaveBridge(
	const DESC& Desc)
{
	const auto& Pattern = Desc.tPlayerWaveBridge;
	return CreateRectSteps(
		Pattern.vAnchorA,
		Pattern.vAnchorB,
		Pattern.vAnchorA,
		Pattern.vAnchorB,
		Pattern.iWidthCount,
		Pattern.fWidthSpacing,
		Desc.tCommon.fSummonWaveDelay);
}

HRESULT CMagicSquareStepController::CreateRectSteps(
	const _float3& vBaseA,
	const _float3& vBaseB,
	const _float3& vPatternA,
	const _float3& vPatternB,
	uint32_t iWidthCount,
	_float fWidthSpacing,
	_float fWaveDelay)
{
	const _float fDeltaX = vBaseB.x - vBaseA.x;
	const _float fDeltaZ = vBaseB.z - vBaseA.z;
	const _float fLength =
		std::sqrt(fDeltaX * fDeltaX + fDeltaZ * fDeltaZ);
	if (fLength <= FLT_EPSILON)
		return E_INVALIDARG;

	const uint32_t iLengthCount =
		static_cast<uint32_t>(
			std::ceil(fLength / m_tCommon.fSpacing)) + 1;
	const _float fPerpX = -fDeltaZ / fLength;
	const _float fPerpZ = fDeltaX / fLength;
	const _float fWidthCenter =
		(static_cast<_float>(iWidthCount) - 1.f) * 0.5f;

	const _float fPatternDeltaX = vPatternB.x - vPatternA.x;
	const _float fPatternDeltaZ = vPatternB.z - vPatternA.z;
	const _float fPatternLength =
		std::sqrt(
			fPatternDeltaX * fPatternDeltaX +
			fPatternDeltaZ * fPatternDeltaZ);
	const _float fPatternPerpX =
		fPatternLength > FLT_EPSILON ?
		-fPatternDeltaZ / fPatternLength : fPerpX;
	const _float fPatternPerpZ =
		fPatternLength > FLT_EPSILON ?
		fPatternDeltaX / fPatternLength : fPerpZ;

	for (uint32_t i = 0; i < iLengthCount; ++i)
	{
		const _float fT = static_cast<_float>(i) /
			static_cast<_float>(iLengthCount - 1);
		const _float3 vBaseCenter{
			std::lerp(vBaseA.x, vBaseB.x, fT),
			std::lerp(vBaseA.y, vBaseB.y, fT),
			std::lerp(vBaseA.z, vBaseB.z, fT)
		};
		const _float3 vPatternCenter{
			std::lerp(vPatternA.x, vPatternB.x, fT),
			std::lerp(vPatternA.y, vPatternB.y, fT),
			std::lerp(vPatternA.z, vPatternB.z, fT)
		};

		for (uint32_t w = 0; w < iWidthCount; ++w)
		{
			const _float fOffset =
				(static_cast<_float>(w) - fWidthCenter) *
				fWidthSpacing;
			const _float3 vBase{
				vBaseCenter.x + fPerpX * fOffset,
				vBaseCenter.y,
				vBaseCenter.z + fPerpZ * fOffset
			};
			const _float3 vPattern{
				vPatternCenter.x + fPatternPerpX * fOffset,
				vPatternCenter.y,
				vPatternCenter.z + fPatternPerpZ * fOffset
			};
			if (FAILED(CreateStep(
				vBase,
				vPattern,
				fT,
				fT * fWaveDelay)))
				return E_FAIL;
		}
	}
	return S_OK;
}

HRESULT CMagicSquareStepController::CreateStep(
	const _float3& vBasePosition,
	const _float3& vPatternPosition,
	_float fProgress,
	_float fSummonDelay)
{
	_float3 vHiddenPosition = vBasePosition;
	vHiddenPosition.y -= m_tCommon.fHiddenDepth;

	CMagicSquareStep::DESC Desc{};
	Desc.sObjectTag = "MagicSquareStep";
	Desc.vInitialPosition = vHiddenPosition;
	Desc.bEnablePhysics = m_tCommon.bEnablePhysics;

	const auto hStep = CGameInstance::Get().AddGameObjectToLayer(
		"LEVEL_CREATURE",
		"Prototype_GameObject_MagicSquareStep",
		"03_MagicSquareStep",
		&Desc);
	if (!hStep)
		return E_FAIL;

	m_Steps.push_back({
		.hStep = *hStep,
		.vHiddenPosition = vHiddenPosition,
		.vBasePosition = vBasePosition,
		.vPatternPosition = vPatternPosition,
		.fProgress = fProgress,
		.fSummonDelay = fSummonDelay
	});
	return S_OK;
}

void CMagicSquareStepController::UpdateSummoning()
{
	for (const STEP_DATA& Step : m_Steps)
	{
		if (m_fStateTime >= Step.fSummonDelay)
			SetStepTarget(
				Step,
				Step.vBasePosition,
				m_tCommon.fSummonSpeed);
	}

	if (m_fStateTime < m_fStateDuration)
		return;

	m_fStateTime = 0.f;
	if (m_ePattern == PATTERN::TARGET_BRIDGE)
	{
		m_eState = STATE::PATTERN_TRANSITION;
		_float fMaxDistance = 0.f;
		for (const STEP_DATA& Step : m_Steps)
		{
			const _vector vDelta =
				XMLoadFloat3(&Step.vPatternPosition) -
				XMLoadFloat3(&Step.vBasePosition);
			fMaxDistance = std::max(
				fMaxDistance,
				XMVectorGetX(XMVector3Length(vDelta)));
		}
		m_fStateDuration =
			m_tTargetBridge.fWaveDelay +
			fMaxDistance / m_tCommon.fPatternMoveSpeed;
	}
	else
	{
		m_eState = STATE::RUNNING;
		m_bFollowInitialized = false;
	}
}

void CMagicSquareStepController::UpdatePatternTransition()
{
	for (const STEP_DATA& Step : m_Steps)
	{
		if (m_fStateTime >=
			Step.fProgress * m_tTargetBridge.fWaveDelay)
			SetStepTarget(
				Step,
				Step.vPatternPosition,
				m_tCommon.fPatternMoveSpeed);
	}

	if (m_fStateTime >= m_fStateDuration)
	{
		m_eState = STATE::RUNNING;
		m_fStateTime = 0.f;
	}
}

void CMagicSquareStepController::UpdateDynamicPattern(
	_float fTimeDelta)
{
	if (m_ePattern == PATTERN::TARGET_BRIDGE)
		return;

	const CHandle hPlayer =
		m_ePattern == PATTERN::COMBAT_CIRCLE ?
		m_tCombatCircle.hPlayer :
		m_tPlayerWaveBridge.hPlayer;
	const auto* pPlayer =
		CGameInstance::Get().GetGameObjectByHandle(hPlayer);
	if (!pPlayer)
		return;

	const _float3& vPlayerPosition =
		pPlayer->GetTransform().GetPosition();
	const _float fFollowDelay =
		m_ePattern == PATTERN::COMBAT_CIRCLE ?
		m_tCombatCircle.fFollowDelay :
		m_tPlayerWaveBridge.fFollowDelay;

	if (!m_bFollowInitialized || fFollowDelay <= FLT_EPSILON)
	{
		m_vFollowPosition = vPlayerPosition;
		m_bFollowInitialized = true;
	}
	else
	{
		const _float fAlpha = 1.f - std::exp(
			-std::max(fTimeDelta, 0.f) / fFollowDelay);
		m_vFollowPosition.x +=
			(vPlayerPosition.x - m_vFollowPosition.x) * fAlpha;
		m_vFollowPosition.z +=
			(vPlayerPosition.z - m_vFollowPosition.z) * fAlpha;
	}

	const _float fInfluenceRadius =
		m_ePattern == PATTERN::COMBAT_CIRCLE ?
		m_tCombatCircle.fInfluenceRadius :
		m_tPlayerWaveBridge.fInfluenceRadius;
	const _float fRaiseHeight =
		m_ePattern == PATTERN::COMBAT_CIRCLE ?
		m_tCombatCircle.fRaiseHeight :
		m_tPlayerWaveBridge.fRaiseHeight;
	const _float fRadiusSq =
		fInfluenceRadius * fInfluenceRadius;

	for (const STEP_DATA& Step : m_Steps)
	{
		_float3 vTarget = Step.vBasePosition;
		const _float fDeltaX =
			m_vFollowPosition.x - Step.vBasePosition.x;
		const _float fDeltaZ =
			m_vFollowPosition.z - Step.vBasePosition.z;
		const _float fDistanceSq =
			fDeltaX * fDeltaX + fDeltaZ * fDeltaZ;
		if (fDistanceSq < fRadiusSq)
		{
			const _float fWeight = 1.f -
				std::sqrt(fDistanceSq) / fInfluenceRadius;
			const _float fSmooth =
				fWeight * fWeight * (3.f - 2.f * fWeight);
			vTarget.y += fRaiseHeight * fSmooth;
		}
		SetStepTarget(
			Step,
			vTarget,
			m_tCommon.fPatternMoveSpeed);
	}
}

void CMagicSquareStepController::UpdateDisappearing()
{
	for (const STEP_DATA& Step : m_Steps)
	{
		const _float fDelay =
			(1.f - Step.fProgress) *
			m_tCommon.fSummonWaveDelay;
		if (m_fStateTime >= fDelay)
			SetStepTarget(
				Step,
				Step.vHiddenPosition,
				m_tCommon.fSummonSpeed);
	}

	if (m_fStateTime >= m_fStateDuration)
	{
		m_eState = STATE::DORMANT;
		m_fStateTime = 0.f;
		m_bFollowInitialized = false;
	}
}

void CMagicSquareStepController::SetStepTarget(
	const STEP_DATA& Step,
	const _float3& vTarget,
	_float fSpeed)
{
	if (auto* pStep = CGameInstance::Get()
		.GetGameObjectByHandleT<CMagicSquareStep>(Step.hStep))
	{
		pStep->SetMoveTarget(vTarget, fSpeed);
	}
}

void CMagicSquareStepController::ClearSteps()
{
	for (const STEP_DATA& Step : m_Steps)
	{
		if (auto* pObject =
			CGameInstance::Get().GetGameObjectByHandle(Step.hStep))
			pObject->SetPendingDestroyCascade();
	}
	m_Steps.clear();
}

UPtr<CMagicSquareStepController>
CMagicSquareStepController::Create()
{
	auto pInstance =
		ToUPtr(new CMagicSquareStepController{});
	if (FAILED(pInstance->InitializePrototype()))
		return nullptr;
	return pInstance;
}

UPtr<CPrototype> CMagicSquareStepController::Clone(void* pArg)
{
	auto pInstance =
		ToUPtr(new CMagicSquareStepController{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
		return nullptr;
	return pInstance;
}
