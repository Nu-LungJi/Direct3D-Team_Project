#include "pch.h"
#include "MagicBridgeController.h"

#include "GameInstance.h"
#include "TestSquareStep.h"

NS_USING(Client)

CMagicBridgeController::CMagicBridgeController() = default;

CMagicBridgeController::CMagicBridgeController(
	const CMagicBridgeController& prototype)
	: CGameObject{ prototype }
{
}

HRESULT CMagicBridgeController::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc || pDesc->fBlockSpacing <= 0.f ||
		pDesc->iWidthCount == 0 || pDesc->fWidthSpacing <= 0.f ||
		pDesc->fMoveSpeed <= 0.f || pDesc->fWaveDelay < 0.f ||
		FAILED(CGameObject::Initialize(pArg)))
		return E_INVALIDARG;

	m_fMoveSpeed = pDesc->fMoveSpeed;
	if (FAILED(CreateBridge(*pDesc)))
	{
		ClearBridgeBlocks();
		return E_FAIL;
	}

	if (pDesc->bStartActivated)
	{
		SetAllTargets(true);
		m_eState = STATE::ACTIVE;
	}

	return S_OK;
}

void CMagicBridgeController::PriorityUpdate(_float fTimeDelta)
{
	if (m_eState != STATE::ACTIVATING &&
		m_eState != STATE::DEACTIVATING)
		return;

	m_fStateTime += std::max(fTimeDelta, 0.f);
	const _bool bActivating = m_eState == STATE::ACTIVATING;

	for (const BRIDGE_BLOCK& Block : m_Blocks)
	{
		if (m_fStateTime < Block.fDelay)
			continue;

		auto* pBlock = CGameInstance::Get()
			.GetGameObjectByHandleT<CTestSquareStep>(Block.hBlock);
		if (!pBlock)
			continue;

		pBlock->SetHeightTarget(
			bActivating ? Block.fActivatedY : Block.fInitialY,
			m_fMoveSpeed);
	}

	if (m_fStateTime >= m_fTransitionDuration)
	{
		SetAllTargets(bActivating);
		m_eState = bActivating ? STATE::ACTIVE : STATE::IDLE;
		m_fStateTime = 0.f;
	}
}

void CMagicBridgeController::UpdateGUI()
{
	CGameObject::UpdateGUI();

	const char* pStateName = "Idle";
	switch (m_eState)
	{
	case STATE::ACTIVATING:
		pStateName = "Activating";
		break;
	case STATE::ACTIVE:
		pStateName = "Active";
		break;
	case STATE::DEACTIVATING:
		pStateName = "Deactivating";
		break;
	default:
		break;
	}

	ImGui::Text("State: %s", pStateName);
	ImGui::Text("Blocks: %zu", m_Blocks.size());
	if (ImGui::Button("Activate Bridge"))
		Activate();
	ImGui::SameLine();
	if (ImGui::Button("Deactivate Bridge"))
		Deactivate();
}

void CMagicBridgeController::Activate()
{
	if (m_eState == STATE::ACTIVE ||
		m_eState == STATE::ACTIVATING)
		return;
	BeginTransition(STATE::ACTIVATING);
}

void CMagicBridgeController::Deactivate()
{
	if (m_eState == STATE::IDLE ||
		m_eState == STATE::DEACTIVATING)
		return;
	BeginTransition(STATE::DEACTIVATING);
}

