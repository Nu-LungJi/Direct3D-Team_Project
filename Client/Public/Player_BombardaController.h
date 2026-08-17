#pragma once

#include "Engine_Base.h"
#include "Client_Defines.h"

NS_BEGIN(Client)

class CPlayer;

// [LSY] 플레이어의 봄바르다 캐스팅 연출과 투사체 생성을 전담한다.
// [LSY] 상태 클래스는 타이밍 신호만 전달하고, 지팡이 위치 추적과 이펙트 수명은 여기서 관리한다.
class CPlayer_BombardaController final : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_BombardaController, CEngineBase)

private:
	explicit CPlayer_BombardaController(CPlayer& Owner);
	~CPlayer_BombardaController() override = default;
	CPlayer_BombardaController(const CPlayer_BombardaController&) = delete;
	CPlayer_BombardaController& operator=(
		const CPlayer_BombardaController&) = delete;

public:
	// [LSY] 캐스팅 중인 연출을 매 프레임 지팡이 끝에 맞춘다.
	void Update();
	// [LSY] 캐스팅 Cue에서 호출한다.
	void StartCastEffect();
	// [LSY] 발사 또는 상태 종료 시 남은 캐스팅 연출을 정리한다.
	void StopCastEffect();
	// [LSY] 발사 Cue에서 타깃을 다시 확인하고 머즐과 투사체를 생성한다.
	_bool FireProjectile();

public:
	static UPtr<CPlayer_BombardaController> Create(CPlayer& Owner);

private:
	HRESULT Initialize();
	void EnsureCastParticleCommandsLoaded();
	_bool TryGetWandWorld(_float4x4& OutWorld) const;
	_bool ResolveTargetPosition(
		const _float3& vStartPosition,
		_float3& OutTargetPosition) const;
	void EmitCastParticleCurve() const;
	void EmitCastEnergyTrail(const _float3& vWandPosition);

protected:
	void Free() override;

private:
	// [LSY] 비소유 참조이며 Player::Free()에서 플레이어보다 먼저 컨트롤러를 해제한다.
	CPlayer& m_Owner;
	_bool m_bCastActive{};
	_bool m_bTrailRegistrationFailureLogged{};
	EFFECT_INSTANCE_ID m_iCastEffectID{ INVALID_EFFECT_INSTANCE_ID };
	// [LSY] 빠른 지팡이 이동에서도 입자가 끊기지 않도록 최근 위치를 Catmull-Rom으로 보간한다.
	std::vector<SPAWN_COMMAND> m_CastParticleCommands{};
	std::array<_float3, 4> m_CastTrailControlPoints{};
	_float m_fCastParticleSpacing{ 0.12f };
};

NS_END
