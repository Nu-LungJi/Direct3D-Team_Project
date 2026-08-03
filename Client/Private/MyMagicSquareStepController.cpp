#include "pch.h"
#include "MyMagicSquareStepController.h"

#include "ComSound.h"
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
	m_StepProtoMajorTag = pDesc->ProtoMajorTag;
	m_StepProtoMinorTag = pDesc->ProtoMinorTag;
	m_StepSpawnLayerName = pDesc->SpawnLayerName;
	m_ResMajorTag = pDesc->ResMajorTag;
	m_ResMinorTag = pDesc->ResMinorTag;

	m_iMaxSpawnPerFrame = pDesc->iMaxSpawnPerFrame;

	CComSound::DESC SoundDesc{};
	if (FAILED(AddComponentFromProto(
		ES_EngineProtoMajorType::PERMANENT,
		ES_EngineProtoComponent::Prototype_Component_ComSound,
		"Com_Sound",
		&SoundDesc,
		&m_pComSound)))
	{
		return E_FAIL;
	}

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
			Group.iIssuedStepCount = 0;
			Group.setIssuedLineEvents.clear();
			QueuePatternEvent(
				GroupID,
				Group,
				PATTERN_TYPE::RISE,
				PATTERN_EVENT::STARTED,
				CalculateGroupCenter(Group));
		}

		if (Group.eState == GROUP_STATE::PATTERN_RUNNING)
			UpdateRisePattern(GroupID, Group, fTimeDelta);
	}

	DispatchPendingPatternEvents();
}

