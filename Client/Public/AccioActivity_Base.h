#pragma once

#include "AccioActivityPartBase.h"

NS_BEGIN(Engine)
class CComPxBoxCollider;
class CComPxRigidBody;
NS_END

NS_BEGIN(Client)

class CAccioActivity_Base final : public CAccioActivityPartBase
{
public:
	DECLARE_DERIVED_TYPE(CAccioActivity_Base, CAccioActivityPartBase)

	enum class PARTICIPANT : uint8_t
	{
		NONE,
		PLAYER,
		NPC
	};

	enum class MATCH_STATE : uint8_t
	{
		READY,
		PLAYER_TURN,
		WAIT_PLAYER_BALL_SETTLED,
		NPC_TURN,
		WAIT_NPC_BALL_SETTLED,
		MATCH_END
	};

	struct DESC : public CAccioActivityPartBase::DESC
	{
		std::array<ACCIO_ACTIVITY_BOX_COLLIDER_DESC, 4> BoxColliders{
			ACCIO_ACTIVITY_BOX_COLLIDER_DESC{
				.vHalfExtents = { 11.75f, 1.f, 0.45f },
				.vLocalOffset = { 0.f, 3.1f, 29.15f } },
			ACCIO_ACTIVITY_BOX_COLLIDER_DESC{
				.vHalfExtents = { 0.3f, 1.f, 1.65f },
				.vLocalOffset = { -11.45f, 3.1f, 27.05f } },
			ACCIO_ACTIVITY_BOX_COLLIDER_DESC{
				.vHalfExtents = { 0.3f, 1.f, 1.65f },
				.vLocalOffset = { 11.4f, 3.1f, 27.05f } },
			ACCIO_ACTIVITY_BOX_COLLIDER_DESC{
				.vHalfExtents = { 11.2f, 0.9f, 22.6f },
				.vLocalOffset = { 0.f, 1.45f, 6.9f } }
		};
		ACCIO_ACTIVITY_BOX_COLLIDER_DESC Score10Trigger{
			.vHalfExtents = { 11.f, 2.f, 6.7f },
			.vLocalOffset = { 0.f, 3.f, 18.2f }
		};
		ACCIO_ACTIVITY_BOX_COLLIDER_DESC Score20Trigger{
			.vHalfExtents = { 11.f, 2.f, 5.75f },
			.vLocalOffset = { 0.f, 3.f, 5.75f }
		};
		ACCIO_ACTIVITY_BOX_COLLIDER_DESC Score30Trigger{
			.vHalfExtents = { 11.f, 2.f, 4.5f },
			.vLocalOffset = { 0.f, 3.f, -4.5f }
		};
		ACCIO_ACTIVITY_BOX_COLLIDER_DESC Score50Trigger{
			.vHalfExtents = { 11.f, 2.f, 3.35f },
			.vLocalOffset = { 0.f, 3.f, -12.35f }
		};
		PX_FILTER_DESC tPhysicsFilter{
			.iLayer = ETOUI(COLLISION_LAYER::WORLD_STATIC),
			.iSimulationMask = PX_ALL_LAYERS,
			.iQueryMask = PX_ALL_LAYERS
		};
		PX_FILTER_DESC tScore10TriggerFilter{
			.iLayer = ETOUI(COLLISION_LAYER::WORLD_STATIC),
			.iSimulationMask = ETOUI(COLLISION_LAYER::WORLD_DYNAMIC),
			.iQueryMask = PX_ALL_LAYERS
		};
		PX_FILTER_DESC tScore20TriggerFilter{
			.iLayer = ETOUI(COLLISION_LAYER::WORLD_STATIC),
			.iSimulationMask = ETOUI(COLLISION_LAYER::WORLD_DYNAMIC),
			.iQueryMask = PX_ALL_LAYERS
		};
		PX_FILTER_DESC tScore30TriggerFilter{
			.iLayer = ETOUI(COLLISION_LAYER::WORLD_STATIC),
			.iSimulationMask = ETOUI(COLLISION_LAYER::WORLD_DYNAMIC),
			.iQueryMask = PX_ALL_LAYERS
		};
		PX_FILTER_DESC tScore50TriggerFilter{
			.iLayer = ETOUI(COLLISION_LAYER::WORLD_STATIC),
			.iSimulationMask = ETOUI(COLLISION_LAYER::WORLD_DYNAMIC),
			.iQueryMask = PX_ALL_LAYERS
		};
	};

private:
	CAccioActivity_Base();
	CAccioActivity_Base(const CAccioActivity_Base& prototype);
	~CAccioActivity_Base() override = default;

public:
	static UPtr<CAccioActivity_Base> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
	void FixedUpdate(_float fTimeDelta) override;
	void UpdateGUI() override;

