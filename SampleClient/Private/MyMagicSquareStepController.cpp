#include "pch.h"
#include "MyMagicSquareStepController.h"

#include "GameInstance.h"
#include "MyMagicSquareStep.h"

NS_USING(Client)

CMyMagicSquareStepController::CMyMagicSquareStepController() = default;

CMyMagicSquareStepController::CMyMagicSquareStepController(
	const CMyMagicSquareStepController& rhs)
	: CGameObject{ rhs }
{
}

HRESULT CMyMagicSquareStepController::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc || pDesc->iMaxSpawnPerFrame == 0 ||
		FAILED(CGameObject::Initialize(pArg)))
		return E_INVALIDARG;

	m_iMaxSpawnPerFrame = pDesc->iMaxSpawnPerFrame;
	return S_OK;
}

void CMyMagicSquareStepController::PriorityUpdate(
	_float fTimeDelta)
{
	for (auto& [GroupID, Group] : m_mapGroup)
	{
		if (!Group.oRisePattern)
			continue;

		if (Group.eState == GROUP_STATE::READY)
		{
			Group.eState = GROUP_STATE::PATTERN_RUNNING;
			Group.fPatternElapsed = 0.f;
			Group.iIssuedLineCount = 0;
		}

		if (Group.eState == GROUP_STATE::PATTERN_RUNNING)
			UpdateRisePattern(Group, fTimeDelta);
	}
}

void CMyMagicSquareStepController::Update(_float)
{
	uint32_t iProcessedCount{};
	while (iProcessedCount < m_iMaxSpawnPerFrame &&
		!m_qSpawn.empty())
	{
		SPAWN_DATA Data = std::move(m_qSpawn.front());
		m_qSpawn.pop();
		++iProcessedCount;

		auto iter = m_mapGroup.find(Data.GroupID);
		if (iter == m_mapGroup.end() ||
			iter->second.eState != GROUP_STATE::SPAWNING)
			continue;

		if (!SpawnOne(Data))
			iter->second.eState = GROUP_STATE::FAILED;
	}
}

void CMyMagicSquareStepController::UpdateGUI()
{
	CGameObject::UpdateGUI();

	ImGui::Text(
		"Spawn Queue: %zu / Budget: %u",
		m_qSpawn.size(),
		m_iMaxSpawnPerFrame);
	ImGui::Text("Groups: %zu", m_mapGroup.size());

	ImGui::Separator();
	ImGui::Text("Rise Pattern Test");
	ImGui::DragFloat(
		"Rise Target Y",
		&m_fGUIRiseTargetY,
		0.1f);
	ImGui::DragFloat(
		"Move Speed",
		&m_fGUIMoveSpeed,
		0.1f,
		0.01f);
	ImGui::DragFloat(
		"Line Interval",
		&m_fGUILineInterval,
		0.01f,
		0.f);

	if (ImGui::RadioButton(
		"Axis X",
		m_eGUIFillAxis == FILL_AXIS::X))
	{
		m_eGUIFillAxis = FILL_AXIS::X;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton(
		"Axis Z",
		m_eGUIFillAxis == FILL_AXIS::Z))
	{
		m_eGUIFillAxis = FILL_AXIS::Z;
	}

	if (ImGui::RadioButton(
		"Forward",
		m_eGUIFillDirection ==
			FILL_DIRECTION::FORWARD))
	{
		m_eGUIFillDirection =
			FILL_DIRECTION::FORWARD;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton(
		"Reverse",
		m_eGUIFillDirection ==
			FILL_DIRECTION::REVERSE))
	{
		m_eGUIFillDirection =
			FILL_DIRECTION::REVERSE;
	}

	std::optional<StringID> oDeleteGroup{};
	for (const auto& [GroupID, Group] : m_mapGroup)
	{
		ImGui::PushID(GroupID.GetDbgStr());
		if (ImGui::TreeNode(GroupID.GetDbgStr()))
		{
			ImGui::Text(
				"State: %u",
				ETOUI(Group.eState));
			ImGui::Text(
				"Spawned: %zu / %zu",
				Group.iSpawnedCount,
				Group.iTargetCount);

			const _bool bCanStartPattern =
				Group.eState == GROUP_STATE::READY ||
				Group.eState ==
					GROUP_STATE::PATTERN_COMPLETE;
			if (bCanStartPattern)
			{
				RISE_PATTERN_DESC PatternDesc{};
				PatternDesc.fMoveSpeed =
					m_fGUIMoveSpeed;
				PatternDesc.fLineInterval =
					m_fGUILineInterval;
				PatternDesc.eAxis =
					m_eGUIFillAxis;
				PatternDesc.eDirection =
					m_eGUIFillDirection;

				if (ImGui::Button("Move Up"))
				{
					PatternDesc.fTargetY =
						m_fGUIRiseTargetY;
					StartRisePattern(
						GroupID,
						PatternDesc);
				}
				ImGui::SameLine();
				if (ImGui::Button("Move Down"))
				{
					PatternDesc.fTargetY =
						Group.tDesc
							.vStartPosition.y;
					StartRisePattern(
						GroupID,
						PatternDesc);
				}
			}
			else
			{
				ImGui::TextDisabled(
					"Pattern is running or spawning");
			}

			if (ImGui::Button("Delete Group"))
				oDeleteGroup = GroupID;

			ImGui::TreePop();
		}
		ImGui::PopID();
	}

	if (oDeleteGroup)
		DeleteGroup(*oDeleteGroup);
}