void CMyMagicSquareStepController::FixedUpdate(
	_float fTimeDelta)
{
	for (auto& [GroupID, Group] : m_mapGroup)
	{
		if (Group.eState ==
			GROUP_STATE::PATTERN_RUNNING &&
			Group.oWavePattern)
		{
			UpdateWavePattern(
				GroupID,
				Group,
				fTimeDelta);
		}
	}

	DispatchPendingPatternEvents();
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
			SetGroupFailed(Data.GroupID, iter->second);
	}

	if (m_pComSound)
		m_pComSound->Update();

	DispatchPendingPatternEvents();
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
		"Start Target Y",
		&m_fGUIStartTargetY,
		0.1f);
	ImGui::DragFloat(
		"End Target Y",
		&m_fGUIEndTargetY,
		0.1f);
	ImGui::DragFloat(
		"Move Speed",
		&m_fGUIMoveSpeed,
		0.1f,
		0.01f);
	ImGui::DragFloat(
		"Bounce Height",
		&m_fGUIBounceHeight,
		0.05f,
		0.f);
	ImGui::DragFloat(
		"Bounce Settle Speed",
		&m_fGUIBounceSettleSpeed,
		0.1f,
		0.01f);
	ImGui::DragFloat(
		"Line Interval",
		&m_fGUILineInterval,
		0.01f,
		0.f);
	ImGui::DragFloat(
		"Step Interval",
		&m_fGUIStepInterval,
		0.005f,
		0.f);
	ImGui::DragFloat(
		"Step Timing Curve",
		&m_fGUIStepTimingCurve,
		0.05f,
		0.1f,
		2.f);
	ImGui::DragFloat(
		"Step Timing Jitter",
		&m_fGUIStepTimingJitter,
		0.001f,
		0.f);

	if (ImGui::RadioButton(
		"Fill Axis X",
		m_eGUIRiseFillMode ==
		RISE_FILL_MODE::X))
	{
		m_eGUIRiseFillMode =
			RISE_FILL_MODE::X;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton(
		"Fill Axis Z",
		m_eGUIRiseFillMode ==
		RISE_FILL_MODE::Z))
	{
		m_eGUIRiseFillMode =
			RISE_FILL_MODE::Z;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton(
		"Fill Radial",
		m_eGUIRiseFillMode ==
		RISE_FILL_MODE::RADIAL))
	{
		m_eGUIRiseFillMode =
			RISE_FILL_MODE::RADIAL;
	}

	if (ImGui::RadioButton(
		"Height Axis X",
		m_eGUIHeightAxis == FILL_AXIS::X))
	{
		m_eGUIHeightAxis = FILL_AXIS::X;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton(
		"Height Axis Z",
		m_eGUIHeightAxis == FILL_AXIS::Z))
	{
		m_eGUIHeightAxis = FILL_AXIS::Z;
	}

	if (ImGui::RadioButton(
		"Forward / Center Out",
		m_eGUIFillDirection ==
		FILL_DIRECTION::FORWARD))
	{
		m_eGUIFillDirection =
			FILL_DIRECTION::FORWARD;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton(
		"Reverse / Outside In",
		m_eGUIFillDirection ==
		FILL_DIRECTION::REVERSE))
	{
		m_eGUIFillDirection =
			FILL_DIRECTION::REVERSE;
	}

	ImGui::Separator();
	ImGui::Text("Wave Pattern Test");
	ImGui::DragFloat(
		"Wave Amplitude",
		&m_fGUIWaveAmplitude,
		0.1f);
	ImGui::DragFloat(
		"Wave Duration",
		&m_fGUIWaveDuration,
		0.05f,
		0.01f);
	ImGui::DragFloat(
		"Wave Line Interval",
		&m_fGUIWaveLineInterval,
		0.01f,
		0.f);

	if (ImGui::RadioButton(
		"Wave Axis X",
		m_eGUIWaveAxis == FILL_AXIS::X))
	{
		m_eGUIWaveAxis = FILL_AXIS::X;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton(
		"Wave Axis Z",
		m_eGUIWaveAxis == FILL_AXIS::Z))
	{
		m_eGUIWaveAxis = FILL_AXIS::Z;
	}

	if (ImGui::RadioButton(
		"Wave Forward",
		m_eGUIWaveDirection ==
		FILL_DIRECTION::FORWARD))
	{
		m_eGUIWaveDirection =
			FILL_DIRECTION::FORWARD;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton(
		"Wave Reverse",
		m_eGUIWaveDirection ==
		FILL_DIRECTION::REVERSE))
	{
		m_eGUIWaveDirection =
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
				PatternDesc.fStepInterval =
					m_fGUIStepInterval;
				PatternDesc.fStepTimingCurve =
					m_fGUIStepTimingCurve;
				PatternDesc.fStepTimingJitter =
					m_fGUIStepTimingJitter;
				PatternDesc.eFillMode =
					m_eGUIRiseFillMode;
				PatternDesc.eHeightAxis =
					m_eGUIHeightAxis;
				PatternDesc.eDirection =
					m_eGUIFillDirection;

				if (ImGui::Button("Move Up"))
				{
					PatternDesc.fStartTargetY =
						m_fGUIStartTargetY;
					PatternDesc.fEndTargetY =
						m_fGUIEndTargetY;
					PatternDesc.fBounceHeight =
						m_fGUIBounceHeight;
					PatternDesc.fBounceSettleSpeed =
						m_fGUIBounceSettleSpeed;
					StartRisePattern(
						GroupID,
						PatternDesc);
				}
				ImGui::SameLine();
				if (ImGui::Button("Move Down"))
				{
					PatternDesc.fStartTargetY =
						Group.fSpawnBaseY;
					PatternDesc.fEndTargetY =
						Group.fSpawnBaseY;
					StartRisePattern(
						GroupID,
						PatternDesc);
				}

				WAVE_PATTERN_DESC WaveDesc{};
				WaveDesc.fAmplitude =
					m_fGUIWaveAmplitude;
				WaveDesc.fWaveDuration =
					m_fGUIWaveDuration;
				WaveDesc.fLineInterval =
					m_fGUIWaveLineInterval;
				WaveDesc.eAxis =
					m_eGUIWaveAxis;
				WaveDesc.eDirection =
					m_eGUIWaveDirection;
				if (ImGui::Button("Start Wave"))
				{
					StartWavePattern(
						GroupID,
						WaveDesc);
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

_bool CMyMagicSquareStepController::RegistRectGroup(
	StringID GroupID,
	const RECT_GROUP_DESC& Desc)
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
	Group.eLayoutType = LAYOUT_TYPE::RECT;
	Group.fSpawnBaseY = Desc.vStartPosition.y;
	Group.iGridCountX = Desc.iCountX;
	Group.iGridCountZ = Desc.iCountZ;
	const _float fCenterIndexX =
		static_cast<_float>(Desc.iCountX - 1) *
		0.5f;
	const _float fCenterIndexZ =
		static_cast<_float>(Desc.iCountZ - 1) *
		0.5f;
	Group.fMaxRadialDistance =
		sqrtf(
			fCenterIndexX * fCenterIndexX +
			fCenterIndexZ * fCenterIndexZ);
	Group.iTargetCount =
		static_cast<size_t>(iTargetCount);
	Group.vecLayoutPoints.reserve(
		Group.iTargetCount);
	Group.vecSteps.reserve(Group.iTargetCount);

	for (uint32_t z = 0; z < Desc.iCountZ; ++z)
	{
		for (uint32_t x = 0; x < Desc.iCountX; ++x)
		{
			Group.vecLayoutPoints.push_back({
				.vPosition = {
					Desc.vStartPosition.x +
						static_cast<_float>(x) *
						Desc.fSpacingX,
					Desc.vStartPosition.y,
					Desc.vStartPosition.z +
						static_cast<_float>(z) *
						Desc.fSpacingZ
				},
				.iIndexX = x,
				.iIndexZ = z,
				.fNormalizedX =
					Desc.iCountX > 1 ?
					static_cast<_float>(x) /
						static_cast<_float>(
							Desc.iCountX - 1) :
					0.f,
				.fNormalizedZ =
					Desc.iCountZ > 1 ?
					static_cast<_float>(z) /
						static_cast<_float>(
							Desc.iCountZ - 1) :
					0.f,
				.fRadialDistance =
					sqrtf(
						(static_cast<_float>(x) -
							fCenterIndexX) *
						(static_cast<_float>(x) -
							fCenterIndexX) +
						(static_cast<_float>(z) -
							fCenterIndexZ) *
						(static_cast<_float>(z) -
							fCenterIndexZ)),
				.fNormalizedRadius =
					Group.fMaxRadialDistance >
						FLT_EPSILON ?
					sqrtf(
						(static_cast<_float>(x) -
							fCenterIndexX) *
						(static_cast<_float>(x) -
							fCenterIndexX) +
						(static_cast<_float>(z) -
							fCenterIndexZ) *
						(static_cast<_float>(z) -
							fCenterIndexZ)) /
						Group.fMaxRadialDistance :
					0.f
				});
		}
	}

	return m_mapGroup.emplace(
		std::move(GroupID),
		std::move(Group)).second;
}

_bool CMyMagicSquareStepController::
RegistFilledCircleGroup(
	StringID GroupID,
	const FILLED_CIRCLE_GROUP_DESC& Desc)
{
	if (GroupID.hash == 0 ||
		Desc.fRadius <= 0.f ||
		Desc.fSpacing <= 0.f ||
		m_mapGroup.contains(GroupID))
		return false;

	const double fHalfCount =
		std::floor(
			static_cast<double>(Desc.fRadius) /
			static_cast<double>(Desc.fSpacing));
	const double fMaxHalfCount =
		static_cast<double>(
			(std::numeric_limits<uint32_t>::max() -
				1u) /
			2u);
	if (fHalfCount > fMaxHalfCount)
		return false;

	const uint32_t iHalfCount =
		static_cast<uint32_t>(fHalfCount);
	const uint32_t iGridCount =
		iHalfCount * 2u + 1u;
	const uint64_t iGridCapacity =
		static_cast<uint64_t>(iGridCount) *
		iGridCount;
	if (iGridCapacity >
		std::numeric_limits<size_t>::max())
		return false;

	GROUP Group{};
	Group.eLayoutType =
		LAYOUT_TYPE::FILLED_CIRCLE;
	Group.fSpawnBaseY = Desc.vCenter.y;
	Group.iGridCountX = iGridCount;
	Group.iGridCountZ = iGridCount;
	Group.fMaxRadialDistance =
		static_cast<_float>(iHalfCount);
	Group.vecLayoutPoints.reserve(
		static_cast<size_t>(iGridCapacity));

	const _float fRadiusSq =
		Desc.fRadius * Desc.fRadius;
	for (uint32_t z = 0; z < iGridCount; ++z)
	{
		for (uint32_t x = 0; x < iGridCount; ++x)
		{
			const int64_t iOffsetX =
				static_cast<int64_t>(x) -
				iHalfCount;
			const int64_t iOffsetZ =
				static_cast<int64_t>(z) -
				iHalfCount;
			const _float fLocalX =
				static_cast<_float>(iOffsetX) *
				Desc.fSpacing;
			const _float fLocalZ =
				static_cast<_float>(iOffsetZ) *
				Desc.fSpacing;
			const _float fLocalRadiusSq =
				fLocalX * fLocalX +
				fLocalZ * fLocalZ;
			if (fLocalRadiusSq > fRadiusSq)
				continue;
			const _float fLocalRadius =
				sqrtf(fLocalRadiusSq);

			Group.vecLayoutPoints.push_back({
				.vPosition = {
					Desc.vCenter.x + fLocalX,
					Desc.vCenter.y,
					Desc.vCenter.z + fLocalZ
				},
				.iIndexX = x,
				.iIndexZ = z,
				.fNormalizedX =
					iGridCount > 1 ?
					static_cast<_float>(x) /
						static_cast<_float>(
							iGridCount - 1) :
					0.f,
				.fNormalizedZ =
					iGridCount > 1 ?
					static_cast<_float>(z) /
						static_cast<_float>(
							iGridCount - 1) :
					0.f,
				.fRadialDistance =
					fLocalRadius /
					Desc.fSpacing,
				.fNormalizedRadius =
					fLocalRadius /
					Desc.fRadius
				});
		}
	}

	Group.iTargetCount =
		Group.vecLayoutPoints.size();
	Group.vecSteps.reserve(
		Group.iTargetCount);

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

	for (const LAYOUT_POINT& Point :
		Group.vecLayoutPoints)
	{
		m_qSpawn.push({
			.GroupID = GroupID,
			.vPosition = Point.vPosition,
			.iIndexX = Point.iIndexX,
			.iIndexZ = Point.iIndexZ,
			.fNormalizedX = Point.fNormalizedX,
			.fNormalizedZ = Point.fNormalizedZ,
			.fRadialDistance =
				Point.fRadialDistance,
			.fNormalizedRadius =
				Point.fNormalizedRadius
			});
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
		Desc.fBounceHeight < 0.f ||
		Desc.fBounceSettleSpeed <= 0.f ||
		Desc.fLineInterval < 0.f ||
		Desc.fStepInterval < 0.f ||
		Desc.fStepTimingCurve <= 0.f ||
		Desc.fStepTimingJitter < 0.f)
		return false;

	GROUP& Group = iter->second;
	if (Group.eState != GROUP_STATE::SPAWNING &&
		Group.eState != GROUP_STATE::READY &&
		Group.eState != GROUP_STATE::PATTERN_COMPLETE)
		return false;

	Group.oRisePattern = Desc;
	Group.oWavePattern.reset();
	Group.fPatternElapsed = 0.f;
	Group.iIssuedStepCount = 0;
	Group.setIssuedLineEvents.clear();
	for (STEP_DATA& StepData : Group.vecSteps)
		StepData.bRiseIssued = false;

	if (Group.eState == GROUP_STATE::PATTERN_COMPLETE)
		Group.eState = GROUP_STATE::READY;

	return true;
}

_bool CMyMagicSquareStepController::StartWavePattern(
	StringID GroupID,
	const WAVE_PATTERN_DESC& Desc)
{
	auto iter = m_mapGroup.find(GroupID);
	if (iter == m_mapGroup.end() ||
		Desc.fWaveDuration <= 0.f ||
		Desc.fLineInterval < 0.f)
		return false;

	GROUP& Group = iter->second;
	if (Group.eState != GROUP_STATE::READY &&
		Group.eState !=
		GROUP_STATE::PATTERN_COMPLETE)
		return false;

	for (STEP_DATA& StepData : Group.vecSteps)
	{
		auto* pStep = CGameInstance::Get()
			.GetGameObjectByHandleT<
			CMyMagicSquareStep>(
				StepData.hStep);
		if (!pStep)
		{
			Group.eState = GROUP_STATE::FAILED;
			return false;
		}

		StepData.vPatternBasePosition =
			pStep->GetTransform().GetPosition();
	}

	Group.oRisePattern.reset();
	Group.oWavePattern = Desc;
	Group.fPatternElapsed = 0.f;
	Group.setIssuedLineEvents.clear();
	Group.eState = GROUP_STATE::PATTERN_RUNNING;
	QueuePatternEvent(
		GroupID,
		Group,
		PATTERN_TYPE::WAVE,
		PATTERN_EVENT::STARTED,
		CalculateGroupCenter(Group));
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
	Desc.ResMajorTag = m_ResMajorTag;
	Desc.ResMinorTag = m_ResMinorTag;

	const auto hStep =
		CGameInstance::Get().AddGameObjectToLayer(
			m_StepProtoMajorTag,
			m_StepProtoMinorTag,
			m_StepSpawnLayerName,
			&Desc);
	if (!hStep)
		return false;

	GROUP& Group = iter->second;
	Group.vecSteps.push_back({
		.hStep = *hStep,
		.iIndexX = Data.iIndexX,
		.iIndexZ = Data.iIndexZ,
		.vSpawnPosition = Data.vPosition,
		.vPatternBasePosition = Data.vPosition,
		.fNormalizedX = Data.fNormalizedX,
		.fNormalizedZ = Data.fNormalizedZ,
		.fRadialDistance =
			Data.fRadialDistance,
		.fNormalizedRadius =
			Data.fNormalizedRadius,
		.bRiseIssued = false
		});
	++Group.iSpawnedCount;
	if (Group.iSpawnedCount == Group.iTargetCount)
		Group.eState = GROUP_STATE::READY;

	return true;
}

void CMyMagicSquareStepController::UpdateRisePattern(
	StringID GroupID,
	GROUP& Group,
	_float fTimeDelta)
{
	const RISE_PATTERN_DESC& Desc =
		*Group.oRisePattern;

	Group.fPatternElapsed += fTimeDelta;

	for (STEP_DATA& StepData : Group.vecSteps)
	{
		if (StepData.bRiseIssued)
			continue;

		const uint32_t iTimingHash =
			StepData.iIndexX * 73856093u ^
			StepData.iIndexZ * 19349663u;
		const _float fNormalizedJitter =
			static_cast<_float>(
				iTimingHash & 0xffffu) /
			65535.f;
		const _float fTimingJitter =
			(fNormalizedJitter * 2.f - 1.f) *
			Desc.fStepTimingJitter;

		_float fStartDelay{};
		if (Desc.eFillMode ==
			RISE_FILL_MODE::RADIAL)
		{
			_float fRadialOrder =
				StepData.fRadialDistance;
			if (Desc.eDirection ==
				FILL_DIRECTION::REVERSE)
			{
				fRadialOrder =
					Group.fMaxRadialDistance -
					fRadialOrder;
			}

			const _float fRadialRandomDelay =
				fNormalizedJitter *
				Desc.fStepTimingJitter *
				std::min(1.f, fRadialOrder);
			fStartDelay = std::max(
				0.f,
				powf(
					std::max(
						0.f,
						fRadialOrder),
					Desc.fStepTimingCurve) *
				Desc.fLineInterval +
				fRadialRandomDelay);
		}
		else
		{
			const _bool bFillX =
				Desc.eFillMode ==
				RISE_FILL_MODE::X;
			const uint32_t iPrimaryCount =
				bFillX ?
				Group.iGridCountX :
				Group.iGridCountZ;
			uint32_t iPrimaryIndex =
				bFillX ?
				StepData.iIndexX :
				StepData.iIndexZ;
			if (Desc.eDirection ==
				FILL_DIRECTION::REVERSE)
			{
				iPrimaryIndex =
					iPrimaryCount - 1 -
					iPrimaryIndex;
			}

			const uint32_t iSecondaryIndex =
				bFillX ?
				StepData.iIndexZ :
				StepData.iIndexX;
			const _float fCurvedStepDelay =
				powf(
					static_cast<_float>(
						iSecondaryIndex),
					Desc.fStepTimingCurve) *
				Desc.fStepInterval;
			const _float fSecondaryDelay =
				std::max(
					0.f,
					fCurvedStepDelay +
					fTimingJitter);
			fStartDelay =
				static_cast<_float>(
					iPrimaryIndex) *
				Desc.fLineInterval +
				fSecondaryDelay;
		}

		if (Group.fPatternElapsed <
			fStartDelay)
			continue;

		if (!IssueRiseStep(GroupID, Group, StepData))
		{
			SetGroupFailed(GroupID, Group);
			return;
		}

		StepData.bRiseIssued = true;
		++Group.iIssuedStepCount;
	}

	if (Group.iIssuedStepCount !=
		Group.vecSteps.size())
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
			SetGroupFailed(GroupID, Group);
			return;
		}

		if (pStep->GetState() !=
			CMyMagicSquareStep::STATE::IDLE)
			return;
	}

	QueuePatternEvent(
		GroupID,
		Group,
		PATTERN_TYPE::RISE,
		PATTERN_EVENT::COMPLETED,
		CalculateGroupCenter(Group));
	Group.eState = GROUP_STATE::PATTERN_COMPLETE;
	Group.oRisePattern.reset();
}

