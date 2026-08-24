#pragma once

#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)
class CComSound;
NS_END

NS_BEGIN(Client)

// [LSY] 봄바르다 투사체의 이동, Sweep 충돌, 비행/트레일/임팩트 연출을 한곳에서 관리한다.
// [LSY] 리지드바디 대신 FixedUpdate의 이동 구간을 Sphere Sweep하여 빠른 이동 중 관통을 방지한다.
class CPlayer_Bombarda_Bullet final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_Bombarda_Bullet, CGameObject)

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		_float3 vStartPosition{};
		_float3 vEndPosition{ 0.f, 0.f, 30.f };
		CHandle hOwner{};
		_float fSpeed{ 110.f };
		_float fLifeTime{ 5.f };
		_float fRadius{ 0.35f };
		_float fCurveAmplitude{ 0.45f };
		_float fCurveFrequency{ 1.75f };
		uint32_t iPathSampleCount{ 64 };
		_float fTrailSpacing{ 0.18f };
		_bool bDebugDraw{ false };
		_string sProjectileEffectName{ "Bombarda_Projectile" };
		_string sTrailParticleQueue{ "LSY_Bombarda_Projectile_Trail_Queue.json" };
		_string sImpactEffectName{ "BombardaImapact" };

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
	CPlayer_Bombarda_Bullet();
	CPlayer_Bombarda_Bullet(const CPlayer_Bombarda_Bullet& Prototype);
	~CPlayer_Bombarda_Bullet() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void OnRegisteredToManager() override;
	void FixedUpdate(_float fTimeDelta) override;
	void Update(_float fTimeDelta) override;
	void LateUpdate(_float fTimeDelta) override;
	void UpdateGUI() override;

private:
	// [LSY] 현재 Fixed 구간에서 충돌할 가장 가까운 지점을 검사한다.
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
	void FinishAt(
		const _float3& vFinalPosition,
		const _float3& vImpactPosition,
		const _float3& vImpactNormal);
	void FinishWithoutImpact();
	void StartProjectileEffect();
	void StopProjectileEffect();
	void UpdateProjectileEffect();
	void StartFlightSound();
	void StopFlightSound();
	void UpdateFlightSound();
	void PlayImpactSounds(const _float3& vPosition) const;
	void EmitTrailBetween(
		const _float3& vPreviousPosition,
		const _float3& vCurrentPosition);
	void SpawnTrailParticle(const _float3& vPosition) const;
	void PlayImpactEffect(
		const _float3& vPosition,
		const _float3& vNormal) const;
	static _float4x4 MakeOrientedWorld(
		const _float3& vPosition,
		const _float3& vForward);

private:
	// [LSY] 경로 샘플과 이동 진행도는 FixedUpdate에서만 변경한다.
	_float3 m_vDirection{ 0.f, 0.f, 1.f };
	_float3 m_vEndPosition{ 0.f, 0.f, 30.f };
	_float m_fRemainingDistance{};
	CHandle m_hOwner{};
	_float m_fSpeed{ 110.f };
	_float m_fLifeTime{ 5.f };
	_float m_fElapsedTime{};
	_float m_fRadius{ 0.35f };
	_float m_fTrailSpacing{ 0.18f };
	_float m_fTrailDistanceAccumulator{};
	std::vector<_float3> m_PathPoints{};
	size_t m_iPathSegmentIndex{};
	_float m_fDistanceOnPathSegment{};
	_string m_sProjectileEffectName{};
	_string m_sTrailParticleQueue{};
	_string m_sImpactEffectName{};
	PX_QUERY_FILTER_DESC m_tQueryFilter{};
	EFFECT_INSTANCE_ID m_iProjectileEffectID{ INVALID_EFFECT_INSTANCE_ID };
	CComSound* m_pComSound{};
	_bool m_bFinished{};
	_bool m_bDebugDraw{ true };

public:
	static UPtr<CPlayer_Bombarda_Bullet> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
