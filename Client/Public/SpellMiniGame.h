#pragma once

#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Client)

class CSpellMiniGame final : public E::CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CSpellMiniGame, E::CGameObject)

	enum class STATE
	{
		WAITING,
		RUNNING,
		COMPLETED
	};

private:
	struct PATH_SAMPLE
	{
		_float2 Position{};
		_float AccumulatedDistance{};
	};

	struct BOOST_PAD
	{
		_float PathDistance{};
		_ubyte KeyCode{};
		CHandle BackdropHandle{};
		CHandle Handle{};
		_bool Consumed{};
	};

private:
	CSpellMiniGame();
	CSpellMiniGame(const CSpellMiniGame& rhs);
	~CSpellMiniGame() override = default;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(_float fTimeDelta) override;
	void Update(_float fTimeDelta) override;
	void LateUpdate(_float fTimeDelta) override;
	HRESULT Render(
		ID3D11DeviceContext* pContext,
		const E::RENDER_CTX& ctx) override;

	STATE GetState() const { return m_eState; }
	_float GetProgress() const;
	void ResetToStart();

public:
	static E::UPtr<CSpellMiniGame> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;

private:
	void Free() override;
	_bool CreateVisuals();
	void DestroyVisuals();
	void BuildIncendioPath();
	void AppendLine(
		const _float2& start,
		const _float2& end,
		uint32_t sampleCount);
	void AppendQuadraticBezier(
		const _float2& start,
		const _float2& control,
		const _float2& end,
		uint32_t sampleCount);
	_float2 EvaluatePosition(_float distance) const;
	_float2 EvaluateForward(_float distance) const;
	void UpdateArrowVisual(
		const _float2& position,
		const _float2& facingDirection);
	void TryActivateBoostPad(_bool aPressed, _bool xPressed);
	void ActivateBoost();
	void UpdateChaser(_float fTimeDelta);
	void ResetChaser();
	void SetChaserVisible(_bool visible);
	void ResetBoostPads();
	void SetStartPadVisible(_bool visible);
	void SetCursorVisible(_bool visible);
	void DestroyUIHandle(CHandle& handle);

private:
	std::vector<PATH_SAMPLE> m_PathSamples{};
	STATE m_eState{ STATE::WAITING };

	_float2 m_vPathTopLeft{};
	_float m_fPathSize{};
	_float m_fPathDistance{};
	_float m_fTotalPathDistance{};
	_float m_fBoostTimeRemaining{};
	_float m_fChaserPathDistance{};
	_float2 m_vLastFacingDirection{ 0.f, -1.f };
	_bool m_bChaserActive{};
	std::vector<BOOST_PAD> m_BoostPads{};

	CHandle m_hPath{};
	CHandle m_hPathProgress{};
	CHandle m_hChaserPathProgress{};
	CHandle m_hChaserCursor{};
	CHandle m_hDestinationSpellMeter{};
	CHandle m_hDestinationSpellMeterBorder{};
	CHandle m_hArrow{};
	CHandle m_hCursor{};
	CHandle m_hStartPadBackdrop{};
	CHandle m_hStartPad{};

	static constexpr _float MIN_MOVE_SPEED = 35.f;
	static constexpr _float MAX_MOVE_SPEED = 120.f;
	static constexpr _float ALIGNMENT_DEAD_ZONE = 0.12f;
	// cos(15 degrees): keep maximum speed inside the +/-15 degree cone.
	static constexpr _float MAX_SPEED_ALIGNMENT = 0.9659258f;
	static constexpr _float MOUSE_DIRECTION_MIN_DISTANCE = 8.f;
	static constexpr _float CORNER_STEERING_SAMPLE_DISTANCE = 24.f;
	static constexpr _float CURSOR_SIZE = 24.f;
	static constexpr _float CURSOR_ARROW_SIZE = 72.f;
	static constexpr _float CURSOR_ARROW_ORBIT_RADIUS = 14.f;
	static constexpr _float DESTINATION_SPELL_METER_SIZE = 120.f;
	static constexpr _float DESTINATION_SPELL_METER_BORDER_SIZE = 136.f;
	static constexpr _float BOOST_PAD_ICON_SIZE = 36.f;
	static constexpr _float BOOST_PAD_BACKDROP_SIZE = 50.f;
	static constexpr _float BOOST_PAD_TRIGGER_RANGE = 36.f;
	static constexpr _float BOOST_DURATION = 0.85f;
	static constexpr _float BOOST_MAX_MULTIPLIER = 2.f;
	static constexpr _float CHASER_MOVE_SPEED = 100.f;
	static constexpr _float CHASER_COLLISION_DISTANCE = 28.f;
	static constexpr _float CHASER_CURSOR_SIZE = 36.f;
};

NS_END