_bool CMyMagicSquareStepController::RegistGroup(
	StringID GroupID,
	const GROUP_DESC& Desc)
{
	if (GroupID.hash == 0 ||
		Desc.iCountX == 0 ||
		Desc.iCountZ == 0 ||
		Desc.fSpacingX <= 0.f ||
		Desc.fSpacingZ <= 0.f ||
		m_mapGroup.contains(GroupID))
		return false;

	const uint64_t iTargetCount =
		static_cast<uint64_t>(Desc.iCountX) * Desc.iCountZ;
	if (iTargetCount > std::numeric_limits<size_t>::max())
		return false;

	GROUP Group{};
	Group.tDesc = Desc;
	Group.iTargetCount =
		static_cast<size_t>(iTargetCount);
	Group.vecSteps.reserve(Group.iTargetCount);

	return m_mapGroup.emplace(
		std::move(GroupID),
		std::move(Group)).second;
}

_bool CMyMagicSquareStepController::SpawnGroup(
	StringID GroupID)
{
	auto iter = m_mapGroup.find(GroupID);
	if (iter == m_mapGroup.end() ||
		iter->second.eState != GROUP_STATE::REGISTERED)
		return false;

	GROUP& Group = iter->second;
	Group.eState = GROUP_STATE::SPAWNING;

	for (uint32_t z = 0; z < Group.tDesc.iCountZ; ++z)
	{
		for (uint32_t x = 0; x < Group.tDesc.iCountX; ++x)
		{
			m_qSpawn.push({
				.GroupID = GroupID,
				.vPosition = {
					Group.tDesc.vStartPosition.x +
						static_cast<_float>(x) *
						Group.tDesc.fSpacingX,
					Group.tDesc.vStartPosition.y,
					Group.tDesc.vStartPosition.z +
						static_cast<_float>(z) *
						Group.tDesc.fSpacingZ
				},
				.iIndexX = x,
				.iIndexZ = z
			});
		}
	}

	return true;
}

_bool CMyMagicSquareStepController::DeleteGroup(
	StringID GroupID)
{
	auto iter = m_mapGroup.find(GroupID);
	if (iter == m_mapGroup.end())
		return false;

	for (const STEP_DATA& StepData :
		iter->second.vecSteps)
	{
		if (auto* pStep = CGameInstance::Get()
			.GetGameObjectByHandle(
				StepData.hStep))
		{
			pStep->SetPendingDestroyCascade();
		}
	}

	std::queue<SPAWN_DATA> qRemaining{};
	while (!m_qSpawn.empty())
	{
		SPAWN_DATA Data =
			std::move(m_qSpawn.front());
		m_qSpawn.pop();

		if (Data.GroupID != GroupID)
			qRemaining.push(std::move(Data));
	}
	m_qSpawn.swap(qRemaining);

	m_mapGroup.erase(iter);
	return true;
}

_bool CMyMagicSquareStepController::StartRisePattern(
	StringID GroupID,
	const RISE_PATTERN_DESC& Desc)
{
	auto iter = m_mapGroup.find(GroupID);
	if (iter == m_mapGroup.end() ||
		Desc.fMoveSpeed <= 0.f ||
		Desc.fLineInterval < 0.f)
		return false;

	GROUP& Group = iter->second;
	if (Group.eState != GROUP_STATE::SPAWNING &&
		Group.eState != GROUP_STATE::READY &&
		Group.eState != GROUP_STATE::PATTERN_COMPLETE)
		return false;

	Group.oRisePattern = Desc;
	Group.fPatternElapsed = 0.f;
	Group.iIssuedLineCount = 0;

	if (Group.eState == GROUP_STATE::PATTERN_COMPLETE)
		Group.eState = GROUP_STATE::READY;

	return true;
}

std::optional<CMyMagicSquareStepController::GROUP_STATE>
CMyMagicSquareStepController::GetGroupState(
	StringID GroupID) const
{
	const auto iter = m_mapGroup.find(GroupID);
	if (iter == m_mapGroup.end())
		return std::nullopt;
	return iter->second.eState;
}

