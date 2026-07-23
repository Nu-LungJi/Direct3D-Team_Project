#pragma once

#include "GameObject.h"

NS_BEGIN(Client)

class CMyMagicSquareStepController final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CMyMagicSquareStepController, CGameObject)

	enum class GROUP_STATE
	{
		REGISTERED,
		SPAWNING,
		READY,
		PATTERN_RUNNING,
		PATTERN_COMPLETE,
		FAILED
	};

	enum class FILL_AXIS
	{
		X,
		Z
	};

	enum class FILL_DIRECTION
	{
		FORWARD,
		REVERSE
	};

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		uint32_t iMaxSpawnPerFrame{ 50 };
	};

	struct GROUP_DESC
	{
		_float3 vStartPosition{};
		uint32_t iCountX{ 1 };
		uint32_t iCountZ{ 1 };
		_float fSpacingX{ 1.007f };
		_float fSpacingZ{ 1.007f };
	};

	struct RISE_PATTERN_DESC
	{
		_float fTargetY{};
		_float fMoveSpeed{ 2.f };
		_float fLineInterval{ 0.1f };
		FILL_AXIS eAxis{ FILL_AXIS::X };
		FILL_DIRECTION eDirection{
			FILL_DIRECTION::FORWARD };
	};

	struct STEP_DATA
	{
		CHandle hStep{};
		uint32_t iIndexX{};
		uint32_t iIndexZ{};
		_float3 vSpawnPosition{};
	};

	struct GROUP
	{
		GROUP_DESC tDesc{};
		std::vector<STEP_DATA> vecSteps{};
		GROUP_STATE eState{ GROUP_STATE::REGISTERED };
		size_t iTargetCount{};
		size_t iSpawnedCount{};
		std::optional<RISE_PATTERN_DESC> oRisePattern{};
		_float fPatternElapsed{};
		uint32_t iIssuedLineCount{};
	};

private:
	struct SPAWN_DATA
	{
		StringID GroupID{};
		_float3 vPosition{};
		uint32_t iIndexX{};
		uint32_t iIndexZ{};
	};

private:
	CMyMagicSquareStepController();
	CMyMagicSquareStepController(
		const CMyMagicSquareStepController& rhs);
	~CMyMagicSquareStepController() override = default;

public:
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(_float fTimeDelta) override;
	void Update(_float fTimeDelta) override;
	void UpdateGUI() override;

	_bool RegistGroup(
		StringID GroupID,
		const GROUP_DESC& Desc);
	_bool SpawnGroup(StringID GroupID);
	_bool DeleteGroup(StringID GroupID);
	_bool StartRisePattern(
		StringID GroupID,
		const RISE_PATTERN_DESC& Desc);

	std::optional<GROUP_STATE> GetGroupState(
		StringID GroupID) const;
	const std::vector<STEP_DATA>* GetGroupSteps(
		StringID GroupID) const;

public:
	static UPtr<CMyMagicSquareStepController> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	_bool SpawnOne(const SPAWN_DATA& Data);
	void UpdateRisePattern(
		GROUP& Group,
		_float fTimeDelta);
	_bool IssueRiseLine(
		GROUP& Group,
		uint32_t iLineIndex);

private:
	std::unordered_map<StringID, GROUP> m_mapGroup{};
	std::queue<SPAWN_DATA> m_qSpawn{};
	uint32_t m_iMaxSpawnPerFrame{ 50 };

private:
	_float m_fGUIRiseTargetY{ 3.f };
	_float m_fGUIMoveSpeed{ 2.f };
	_float m_fGUILineInterval{ 0.1f };
	FILL_AXIS m_eGUIFillAxis{ FILL_AXIS::X };
	FILL_DIRECTION m_eGUIFillDirection{
		FILL_DIRECTION::FORWARD };
};

NS_END
