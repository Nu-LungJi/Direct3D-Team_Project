#pragma once

#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Client)

class CAccioActivity_Base;
class CAccioBall;
class CAccioActivity_Platform;

// [LSY] 아씨오 액티비티의 NPC 턴만 담당한다.
// 실제 공 이동과 점수 판정은 각각 CAccioBall과 CAccioActivity_Base가 수행한다.
class CAccioActivity_Npc final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CAccioActivity_Npc, CGameObject)

	enum class STATE : uint8_t
	{
		IDLE,
		MOVING,
		AIMING,
		PULLING,
		RETURNING,
		WAIT_BALL_SETTLED
	};

	enum class TACTIC : uint8_t
	{
		SCORE,
		ATTACK
	};

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		CHandle hActivity{};
		CHandle hPlatform{};
		_float3 vInitialPosition{};
		_float3 vInitialRotation{};
		_float fTurnStartDelay{ 1.f };
		_float fMoveSpeed{ 5.f };
		_float fMoveAcceleration{ 9.f };
		_float fMoveDeceleration{ 14.f };
		_float fMoveArrivalDistance{ 0.1f };
		_float fMoveAreaMargin{ 0.75f };
		_float fAimDelay{ 0.6f };
		_float fScorePullDuration{ 1.35f };
		_float fMaximumAttackEdgeDistance{ 14.f };
		_float fEstimatedAttackPullSpeed{ 18.f };
		_float fAttackEdgeHoldSecondsPerUnit{ 0.04f };
		_float fMinimumAttackPullDuration{ 1.f };
		_float fMaximumAttackPullDuration{ 2.5f };
		_float fPullTimeout{ 3.f };
		int32_t iMinimumAttackTargetScore{ 30 };
		_bool bDebugDraw{ true };
	};

private:
	CAccioActivity_Npc();
	CAccioActivity_Npc(const CAccioActivity_Npc& prototype);
	~CAccioActivity_Npc() override = default;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void FixedUpdate(_float fTimeDelta) override;
	void LateUpdate(_float fTimeDelta) override;
	void UpdateGUI() override;

	STATE GetState() const { return m_eState; }
	CHandle GetActiveBallHandle() const { return m_hActiveBall; }

protected:
	void OnRegisteredToManager() override;

private:
	struct ATTACK_PLAN
	{
		CHandle hTargetBall{};
		_float fLocalNpcX{};
		_float fCollisionDistance{};
		_float fTargetDistanceToEdge{};
		_float fPullDuration{};
	};

	void UpdateNpcTurn(CAccioActivity_Base& activity, _float fTimeDelta);
	void AbortNpcTurn(CAccioActivity_Base& activity);
	_bool BeginReturnToRest();
	void UpdateReturnToRest(_float fTimeDelta, _bool bWaitForBall);
	void ResetTurnState();
	void SanitizeTuning();
	_bool PrepareMoveTarget(
		const CAccioActivity_Base& activity,
		const CAccioBall& ball);
	std::optional<ATTACK_PLAN> BuildAttackPlan(
		const CAccioActivity_Base& activity,
		const CAccioBall& ball,
		const _matrix& inverseMoveArea,
		_float fUsableHalfX) const;
	_float CalculateAttackPullDuration(
		_float fCollisionDistance,
		_float fTargetDistanceToEdge) const;
	_bool IsAtPreparedMoveTarget() const;
	_bool MoveToPreparedTarget(_float fTimeDelta);
	void FaceTowards(const _float3& vWorldPosition);
	_bool AcquireSelectedBall();
	_bool ReleaseSelectedBall();
	CAccioBall* GetSelectedBall() const;
	CAccioBall* GetDisruptionTargetBall() const;
	void DrawDebugState() const;
	static const _char* GetStateName(STATE eState);

public:
	static UPtr<CAccioActivity_Npc> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	CHandle m_hActivity{};
	CHandle m_hPlatform{};
	CHandle m_hActiveBall{};
	CHandle m_hDisruptionTargetBall{};
	STATE m_eState{ STATE::IDLE };
	TACTIC m_eTactic{ TACTIC::SCORE };
	_float3 m_vMoveTarget{};
	_float3 m_vRestFacingTarget{};
	_float m_fStateElapsed{};
	_float m_fTurnStartDelay{ 1.f };
	_float m_fMoveSpeed{ 5.f };
	_float m_fMoveAcceleration{ 9.f };
	_float m_fMoveDeceleration{ 14.f };
	_float m_fCurrentMoveSpeed{};
	_float m_fMoveArrivalDistance{ 0.1f };
	_float m_fMoveAreaMargin{ 0.75f };
	_float m_fAimDelay{ 0.6f };
	_float m_fScorePullDuration{ 1.35f };
	_float m_fMaximumAttackEdgeDistance{ 14.f };
	_float m_fEstimatedAttackPullSpeed{ 18.f };
	_float m_fAttackEdgeHoldSecondsPerUnit{ 0.04f };
	_float m_fMinimumAttackPullDuration{ 1.f };
	_float m_fMaximumAttackPullDuration{ 2.5f };
	_float m_fPullTimeout{ 3.f };
	_float m_fPlannedPullDuration{ 1.35f };
	_float m_fPlannedCollisionDistance{};
	_float m_fPlannedTargetEdgeDistance{};
	int32_t m_iMinimumAttackTargetScore{ 30 };
	_bool m_bDebugDraw{ true };
};

NS_END
