#pragma once

#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)
class CComSound;
NS_END

NS_BEGIN(Client)

// [LSY] 콘프링고 투사체의 경로 이동, Sphere Sweep 충돌, 비행/트레일/임팩트 연출을 담당한다.
// 리지드 바디 투사체가 아니며 FixedUpdate에서 경로 구간마다 Sweep Query로 충돌을 검사한다.
class CPlayer_Confringo_Bullet final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_Confringo_Bullet, CGameObject)

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		_float3 vStartPosition{};               // 발사 순간 지팡이 끝의 월드 위치
		_float3 vEndPosition{ 0.f, 0.f, 30.f }; // 발사 순간 확정한 목표 월드 위치
		CHandle hOwner{};                       // 발사자 핸들, Sweep Query에서 제외
		_float fSpeed{ 90.f };                  // 경로를 따라 이동하는 초당 거리
		_float fLifeTime{ 5.f };                // 충돌하지 않았을 때의 최대 생존 시간
		_float fRadius{ 0.25f };                // Sphere Sweep 반지름
		_float fCurveAmplitude{ 0.35f };        // 직선 경로에 더할 흔들림 크기
		_float fCurveFrequency{ 1.75f };        // 경로 흔들림 횟수
		uint32_t iPathSampleCount{ 64 };         // 곡선 경로를 나눌 구간 수
		_bool bDebugDraw{ true };                // Sweep 구체와 전체 경로 표시 여부
		PLAYER_SKILL_TYPE eSkillType{ PLAYER_SKILL_TYPE::ATTACK }; // 피격 처리에 전달할 스킬 타입
		_string sProjectileEffectName{ "Confringo_Projectile" };  // 투사체를 따라가는 Effect 이름
		_string sTrailParticleQueue{ "LSY_Confringo_Projectile_Trail_Queue.json" }; // 이동 경로 Queue
		_string sImpactEffectName{ "Confringo_Impact" };          // 충돌 또는 목표 도달 Effect
		_float fTrailSpacing{ 0.32f };          // 이동 거리 기준 트레일 생성 간격

		// [LSY] 월드와 적만 검사하고 발사자 자신과 Trigger Shape은 제외한다.
		PX_QUERY_FILTER_DESC tQueryFilter{
			.iQueryMask =
				ETOUI(COLLISION_LAYER::WORLD_STATIC) |
				ETOUI(COLLISION_LAYER::WORLD_DYNAMIC) |
				ETOUI(COLLISION_LAYER::ENEMY_BODY) |
				ETOUI(COLLISION_LAYER::ENEMY_HURTBOX),
			.bQueryStatic = true,
			.bQueryDynamic = true,
			.bIncludeTrigger = false
		};
	};

private:
	CPlayer_Confringo_Bullet();
	CPlayer_Confringo_Bullet(const CPlayer_Confringo_Bullet& Prototype);
	~CPlayer_Confringo_Bullet() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	// [LSY] 유효한 오브젝트 핸들이 발급된 뒤 비행 이펙트와 3D 사운드를 시작한다.
	void OnRegisteredToManager() override;
	// [LSY] 고정 시간으로 경로를 진행하며 이동 구간마다 Sphere Sweep을 수행한다.
	void FixedUpdate(_float fTimeDelta) override;
	// [LSY] 이동 중인 비행 사운드의 3D 위치를 갱신한다.
	void Update(_float fTimeDelta) override;
	// [LSY] Transform을 확정하고 요청된 경우 Sweep 구체와 경로를 시각화한다.
	void LateUpdate(_float fTimeDelta) override;
	void UpdateGUI() override;

private:
	// [LSY] 두 경로 위치 사이를 구형 Sweep하여 가장 가까운 충돌 결과를 얻는다.
	_bool SweepTo(
		const _float3& vStartPosition,
		const _float3& vTargetPosition,
		PX_SWEEP_RESULT& OutHit) const;
	void BuildDynamicPath(
		const _float3& vStartPosition,
		const _float3& vEndPosition,
		_float fCurveAmplitude,
		_float fCurveFrequency,
		uint32_t iSampleCount);
	void UpdateFlightTransform(
		const _float3& vPosition,
		const _float3& vDirection);
	// [LSY] 충돌 위치 확정, 비행 연출 종료, 피격 처리, 임팩트 재생을 한 번만 수행한다.
	void HandleHit(const PX_SWEEP_RESULT& Hit);
	// [LSY] 피해·상태 이상 등 충돌 후 게임플레이 로직을 확장하는 지점이다.
	void ProcessHitGameplay(const PX_SWEEP_RESULT& Hit);
	// [LSY] 목표점 도달은 임팩트를 재생하고, 수명 종료는 조용히 제거한다.
	void FinishAtEndPosition();
	void FinishWithoutHit();
	void StartProjectileEffect();
	void StopProjectileEffect();
	void UpdateProjectileEffect();
	void StartFlightSound();
	void StopFlightSound();
	void UpdateFlightSound();
	void PlayImpactSounds(const _float3& vPosition) const;
	// [LSY] 프레임 시간이 아니라 실제 이동 거리를 기준으로 일정 간격의 트레일을 생성한다.
	void EmitTrailBetween(
		const _float3& vPreviousPosition,
		const _float3& vCurrentPosition);
	void SpawnTrailParticle(const _float3& vPosition) const;
	void PlayImpactEffect(
		const _float3& vPosition,
		const _float3& vNormal);
	static _float4x4 MakeOrientedWorld(
		const _float3& vPosition,
		const _float3& vForward);

private:
	// [LSY] 현재 진행 방향과 발사 시 확정한 목표 및 경로 진행 상태다.
	_float3 m_vDirection{ 0.f, 0.f, 1.f };
	_float3 m_vEndPosition{ 0.f, 0.f, 30.f };
	_float m_fRemainingDistance{};
	CHandle m_hOwner{};
	_float m_fSpeed{ 90.f };
	_float m_fLifeTime{ 5.f };
	_float m_fElapsedTime{};
	_float m_fRadius{ 0.25f };
	std::vector<_float3> m_PathPoints{};
	size_t m_iPathSegmentIndex{};
	_float m_fDistanceOnPathSegment{};
	// [LSY] 피격 판정과 비행/임팩트 연출 설정이다.
	PLAYER_SKILL_TYPE m_eSkillType{ PLAYER_SKILL_TYPE::ATTACK };
	_string m_sProjectileEffectName{};
	_string m_sTrailParticleQueue{};
	_string m_sImpactEffectName{};
	_float m_fTrailSpacing{ 0.32f };
	_float m_fTrailDistanceAccumulator{};
	PX_QUERY_FILTER_DESC m_tQueryFilter{};
	// [LSY] 이동 중 함께 갱신하고 종료 시 정리할 런타임 이펙트와 사운드 상태다.
	EFFECT_INSTANCE_ID m_iProjectileEffectID{ INVALID_EFFECT_INSTANCE_ID };
	CComSound* m_pComSound{};
	_bool m_bFinished{};
	_bool m_bDebugDraw{ true };

public:
	static UPtr<CPlayer_Confringo_Bullet> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