_bool CMyMagicSquareStepController::IssueRiseStep(
	StringID GroupID,
	GROUP& Group,
	STEP_DATA& StepData)
{
	const RISE_PATTERN_DESC& Desc =
		*Group.oRisePattern;

	auto* pStep = CGameInstance::Get()
		.GetGameObjectByHandleT<
		CMyMagicSquareStep>(
			StepData.hStep);
	if (!pStep)
		return false;

	_float3 vTarget =
		StepData.vSpawnPosition;
	const _float fHeightRatio =
		Desc.eHeightAxis == FILL_AXIS::X ?
		StepData.fNormalizedX :
		StepData.fNormalizedZ;
	vTarget.y =
		Desc.fStartTargetY +
		(Desc.fEndTargetY -
			Desc.fStartTargetY) *
		fHeightRatio;
	if (Desc.fBounceHeight > 0.f)
	{
		pStep->SetBounceMoveTarget(
			XMLoadFloat3(&vTarget),
			Desc.fMoveSpeed,
			Desc.fBounceHeight,
			Desc.fBounceSettleSpeed);
	}
	else
	{
		pStep->SetSpeed(Desc.fMoveSpeed);
		pStep->SetMoveTarget(
			XMLoadFloat3(&vTarget));
	}

	const uint32_t iLineIndex =
		CalculateRiseLineIndex(
			Group,
			StepData,
			Desc);
	if (Group.setIssuedLineEvents.insert(
		iLineIndex).second)
	{
		QueuePatternEvent(
			GroupID,
			Group,
			PATTERN_TYPE::RISE,
			PATTERN_EVENT::LINE_ISSUED,
			StepData.vSpawnPosition,
			iLineIndex,
			StepData.hStep);
	}

	return true;
}