const std::vector<CMyMagicSquareStepController::STEP_DATA>*
CMyMagicSquareStepController::GetGroupSteps(
	StringID GroupID) const
{
	const auto iter = m_mapGroup.find(GroupID);
	if (iter == m_mapGroup.end())
		return nullptr;
	return &iter->second.vecSteps;
}

_bool CMyMagicSquareStepController::SpawnOne(
	const SPAWN_DATA& Data)
{
	auto iter = m_mapGroup.find(Data.GroupID);
	if (iter == m_mapGroup.end())
		return false;

	CMyMagicSquareStep::DESC Desc{};
	Desc.sObjectTag = "MyMagicSquareStep";
	Desc.vInitialPosition = Data.vPosition;

	const auto hStep =
		CGameInstance::Get().AddGameObjectToLayer(
			"LEVEL_CREATURE",
			"Prototype_GameObject_MyMagicSquareStep",
			"03_MyMagicSquareStep",
			&Desc);
	if (!hStep)
		return false;

	GROUP& Group = iter->second;
	Group.vecSteps.push_back({
		.hStep = *hStep,
		.iIndexX = Data.iIndexX,
		.iIndexZ = Data.iIndexZ,
		.vSpawnPosition = Data.vPosition
	});
	++Group.iSpawnedCount;
	if (Group.iSpawnedCount == Group.iTargetCount)
		Group.eState = GROUP_STATE::READY;

	return true;
}

void CMyMagicSquareStepController::UpdateRisePattern(
	GROUP& Group,
	_float fTimeDelta)
{
	const RISE_PATTERN_DESC& Desc =
		*Group.oRisePattern;
	const uint32_t iLineCount =
		Desc.eAxis == FILL_AXIS::X ?
		Group.tDesc.iCountX :
		Group.tDesc.iCountZ;

	Group.fPatternElapsed += fTimeDelta;

	uint32_t iTargetIssuedLineCount = iLineCount;
	if (Desc.fLineInterval > 0.f)
	{
		iTargetIssuedLineCount = std::min(
			iLineCount,
			static_cast<uint32_t>(
				Group.fPatternElapsed /
				Desc.fLineInterval) + 1);
	}

	while (Group.iIssuedLineCount <
		iTargetIssuedLineCount)
	{
		uint32_t iLineIndex =
			Group.iIssuedLineCount;
		if (Desc.eDirection ==
			FILL_DIRECTION::REVERSE)
		{
			iLineIndex =
				iLineCount - 1 - iLineIndex;
		}

		if (!IssueRiseLine(Group, iLineIndex))
		{
			Group.eState = GROUP_STATE::FAILED;
			return;
		}

		++Group.iIssuedLineCount;
	}

	if (Group.iIssuedLineCount != iLineCount)
		return;

	for (const STEP_DATA& StepData :
		Group.vecSteps)
	{
		auto* pStep = CGameInstance::Get()
			.GetGameObjectByHandleT<
				CMyMagicSquareStep>(
					StepData.hStep);
		if (!pStep)
		{
			Group.eState = GROUP_STATE::FAILED;
			return;
		}

		if (pStep->GetState() !=
			CMyMagicSquareStep::STATE::IDLE)
			return;
	}

	Group.eState = GROUP_STATE::PATTERN_COMPLETE;
	Group.oRisePattern.reset();
}

_bool CMyMagicSquareStepController::IssueRiseLine(
	GROUP& Group,
	uint32_t iLineIndex)
{
	const RISE_PATTERN_DESC& Desc =
		*Group.oRisePattern;

	for (const STEP_DATA& StepData :
		Group.vecSteps)
	{
		const uint32_t iStepLine =
			Desc.eAxis == FILL_AXIS::X ?
			StepData.iIndexX :
			StepData.iIndexZ;
		if (iStepLine != iLineIndex)
			continue;

		auto* pStep = CGameInstance::Get()
			.GetGameObjectByHandleT<
				CMyMagicSquareStep>(
					StepData.hStep);
		if (!pStep)
			return false;

		_float3 vTarget =
			StepData.vSpawnPosition;
		vTarget.y = Desc.fTargetY;
		pStep->SetSpeed(Desc.fMoveSpeed);
		pStep->SetMoveTarget(
			XMLoadFloat3(&vTarget));
	}

	return true;
}

UPtr<CMyMagicSquareStepController>
CMyMagicSquareStepController::Create()
{
	auto pInstance =
		ToUPtr(new CMyMagicSquareStepController{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed Create CMyMagicSquareStepController");
		return nullptr;
	}
	return pInstance;
}

UPtr<CPrototype>
CMyMagicSquareStepController::Clone(void* pArg)
{
	auto pInstance =
		ToUPtr(new CMyMagicSquareStepController{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed Clone CMyMagicSquareStepController");
		return nullptr;
	}
	return pInstance;
}
