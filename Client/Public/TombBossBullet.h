#pragma once

#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)
class CComPathPlayback;
class CResPathPlayback;
NS_END

NS_BEGIN(Client)

class CTombBossBullet final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CTombBossBullet, CGameObject)

	enum class MOVE_STATE : uint8_t
	{
		PATH_PLAYBACK,
		TARGET_ARC,
		FINISHED
	};

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		_float3 vInitialPosition{};
		CHandle hTarget{};
		_float3 vTargetOffset{ 0.f, 1.5f, 0.f };
		_float fArcMoveSpeed{ 10.f };
		_float fArcHeight{ 3.f };
		_float fArcLifeTime{ 3.f };
		_float fRadius{ 0.5f };
		_float fPlaybackRate{ 1.f };
		_string sEffectName{ "Boss_StarBurst_A" };
		PX_QUERY_FILTER_DESC tQueryFilter{
			.iQueryMask =
				ETOUI(COLLISION_LAYER::WORLD_STATIC) |
				ETOUI(COLLISION_LAYER::PLAYER_HURTBOX),
			.bQueryStatic = true,
			.bQueryDynamic = true,
			.bIncludeTrigger = false
		};
	};

private:
	CTombBossBullet();
	CTombBossBullet(const CTombBossBullet& Prototype);
	~CTombBossBullet() override = default;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void FixedUpdate(_float fTimeDelta) override;
	void LateUpdate(_float fTimeDelta) override;
	void UpdateGUI() override;

private:
	HRESULT BuildTestPathResource();
	void FixedUpdatePathPlayback(_float fTimeDelta);
	void BeginTargetArc();
	void FixedUpdateTargetArc(_float fTimeDelta);
	void HandleCommittedPathEvents();
	_bool SweepTo(
		const _float3& vTargetPosition,
		PX_SWEEP_RESULT& OutHit) const;
	void FinishByHit(const PX_SWEEP_RESULT& Hit);
	void UpdateEffectTransform();

private:
	SPtr<CResPathPlayback> m_pPathResource{};
	CComPathPlayback* m_pComPathPlayback{};

	MOVE_STATE m_eMoveState{ MOVE_STATE::PATH_PLAYBACK };
	CHandle m_hTarget{};
	_float3 m_vTargetOffset{ 0.f, 1.5f, 0.f };
	_float3 m_vArcStartPosition{};
	_float3 m_vArcDirection{};
	_float m_fArcMoveSpeed{ 10.f };
	_float m_fArcHeight{ 3.f };
	_float m_fArcLifeTime{ 3.f };
	_float m_fArcElapsedTime{};
	_float m_fRadius{ 0.5f };
	PX_QUERY_FILTER_DESC m_tQueryFilter{};
	EFFECT_INSTANCE_ID m_iEffectID{ INVALID_EFFECT_INSTANCE_ID };

public:
	static UPtr<CTombBossBullet> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
