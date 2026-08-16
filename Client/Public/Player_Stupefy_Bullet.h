#pragma once
#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Client)

class CPlayer_Stupefy_Bullet final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_Stupefy_Bullet, CGameObject)
	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		_float3 vStartPosition{};
		_float3 vEndPosition{ 0.f, 0.f, 30.f };
		CHandle hOwner{};
		_float fSpeed{ 120.f };
		_float fLifeTime{ 2.f };
		_float fRadius{ 0.18f };
		_float fCurveAmplitude{ 0.08f };
		_float fCurveFrequency{ 1.2f };
		uint32_t iPathSampleCount{ 48 };
		PLAYER_SKILL_TYPE eSkillType{ PLAYER_SKILL_TYPE::ATTACK };
		_string sProjectileEffectName{ "KMS_Stupefy_Core" };
		_string sTrailParticleQueue{ "KMS_Stupefy_Trail" };
		_string sImpactEffectName{ "KMS_Stupefy_Impact" };
		_float fTrailSpacing{ 0.45f };
		_bool bDebugSphere{};
		_bool bDebugPath{};
		_bool bEnableSounds{};
		PX_QUERY_FILTER_DESC tQueryFilter{
			.iQueryMask = ETOUI(COLLISION_LAYER::WORLD_STATIC) |
				ETOUI(COLLISION_LAYER::WORLD_DYNAMIC) |
				ETOUI(COLLISION_LAYER::ENEMY_BODY) |
				ETOUI(COLLISION_LAYER::ENEMY_HURTBOX),
			.bQueryStatic = true,
			.bQueryDynamic = true,
			.bIncludeTrigger = false
		};
	};
private:
	CPlayer_Stupefy_Bullet() = default;
	CPlayer_Stupefy_Bullet(const CPlayer_Stupefy_Bullet& rhs) : CGameObject(rhs) {}
	~CPlayer_Stupefy_Bullet() override;
public:
	HRESULT InitializePrototype(void* pArg = nullptr) override { return S_OK; }
	HRESULT Initialize(void* pArg) override;
	void OnRegisteredToManager() override;
	void FixedUpdate(_float fTimeDelta) override;
	void LateUpdate(_float fTimeDelta) override;
	void UpdateGUI() override;
	static UPtr<CPlayer_Stupefy_Bullet> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
private:
	_bool SweepTo(const _float3& start, const _float3& end, PX_SWEEP_RESULT& hit) const;
	void BuildPath(const DESC& desc);
	void SetFlightTransform(const _float3& position, const _float3& direction);
	void EmitTrail(const _float3& start, const _float3& end);
	void Finish(const _float3& position, const _float3& normal, CGameObject* hitObject);
	void StopCoreEffect();
private:
	std::vector<_float3> m_Path{};
	size_t m_iSegment{};
	_float m_fSegmentDistance{};
	_float m_fRemainingDistance{};
	_float m_fElapsed{};
	_float m_fSpeed{};
	_float m_fLifeTime{};
	_float m_fRadius{};
	_float m_fTrailSpacing{};
	_float3 m_vDirection{ 0.f, 0.f, 1.f };
	CHandle m_hOwner{};
	PLAYER_SKILL_TYPE m_eSkillType{ PLAYER_SKILL_TYPE::ATTACK };
	_string m_sCoreEffect{};
	_string m_sTrailQueue{};
	_string m_sImpactEffect{};
	PX_QUERY_FILTER_DESC m_QueryFilter{};
	EFFECT_INSTANCE_ID m_iCoreEffect{ INVALID_EFFECT_INSTANCE_ID };
	_bool m_bFinished{};
	_bool m_bDebugSphere{};
	_bool m_bDebugPath{};
};

NS_END