HRESULT CMagicBridgeController::CreateBridge(const DESC& Desc)
{
	const _float fDeltaX = Desc.vAnchorB.x - Desc.vAnchorA.x;
	const _float fDeltaZ = Desc.vAnchorB.z - Desc.vAnchorA.z;
	const _float fHorizontalLength =
		std::sqrt(fDeltaX * fDeltaX + fDeltaZ * fDeltaZ);
	if (fHorizontalLength <= FLT_EPSILON)
		return E_INVALIDARG;

	const uint64_t iBlockCount64 = static_cast<uint64_t>(
		std::ceil(fHorizontalLength / Desc.fBlockSpacing)) + 1;
	if (iBlockCount64 < 2 ||
		iBlockCount64 > std::numeric_limits<uint32_t>::max())
		return E_INVALIDARG;

	const uint32_t iBlockCount =
		static_cast<uint32_t>(iBlockCount64);
	if (iBlockCount64 >
		std::numeric_limits<size_t>::max() / Desc.iWidthCount)
		return E_INVALIDARG;

	const size_t iTotalBlockCount =
		static_cast<size_t>(iBlockCount64) * Desc.iWidthCount;
	m_Blocks.clear();
	m_Blocks.reserve(iTotalBlockCount);

	const _float fPerpendicularX = -fDeltaZ / fHorizontalLength;
	const _float fPerpendicularZ = fDeltaX / fHorizontalLength;
	const _float fWidthCenter =
		(static_cast<_float>(Desc.iWidthCount) - 1.f) * 0.5f;

	const _float fMaxHeightDistance =
		std::abs(Desc.fActivatedAnchorAY - Desc.vAnchorA.y);
	m_fTransitionDuration =
		std::max(
			Desc.fWaveDelay,
			fMaxHeightDistance / m_fMoveSpeed);

	for (uint32_t i = 0; i < iBlockCount; ++i)
	{
		const _float fT = static_cast<_float>(i) /
			static_cast<_float>(iBlockCount - 1);
		const _float3 vInitialPosition{
			std::lerp(Desc.vAnchorA.x, Desc.vAnchorB.x, fT),
			std::lerp(Desc.vAnchorA.y, Desc.vAnchorB.y, fT),
			std::lerp(Desc.vAnchorA.z, Desc.vAnchorB.z, fT)
		};
		const _float fActivatedY =
			std::lerp(Desc.fActivatedAnchorAY, Desc.vAnchorB.y, fT);
		const _float fDelay = fT * Desc.fWaveDelay;

		for (uint32_t iWidth = 0;
			iWidth < Desc.iWidthCount;
			++iWidth)
		{
			const _float fWidthOffset =
				(static_cast<_float>(iWidth) - fWidthCenter) *
				Desc.fWidthSpacing;
			const _float3 vBlockPosition{
				vInitialPosition.x +
					fPerpendicularX * fWidthOffset,
				vInitialPosition.y,
				vInitialPosition.z +
					fPerpendicularZ * fWidthOffset
			};

			if (FAILED(CreateBridgeBlock(
				vBlockPosition,
				fActivatedY,
				fDelay,
				Desc.bEnablePhysics)))
				return E_FAIL;
		}
	}

	return S_OK;
}

HRESULT CMagicBridgeController::CreateBridgeBlock(
	const _float3& vPosition,
	_float fActivatedY,
	_float fDelay,
	_bool bEnablePhysics)
{
	CTestSquareStep::DESC Desc{};
	Desc.sObjectTag = "MagicBridgeBlock";
	Desc.vInitialPosition = vPosition;
	Desc.bEnablePhysics = bEnablePhysics;

	const auto hBlock = CGameInstance::Get().AddGameObjectToLayer(
		"LEVEL_CREATURE",
		"Prototype_GameObject_TestSquareStep",
		"03_MagicBridge",
		&Desc);
	if (!hBlock)
		return E_FAIL;

	m_Blocks.push_back({
		.hBlock = *hBlock,
		.fInitialY = vPosition.y,
		.fActivatedY = fActivatedY,
		.fDelay = fDelay
	});
	return S_OK;
}

void CMagicBridgeController::BeginTransition(STATE eState)
{
	HoldCurrentHeights();
	m_eState = eState;
	m_fStateTime = 0.f;
}

void CMagicBridgeController::SetAllTargets(_bool bActivated)
{
	for (const BRIDGE_BLOCK& Block : m_Blocks)
	{
		auto* pBlock = CGameInstance::Get()
			.GetGameObjectByHandleT<CTestSquareStep>(Block.hBlock);
		if (!pBlock)
			continue;
		pBlock->SetHeightTarget(
			bActivated ? Block.fActivatedY : Block.fInitialY,
			m_fMoveSpeed);
	}
}

void CMagicBridgeController::HoldCurrentHeights()
{
	for (const BRIDGE_BLOCK& Block : m_Blocks)
	{
		auto* pBlock = CGameInstance::Get()
			.GetGameObjectByHandleT<CTestSquareStep>(Block.hBlock);
		if (!pBlock)
			continue;
		pBlock->SetHeightTarget(
			pBlock->GetTransform().GetPosition().y,
			m_fMoveSpeed);
	}
}

void CMagicBridgeController::ClearBridgeBlocks()
{
	for (const BRIDGE_BLOCK& Block : m_Blocks)
	{
		if (auto* pObject =
			CGameInstance::Get().GetGameObjectByHandle(Block.hBlock))
			pObject->SetPendingDestroyCascade();
	}
	m_Blocks.clear();
}

UPtr<CMagicBridgeController> CMagicBridgeController::Create()
{
	auto pInstance = ToUPtr(new CMagicBridgeController{});
	if (FAILED(pInstance->InitializePrototype()))
		return nullptr;
	return pInstance;
}

UPtr<CPrototype> CMagicBridgeController::Clone(void* pArg)
{
	auto pInstance =
		ToUPtr(new CMagicBridgeController{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
		return nullptr;
	return pInstance;
}
