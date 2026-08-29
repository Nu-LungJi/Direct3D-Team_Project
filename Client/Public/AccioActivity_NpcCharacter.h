#pragma once

#include "Client_Defines.h"
#include "WorldAgent.h"

NS_BEGIN(Client)

// [LSY] 아씨오 액티비티의 화면 표시와 물리 표현을 담당하는 NPC 캐릭터다.
// 전략과 턴 판단은 CAccioActivity_NpcController에 남기고, 이 객체는 위치·회전·애니메이션만 담당한다.
class CAccioActivity_NpcCharacter final : public CWorldAgent
{
public:
	DECLARE_DERIVED_TYPE(CAccioActivity_NpcCharacter, CWorldAgent)

	enum class ACTION : uint8_t
	{
		IDLE,
		MOVE,
		AIM,
		PULL
	};

	enum class FOOT_COLLISION : uint32_t
	{
		LEFT = 1,
		RIGHT
	};

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		_string sResourceGroup{};
		_string sModelResourceTag{};
		_string sWeaponResourceTag{ "PLAYER_WEAPON_SKELETON_RESOURCE" };
		_string sWeaponLayerTag{ "02_Npc" };
		_float3 vInitialPosition{};
		_float3 vInitialRotation{};
		_float3 vInitialScale{ 1.f, 1.f, 1.f };
		_float fPullHoldRatio{ 0.55f };
		_float fTurnSpeed{ 180.f };
		// [LSY] 플레이어와 같은 스케일의 학생 모델을 다리까지 감싸는 CCT 규격이다.
		_float fCCTHeight{ 3.6f };
		_float fCCTRadius{ 0.6f };
		_float fCCTStepOffset{ 0.1f };
		_float3 vCCTCenterOffset{ 0.f, 0.6f, 0.f };
		PX_FILTER_DESC tPhysicsFilter{
			.iLayer = ETOUI(COLLISION_LAYER::NPC_BODY),
			.iSimulationMask = PX_ALL_LAYERS,
			.iQueryMask =
				ETOUI(COLLISION_LAYER::WORLD_STATIC) |
				ETOUI(COLLISION_LAYER::WORLD_DYNAMIC) |
				ETOUI(COLLISION_LAYER::MOVING_PLATFORM) |
				ETOUI(COLLISION_LAYER::PLAYER_BODY) |
				ETOUI(COLLISION_LAYER::ENEMY_BODY)
		};
	};

private:
	CAccioActivity_NpcCharacter();
	CAccioActivity_NpcCharacter(const CAccioActivity_NpcCharacter& prototype);
	~CAccioActivity_NpcCharacter() override = default;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void FixedUpdate(_float fTimeDelta) override;
	void PriorityUpdate(_float fTimeDelta) override;
	void Update(_float fTimeDelta) override;
	void LateUpdate(_float fTimeDelta) override;
	void UpdateGUI() override;
	void OnTriggerEnter(
		CGameObject* pObj,
		const PX_ON_TRIGGER_DATA& info) override;

	void SetAction(ACTION eAction);
	void SetWorldPosition(const _float3& vWorldPosition);
	void SetMoveIntent(const _float3& vDirection, _float fSpeed);
	void FaceTowards(const _float3& vWorldPosition);
	_bool IsPullAnimationFinished() const;
	const CHandle& GetWeaponHandle() const { return m_hWeapon; }
	_bool TryGetWandSpawnWorldMatrix(_float4x4& outWorld) const;
	_bool PlayDialogueAnimation(const _string& sAnimationName, _bool bLoop);

public:
	static UPtr<CAccioActivity_NpcCharacter> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	void OnRegisteredToManager() override;
	int32_t FindAnimationIndex(const _char* pAnimationName) const;
	int32_t FindWandAttachBoneIndex() const;
	void PlayAction(ACTION eAction, _float fBlendDuration = 0.15f);
	void UpdatePullAnimation();
	void UpdateFootGroundContact(
		FOOT_COLLISION eFoot,
		const CComPxSphereCollider* pFootCollider,
		_bool& bWasGrounded);
	void PlayFootstepSound(FOOT_COLLISION eFoot);
	static const _char* GetActionName(ACTION eAction);

private:
	int32_t m_iIdleAnimation{ -1 };
	int32_t m_iMoveAnimation{ -1 };
	int32_t m_iAimAnimation{ -1 };
	int32_t m_iPullAnimation{ -1 };
	ACTION m_eAction{ ACTION::IDLE };
	ACTION m_ePendingAction{ ACTION::IDLE };
	_float m_fPullHoldRatio{ 0.55f };
	_float m_fTurnSpeed{ 180.f };
	_float3 m_vCCTCenterOffset{ 0.f, 0.6f, 0.f };
	_bool m_bFinishingPull{};
	_bool m_bDialogueAnimationPlaying{};
	_float m_fFootstepSoundCooldown{};
	CComPxSphereCollider* m_pComPxLeftFootCollider{};
	CComPxSphereCollider* m_pComPxRightFootCollider{};
	int32_t m_iLeftFootBoneIndex{ -1 };
	int32_t m_iRightFootBoneIndex{ -1 };
	_bool m_bLeftFootGroundContact{};
	_bool m_bRightFootGroundContact{};
	_string m_sResourceGroup{};
	_string m_sWeaponResourceTag{};
	_string m_sWeaponLayerTag{};
	CHandle m_hWeapon{};
	int32_t m_iWandAttachBoneIndex{ -1 };
	int32_t m_iWandTipBoneIndex{ -1 };
};

NS_END