void CMyMagicSquareStepController::UpdateWavePattern(
	StringID GroupID,
	GROUP& Group,
	_float fTimeDelta)
{
	const WAVE_PATTERN_DESC& Desc =
		*Group.oWavePattern;
	const uint32_t iLineCount =
		Desc.eAxis == FILL_AXIS::X ?
		Group.iGridCountX :
		Group.iGridCountZ;
	const _float fLastLineDelay =
		static_cast<_float>(iLineCount - 1) *
		Desc.fLineInterval;
	const _float fTotalDuration =
		fLastLineDelay + Desc.fWaveDuration;

	Group.fPatternElapsed += fTimeDelta;

	for (const STEP_DATA& StepData :
		Group.vecSteps)
	{
		auto* pStep = CGameInstance::Get()
			.GetGameObjectByHandleT<
			CMyMagicSquareStep>(
				StepData.hStep);
		if (!pStep)
		{
			SetGroupFailed(GroupID, Group);
			return;
		}

		uint32_t iLineIndex =
			Desc.eAxis == FILL_AXIS::X ?
			StepData.iIndexX :
			StepData.iIndexZ;
		if (Desc.eDirection ==
			FILL_DIRECTION::REVERSE)
		{
			iLineIndex =
				iLineCount - 1 - iLineIndex;
		}

		const _float fLocalTime =
			Group.fPatternElapsed -
			static_cast<_float>(iLineIndex) *
			Desc.fLineInterval;
		if (fLocalTime > 0.f &&
			Group.setIssuedLineEvents.insert(
				iLineIndex).second)
		{
			QueuePatternEvent(
				GroupID,
				Group,
				PATTERN_TYPE::WAVE,
				PATTERN_EVENT::LINE_ISSUED,
				StepData.vPatternBasePosition,
				iLineIndex,
				StepData.hStep);
		}

		_float fOffsetY{};
		if (fLocalTime > 0.f &&
			fLocalTime < Desc.fWaveDuration)
		{
			const _float fRatio =
				fLocalTime /
				Desc.fWaveDuration;
			fOffsetY =
				XMScalarSin(XM_PI * fRatio) *
				Desc.fAmplitude;
		}

		_float3 vPosition =
			StepData.vPatternBasePosition;
		vPosition.y += fOffsetY;
		pStep->SetKinematicPosition(vPosition);
	}

	if (Group.fPatternElapsed < fTotalDuration)
		return;

	QueuePatternEvent(
		GroupID,
		Group,
		PATTERN_TYPE::WAVE,
		PATTERN_EVENT::COMPLETED,
		CalculateGroupCenter(Group));
	Group.eState = GROUP_STATE::PATTERN_COMPLETE;
	Group.oWavePattern.reset();
}

