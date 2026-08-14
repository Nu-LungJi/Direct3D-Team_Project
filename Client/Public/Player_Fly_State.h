

#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"

NS_BEGIN(Client)

class CPlayer;

class CPlayer_Fly_State final : public CState
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_Fly_State, CState)


	// 빗자루 탑승부터 해제까지의 비행 진행 단계.
	enum class FLIGHT_PHASE : uint32_t
	{
		LIFTING,
		MOUNTING,
		HOVER,
		INTO_FLY,
		FLYING,
		INTO_HOVER,
		DISMOUNTING,
	};

private:
	CPlayer_Fly_State();
	~CPlayer_Fly_State() override = default;

public:
	void Enter(CStateMachine* pStateMachine) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Exit(CStateMachine* pStateMachine) override;

	static SPtr<CPlayer_Fly_State> Create();

private:
	void CacheAnimationIndices(const CPlayer& player);
	int32_t ResolveFlightAnimation() const;
	int32_t FindAnimationIndex(const CPlayer& player, const _string_view& sAnimationName) const;

	int32_t m_iHoverAnimation{ -1 };
	int32_t m_iMountHoverAnimation{ -1 };
	int32_t m_iMountJogAnimation{ -1 };
	int32_t m_iDismountAnimation{ -1 };
	int32_t m_iIntoFlyAnimation{ -1 };
	int32_t m_iIntoHoverAnimation{ -1 };
	int32_t m_iSlowForwardAnimation{ -1 };
	int32_t m_iFastForwardAnimation{ -1 };
	int32_t m_iTurboForwardAnimation{ -1 };
	int32_t m_iActiveAnimation{ -1 };
	_bool m_bAnimationIndicesCached{};
	FLIGHT_PHASE m_eFlightPhase{ FLIGHT_PHASE::MOUNTING };
	_float m_fLiftElapsed{};
	_float m_fAppliedLiftHeight{};
	_float3 m_vFlightDirection{};
	_float3 m_vLastFlightDirection{ 0.f, 0.f, 1.f };
	_float m_fCurrentFlightSpeed{};
	_bool m_bBoosting{};
	_bool m_bMountFromMovement{};
	_float3 m_vMountGlideDirection{};
	_float m_fMountGlideSpeed{};

	// 탑승 자세의 발과 망토가 지면에 붙지 않도록 충분히 띄운 뒤 탑승한다.
	_float m_fMountInitialHeight{ 0.75f };
	_float m_fMountLiftHeight{ 2.7f };
	_float m_fMountLiftDuration{ 0.55f };
	_float m_fMountGlideMinSpeedRatio{ 0.15f };
	_float m_fMountControlEnableRatio{ 0.6f };
	_float m_fDismountInitialFallSpeed{ 1.5f };
	_float m_fCruiseFlightSpeed{ 12.f };
	_float m_fBoostFlightSpeed{ 36.f };
	_float m_fFlightAcceleration{ 10.f };
	_float m_fBoostFlightAcceleration{ 26.f };
	_float m_fFlightDeceleration{ 7.f };
	_float m_fFlightDirectionResponse{ 3.5f };
	_float m_fHoverSpeedThreshold{ 0.25f };
	_float m_fTurboAnimationSpeedThreshold{ 24.f };
	_float m_fFacingTurnSpeed{ 150.f };

};

NS_END
