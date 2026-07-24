#pragma once

#include "GameObject.h"

NS_BEGIN(Client)

class CMagicSquareStepController final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CMagicSquareStepController, CGameObject)

	enum class PATTERN
	{
		COMBAT_CIRCLE,
		TARGET_BRIDGE,
		PLAYER_WAVE_BRIDGE
	};

	enum class STATE
	{
		DORMANT,
		SUMMONING,
		PATTERN_TRANSITION,
		RUNNING,
		DISAPPEARING
	};

	struct COMMON_DESC
	{
		_float fSpacing{ 1.007f };
		_float fHiddenDepth{ 5.f };
		_float fSummonSpeed{ 4.f };
		_float fSummonWaveDelay{ 0.75f };
		_float fPatternMoveSpeed{ 2.f };
		_bool bEnablePhysics{ true };
	};

	struct COMBAT_CIRCLE_DESC
	{
		CHandle hPlayer{};
		_float3 vCenter{};
		_float fRadius{ 13.f };
		_float fInfluenceRadius{ 3.f };
		_float fRaiseHeight{ 1.f };
		_float fFollowDelay{ 0.3f };
	};

	struct TARGET_BRIDGE_DESC
	{
		_float3 vSourceA{ 0.f, 3.f, 0.f };
		_float3 vSourceB{ 13.f, 2.f, 0.f };
		_float3 vTargetA{ 0.f, 1.f, 0.f };
		_float3 vTargetB{ 13.f, 2.f, 0.f };
		uint32_t iWidthCount{ 5 };
		_float fWidthSpacing{ 1.007f };
		_float fWaveDelay{ 1.f };
	};

	struct PLAYER_WAVE_BRIDGE_DESC
	{
		CHandle hPlayer{};
		_float3 vAnchorA{ 0.f, 3.f, 0.f };
		_float3 vAnchorB{ 13.f, 3.f, 0.f };
		uint32_t iWidthCount{ 5 };
		_float fWidthSpacing{ 1.007f };
		_float fInfluenceRadius{ 3.f };
		_float fRaiseHeight{ 1.f };
		_float fFollowDelay{ 0.3f };
	};

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		PATTERN ePattern{ PATTERN::COMBAT_CIRCLE };
		COMMON_DESC tCommon{};
		COMBAT_CIRCLE_DESC tCombatCircle{};
		TARGET_BRIDGE_DESC tTargetBridge{};
		PLAYER_WAVE_BRIDGE_DESC tPlayerWaveBridge{};
	};

private:
	struct STEP_DATA
	{
		CHandle hStep{};
		_float3 vHiddenPosition{};
		_float3 vBasePosition{};
		_float3 vPatternPosition{};
		_float fProgress{};
		_float fSummonDelay{};
	};

private:
	CMagicSquareStepController();
	CMagicSquareStepController(
		const CMagicSquareStepController& prototype);
	~CMagicSquareStepController() override = default;

public:
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(_float fTimeDelta) override;
	void UpdateGUI() override;

	void Activate();
	void Deactivate();
	PATTERN GetPattern() const { return m_ePattern; }
	STATE GetState() const { return m_eState; }

public:
	static UPtr<CMagicSquareStepController> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	HRESULT ValidateDesc(const DESC& Desc) const;
	HRESULT CreatePatternSteps(const DESC& Desc);
	HRESULT CreateCombatCircle(const DESC& Desc);
	HRESULT CreateTargetBridge(const DESC& Desc);
	HRESULT CreatePlayerWaveBridge(const DESC& Desc);
	HRESULT CreateRectSteps(
		const _float3& vBaseA,
		const _float3& vBaseB,
		const _float3& vPatternA,
		const _float3& vPatternB,
		uint32_t iWidthCount,
		_float fWidthSpacing,
		_float fWaveDelay);
	HRESULT CreateStep(
		const _float3& vBasePosition,
		const _float3& vPatternPosition,
		_float fProgress,
		_float fSummonDelay);

	void UpdateSummoning();
	void UpdatePatternTransition();
	void UpdateDynamicPattern(_float fTimeDelta);
	void UpdateDisappearing();
	void SetStepTarget(
		const STEP_DATA& Step,
		const _float3& vTarget,
		_float fSpeed);
	void ClearSteps();

private:
	std::vector<STEP_DATA> m_Steps{};
	PATTERN m_ePattern{ PATTERN::COMBAT_CIRCLE };
	STATE m_eState{ STATE::DORMANT };
	COMMON_DESC m_tCommon{};
	COMBAT_CIRCLE_DESC m_tCombatCircle{};
	TARGET_BRIDGE_DESC m_tTargetBridge{};
	PLAYER_WAVE_BRIDGE_DESC m_tPlayerWaveBridge{};
	_float m_fStateTime{};
	_float m_fStateDuration{};
	_float3 m_vFollowPosition{};
	_bool m_bFollowInitialized{};
};

NS_END