void CMyMagicSquareStepController::SetGroupFailed(
	StringID GroupID,
	GROUP& Group)
{
	if (Group.eState == GROUP_STATE::FAILED)
		return;

	if (Group.oRisePattern)
	{
		QueuePatternEvent(
			GroupID,
			Group,
			PATTERN_TYPE::RISE,
			PATTERN_EVENT::FAILED,
			CalculateGroupCenter(Group));
	}
	else if (Group.oWavePattern)
	{
		QueuePatternEvent(
			GroupID,
			Group,
			PATTERN_TYPE::WAVE,
			PATTERN_EVENT::FAILED,
			CalculateGroupCenter(Group));
	}

	Group.eState = GROUP_STATE::FAILED;
}

void CMyMagicSquareStepController::QueuePatternEvent(
	StringID GroupID,
	GROUP& Group,
	PATTERN_TYPE ePatternType,
	PATTERN_EVENT eEvent,
	const _float3& vPosition,
	uint32_t iLineIndex,
	CHandle hStep)
{
	const PATTERN_EVENT_CALLBACK* pCallback{};
	if (ePatternType == PATTERN_TYPE::RISE &&
		Group.oRisePattern)
	{
		pCallback =
			&Group.oRisePattern->fnEventCallback;
	}
	else if (ePatternType == PATTERN_TYPE::WAVE &&
		Group.oWavePattern)
	{
		pCallback =
			&Group.oWavePattern->fnEventCallback;
	}

	if (!pCallback || !*pCallback)
		return;

	m_vecPendingPatternEvents.push_back({
		.Callback = *pCallback,
		.Data = {
			.GroupID = GroupID,
			.ePatternType = ePatternType,
			.eEvent = eEvent,
			.vPosition = vPosition,
			.iLineIndex = iLineIndex,
			.hStep = hStep
		}
		});
}

