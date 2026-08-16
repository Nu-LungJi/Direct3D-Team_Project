#pragma once

#include "Engine_Base.h"
#include "Client_Defines.h"

NS_BEGIN(Client)

class CPlayer;

// [LSY] 아바다 케다브라의 완드 부착 연출과 빔/피격 연출을 전담한다.
// 상태 클래스는 애니메이션 Cue만 전달하고, 실제 연출의 위치와 수명은 여기서 관리한다.
class CPlayer_AvadaKedavraController final : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_AvadaKedavraController, CEngineBase)

private:
	explicit CPlayer_AvadaKedavraController(CPlayer& Owner);
	~CPlayer_AvadaKedavraController() override = default;
	CPlayer_AvadaKedavraController(
		const CPlayer_AvadaKedavraController&) = delete;
	CPlayer_AvadaKedavraController& operator=(
		const CPlayer_AvadaKedavraController&) = delete;

public:
	// [LSY] 캐스팅 중인 이펙트를 완드 끝에 갱신하고 예약된 피격 연출을 처리한다.
	void Update(_float fTimeDelta);
	// [LSY] 애니메이션 진입 Cue에서 완드 충전 이펙트를 시작한다.
	void StartCastEffect();
	// [LSY] 상태 중단 또는 방출 시 완드 충전 이펙트를 정리한다.
	void StopCastEffect();
	// [LSY] 현재 타깃을 향해 빔을 방출하고 피격 연출을 예약한다.
	_bool ReleaseSpell();

public:
	static UPtr<CPlayer_AvadaKedavraController> Create(CPlayer& Owner);

private:
	HRESULT Initialize();
	_bool TryGetWandWorld(_float4x4& OutWorld) const;
	_bool ResolveTargetPosition(
		const _float3& vStartPosition,
		_float3& OutTargetPosition) const;
	_float3 CalculateVisualTargetPosition(
		const _float3& vStartPosition,
		const _float3& vTargetPosition) const;
	void EmitCastTrail(const _float3& vWandPosition);
	void PlayImpactEffects(const _float3& vImpactPosition) const;
	void PlayImpactArcs(
		const _float4x4& impactWorld,
		const _float3& vImpactPosition) const;

protected:
	void Free() override;

private:
	// [LSY] 비소유 참조이며 Player::Free()에서 플레이어보다 먼저 컨트롤러를 해제한다.
	CPlayer& m_Owner;
	_bool m_bCastActive{};
	_bool m_bImpactPending{};
	_bool m_bTrailRegistrationFailureLogged{};
	_float m_fImpactDelayRemaining{};
	_float3 m_vPendingImpactPosition{};
	EFFECT_INSTANCE_ID m_iCastEffectID{ INVALID_EFFECT_INSTANCE_ID };

	// [LSY] 빔 방출 직후보다 약간 늦게 피격 연출을 시작해 타격의 선후 관계를 보이게 한다.
	static constexpr _float IMPACT_DELAY = 0.05f;
};

NS_END
