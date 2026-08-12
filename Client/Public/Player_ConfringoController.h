#pragma once

#include "Engine_Base.h"
#include "Client_Defines.h"

NS_BEGIN(Client)

class CPlayer;

// [LSY] 플레이어의 콘프링고 시전 연출과 투사체 생성을 전담한다.
// 상태 머신은 CPlayer의 전달 API만 호출하고, 지팡이 위치 추적과 파티클 제어는 이 클래스가 처리한다.
class CPlayer_ConfringoController final : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_ConfringoController, CEngineBase)

private:
	explicit CPlayer_ConfringoController(CPlayer& Owner);
	~CPlayer_ConfringoController() override = default;
	CPlayer_ConfringoController(const CPlayer_ConfringoController&) = delete;
	CPlayer_ConfringoController& operator=(
		const CPlayer_ConfringoController&) = delete;

public:
	// [LSY] 콘프링고 수치 확인과 캐스팅 연출 단독 테스트용 GUI를 출력한다.
	void UpdateGUI();
	// [LSY] 최신 지팡이 위치로 Flame을 이동하고 이동 궤적에 Spark를 생성한다.
	void Update(_float fTimeDelta);
	// [LSY] 지팡이 끝에 캐스팅 연출을 시작한다. 중복 호출 시 기존 연출을 먼저 정리한다.
	void StartCastEffect();
	// [LSY] 컨트롤러가 소유한 캐스팅 파티클을 즉시 정리한다.
	void StopCastEffect();
	// [LSY] 현재 타깃 또는 플레이어 전방을 목표로 콘프링고 투사체를 생성한다.
	_bool FireProjectile();

public:
	static UPtr<CPlayer_ConfringoController> Create(CPlayer& Owner);

private:
	HRESULT Initialize();
	// [LSY] 캐스팅 Queue JSON은 최초 사용 시 한 번 파싱하여 재사용한다.
	_bool EnsureParticleCommandsLoaded();
	// [LSY] 플레이어 무기의 Spawn World 행렬에서 지팡이 끝 위치를 구한다.
	_bool TryGetWandPosition(_float3& OutPosition) const;
	// [LSY] 최근 지팡이 위치 네 점을 Catmull-Rom 곡선으로 연결해 Spark를 배치한다.
	void EmitSparkCurve();

protected:
	void Free() override;

private:
	// [LSY] 컨트롤러는 플레이어를 소유하지 않으며 플레이어보다 먼저 파괴한다.
	CPlayer& m_Owner;
	// [LSY] 최초 파싱 후 재사용하는 캐스팅 파티클 명령이다.
	std::vector<SPAWN_COMMAND> m_FlameCommands{};
	std::vector<SPAWN_COMMAND> m_SparkCommands{};
	// [LSY] Flame Owner ID를 보관하여 이동과 종료를 한 단위로 처리한다.
	uint32_t m_iFlameOwnerId{ INVALID_PARTICLE_OWNER_ID };
	_bool m_bCastEffectActive{};
	// [LSY] Spark 생성 빈도와 곡선 위 배치 간격이다.
	_float m_fSparkElapsed{};
	_float m_fSparkInterval{ 0.05f };
	_float m_fSparkTrailSpacing{ 0.08f };
	// [LSY] 지팡이 이동량 및 부드러운 Spark 궤적 계산에 사용하는 위치 기록이다.
	_float3 m_vPreviousWandPosition{};
	std::array<_float3, 4> m_SparkControlPoints{};
};

NS_END