void CMyMagicSquareStepController::
DispatchPendingPatternEvents()
{
	if (m_vecPendingPatternEvents.empty())
		return;

	auto PendingEvents =
		std::move(m_vecPendingPatternEvents);
	m_vecPendingPatternEvents.clear();

	for (const auto& PendingEvent : PendingEvents)
	{
		if (!PendingEvent.Callback ||
			m_mapGroup.find(
				PendingEvent.Data.GroupID) ==
			m_mapGroup.end())
		{
			continue;
		}

		PendingEvent.Callback(PendingEvent.Data);
	}
}

_float3 CMyMagicSquareStepController::CalculateGroupCenter(
	const GROUP& Group) const
{
	_float3 vCenter{};
	if (!Group.vecSteps.empty())
	{
		for (const STEP_DATA& StepData : Group.vecSteps)
		{
			vCenter.x += StepData.vPatternBasePosition.x;
			vCenter.y += StepData.vPatternBasePosition.y;
			vCenter.z += StepData.vPatternBasePosition.z;
		}

		const _float fInverseCount =
			1.f / static_cast<_float>(
				Group.vecSteps.size());
		vCenter.x *= fInverseCount;
		vCenter.y *= fInverseCount;
		vCenter.z *= fInverseCount;
		return vCenter;
	}

	if (Group.vecLayoutPoints.empty())
		return vCenter;

	for (const LAYOUT_POINT& Point :
		Group.vecLayoutPoints)
	{
		vCenter.x += Point.vPosition.x;
		vCenter.y += Point.vPosition.y;
		vCenter.z += Point.vPosition.z;
	}

	const _float fInverseCount =
		1.f / static_cast<_float>(
			Group.vecLayoutPoints.size());
	vCenter.x *= fInverseCount;
	vCenter.y *= fInverseCount;
	vCenter.z *= fInverseCount;
	return vCenter;
}

