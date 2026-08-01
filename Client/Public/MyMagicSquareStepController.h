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

	enum class RISE_FILL_MODE
	{
		X,
		Z,
		RADIAL
	};

	enum class FILL_DIRECTION
	{
		FORWARD,
		REVERSE
	};

	enum class LAYOUT_TYPE
	{
		RECT,
		FILLED_CIRCLE
	};

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		uint32_t iMaxSpawnPerFrame{ 50 };
		E::StringID ProtoMajorTag{};
		E::StringID ProtoMinorTag{};
		std::string SpawnLayerName{};
		StringID ResMajorTag{};
		StringID ResMinorTag{};
	};

	struct RECT_GROUP_DESC
	{
		_float3 vStartPosition{};
		uint32_t iCountX{ 1 };
		uint32_t iCountZ{ 1 };
		_float fSpacingX{ 1.007f };
		_float fSpacingZ{ 1.007f };
	};

	struct FILLED_CIRCLE_GROUP_DESC
	{
		_float3 vCenter{};
		_float fRadius{ 5.f };
		_float fSpacing{ 1.007f };
	};

	struct RISE_PATTERN_DESC
	{
		_float fStartTargetY{};
		_float fEndTargetY{};
		_float fMoveSpeed{ 2.f };
		_float fBounceHeight{};
		_float fBounceSettleSpeed{ 1.f };
		_float fLineInterval{ 0.1f };
		_float fStepInterval{ 0.02f };
		_float fStepTimingCurve{ 0.55f };
		_float fStepTimingJitter{ 0.01f };
		RISE_FILL_MODE eFillMode{
			RISE_FILL_MODE::X };
		FILL_AXIS eHeightAxis{ FILL_AXIS::X };
		FILL_DIRECTION eDirection{
			FILL_DIRECTION::FORWARD };
	};

	struct WAVE_PATTERN_DESC
	{
		_float fAmplitude{ 1.f };
		_float fWaveDuration{ 1.f };
		_float fLineInterval{ 0.1f };
		FILL_AXIS eAxis{ FILL_AXIS::X };
		FILL_DIRECTION eDirection{
			FILL_DIRECTION::FORWARD };
	};

	struct LAYOUT_POINT
	{
		_float3 vPosition{};
		uint32_t iIndexX{};
		uint32_t iIndexZ{};
		_float fNormalizedX{};
		_float fNormalizedZ{};
		_float fRadialDistance{};
		_float fNormalizedRadius{};
	};

	struct STEP_DATA
	{
		CHandle hStep{};
		uint32_t iIndexX{};
		uint32_t iIndexZ{};
		_float3 vSpawnPosition{};
		_float3 vPatternBasePosition{};
		_float fNormalizedX{};
		_float fNormalizedZ{};
		_float fRadialDistance{};
		_float fNormalizedRadius{};
		_bool bRiseIssued{};
	};

	struct GROUP
	{
		LAYOUT_TYPE eLayoutType{
			LAYOUT_TYPE::RECT };
		std::vector<LAYOUT_POINT> vecLayoutPoints{};
		std::vector<STEP_DATA> vecSteps{};
		_float fSpawnBaseY{};
		uint32_t iGridCountX{};
		uint32_t iGridCountZ{};
		_float fMaxRadialDistance{};
		GROUP_STATE eState{ GROUP_STATE::REGISTERED };
		size_t iTargetCount{};
		size_t iSpawnedCount{};
		std::optional<RISE_PATTERN_DESC> oRisePattern{};
		std::optional<WAVE_PATTERN_DESC> oWavePattern{};
		_float fPatternElapsed{};
		size_t iIssuedStepCount{};
	};

private:
	struct SPAWN_DATA
	{
		StringID GroupID{};
		_float3 vPosition{};
		uint32_t iIndexX{};
		uint32_t iIndexZ{};
		_float fNormalizedX{};
		_float fNormalizedZ{};
		_float fRadialDistance{};
		_float fNormalizedRadius{};
	};

private:
	CMyMagicSquareStepController();
	CMyMagicSquareStepController(
		const CMyMagicSquareStepController& rhs);
	~CMyMagicSquareStepController() override = default;

public:
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(_float fTimeDelta) override;
	void FixedUpdate(_float fTimeDelta) override;
	void Update(_float fTimeDelta) override;
	void UpdateGUI() override;

	_bool RegistRectGroup(
		StringID GroupID,
		const RECT_GROUP_DESC& Desc);
	_bool RegistFilledCircleGroup(
		StringID GroupID,
		const FILLED_CIRCLE_GROUP_DESC& Desc);
	_bool SpawnGroup(StringID GroupID);
	_bool DeleteGroup(StringID GroupID);
	_bool StartRisePattern(
		StringID GroupID,
		const RISE_PATTERN_DESC& Desc);
	_bool StartWavePattern(
		StringID GroupID,
		const WAVE_PATTERN_DESC& Desc);

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
	_bool IssueRiseStep(
		GROUP& Group,
		STEP_DATA& StepData);
	void UpdateWavePattern(
		GROUP& Group,
		_float fTimeDelta);

private:
	std::unordered_map<StringID, GROUP> m_mapGroup{};
	std::queue<SPAWN_DATA> m_qSpawn{};
	uint32_t m_iMaxSpawnPerFrame{ 50 };

private:
	_float m_fGUIStartTargetY{ -214.f };
	_float m_fGUIEndTargetY{ -214.f };
	_float m_fGUIMoveSpeed{ 2.f };
	_float m_fGUIBounceHeight{ 0.3f };
	_float m_fGUIBounceSettleSpeed{ 1.f };
	_float m_fGUILineInterval{ 0.1f };
	_float m_fGUIStepInterval{ 0.02f };
	_float m_fGUIStepTimingCurve{ 0.55f };
	_float m_fGUIStepTimingJitter{ 0.01f };
	RISE_FILL_MODE m_eGUIRiseFillMode{
		RISE_FILL_MODE::X };
	FILL_AXIS m_eGUIHeightAxis{ FILL_AXIS::X };
	FILL_DIRECTION m_eGUIFillDirection{
		FILL_DIRECTION::FORWARD };
	_float m_fGUIWaveAmplitude{ 1.f };
	_float m_fGUIWaveDuration{ 1.f };
	_float m_fGUIWaveLineInterval{ 0.1f };
	FILL_AXIS m_eGUIWaveAxis{ FILL_AXIS::X };
	FILL_DIRECTION m_eGUIWaveDirection{
		FILL_DIRECTION::FORWARD };

private:
	E::StringID m_StepProtoMajorTag{};
	E::StringID m_StepProtoMinorTag{};
	std::string m_StepSpawnLayerName{};
	StringID m_ResMajorTag{};
	StringID m_ResMinorTag{};
};

NS_END