	void OnTriggerEnter(
		CGameObject* pObj,
		const PX_ON_TRIGGER_DATA& info) override;
	void OnTriggerExit(
		CGameObject* pObj,
		const PX_ON_TRIGGER_DATA& info) override;

	int32_t GetBlueScore() const { return m_iBlueScore; }
	int32_t GetRedScore() const { return m_iRedScore; }
	MATCH_STATE GetMatchState() const { return m_eMatchState; }
	uint32_t GetCurrentRound() const { return m_iCurrentRound; }
	_bool RegisterBall(const CHandle& hBall);
	void SetParticipantHandle(PARTICIPANT eParticipant, const CHandle& hObject);
	_bool StartMatch();
	void ResetMatch(_bool bResetBalls);
	_bool CanControlBall(
		const CHandle& hController,
		const CHandle& hBall) const;
	void NotifyBallControlAcquired(
		const CHandle& hController,
		const CHandle& hBall);
	void NotifyBallControlReleased(
		const CHandle& hController,
		const CHandle& hBall);

protected:
	StringID GetModelResourceTag() const override;

private:
	HRESULT InitializeBasePhysics(const DESC& desc);
	void UpdateBallScoreOverlap(
		CGameObject* pObj,
		const PX_ON_TRIGGER_DATA& info,
		_bool bEntered);
	void UpdateSettledScores();
	void RefreshScores();
	void UpdateTurnState();
	int32_t ResolveScoreAtPosition(const _float3& vWorldPosition) const;
	_bool IsPointInsideScoreZone(
		const _float3& vWorldPosition,
		const ACCIO_ACTIVITY_BOX_COLLIDER_DESC& zone) const;
	PARTICIPANT ResolveParticipant(const CHandle& hController) const;
	static const _char* GetMatchStateName(MATCH_STATE eState);
	static uint8_t GetScoreZoneBit(uint32_t iShapeSubIndex);

private:
	enum class SCORE_TEAM : uint8_t
	{
		NONE,
		BLUE,
		RED
	};

	struct BALL_SCORE_STATE
	{
		uint8_t iZoneMask{};
		SCORE_TEAM eTeam{ SCORE_TEAM::NONE };
		int32_t iCommittedScore{};
		_bool bScoreCommitted{};
	};

	struct HANDLE_HASH
	{
		size_t operator()(const CHandle& handle) const noexcept
		{
			size_t seed = std::hash<size_t>{}(handle.GetIndex());
			seed ^= std::hash<uint32_t>{}(handle.GetGeneration()) +
				0x9e3779b9u + (seed << 6) + (seed >> 2);
			return seed;
		}
	};

private:
	CComPxRigidBody* m_pComPxRigidBody{};
	std::array<CComPxBoxCollider*, 4> m_pComPxBoxColliders{};
	CComPxBoxCollider* m_pComPxScore10Trigger{};
	CComPxBoxCollider* m_pComPxScore20Trigger{};
	CComPxBoxCollider* m_pComPxScore30Trigger{};
	CComPxBoxCollider* m_pComPxScore50Trigger{};
	std::array<ACCIO_ACTIVITY_BOX_COLLIDER_DESC, 4> m_ScoreZones{};
	std::unordered_map<CHandle, BALL_SCORE_STATE, HANDLE_HASH> m_BallScoreStates{};
	std::unordered_set<CHandle, HANDLE_HASH> m_UsedBalls{};
	std::vector<CHandle> m_BallHandles{};
	CHandle m_hPlayer{};
	CHandle m_hNpc{};
	CHandle m_hActiveBall{};
	MATCH_STATE m_eMatchState{ MATCH_STATE::READY };
	uint32_t m_iCurrentRound{};
	uint32_t m_iMaxRounds{ 3u };
	int32_t m_iBlueScore{};
	int32_t m_iRedScore{};
};

NS_END