uint32_t CMyMagicSquareStepController::
CalculateRiseLineIndex(
	const GROUP& Group,
	const STEP_DATA& StepData,
	const RISE_PATTERN_DESC& Desc) const
{
	if (Desc.eFillMode == RISE_FILL_MODE::RADIAL)
	{
		const uint32_t iMaxLine =
			static_cast<uint32_t>(std::ceil(
				std::max(0.f,
					Group.fMaxRadialDistance)));
		uint32_t iLineIndex =
			static_cast<uint32_t>(std::floor(
				std::max(0.f,
					StepData.fRadialDistance)));
		iLineIndex = std::min(
			iLineIndex,
			iMaxLine);
		if (Desc.eDirection ==
			FILL_DIRECTION::REVERSE)
		{
			iLineIndex = iMaxLine - iLineIndex;
		}
		return iLineIndex;
	}

	const _bool bFillX =
		Desc.eFillMode == RISE_FILL_MODE::X;
	const uint32_t iLineCount =
		bFillX ?
		Group.iGridCountX :
		Group.iGridCountZ;
	uint32_t iLineIndex =
		bFillX ?
		StepData.iIndexX :
		StepData.iIndexZ;
	if (Desc.eDirection ==
		FILL_DIRECTION::REVERSE &&
		iLineCount > 0)
	{
		iLineIndex =
			iLineCount - 1 - iLineIndex;
	}
	return iLineIndex;
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
