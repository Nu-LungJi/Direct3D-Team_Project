#pragma once

#include "Engine_Base.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CComPxRagdoll;
NS_END

NS_BEGIN(Client)
class CPlayer;

// [LSY] CComPxRagdoll의 범용 물리 기능과 CPlayer의 게임플레이 상태를 연결한다.
// [LSY] 실제 PhysX 컴포넌트의 소유권은 CPlayer의 컴포넌트 컨테이너에 있다.
class CPlayerRagdollController final : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CPlayerRagdollController, CEngineBase)

private:
	explicit CPlayerRagdollController(CPlayer& Owner);
	~CPlayerRagdollController() override = default;
	CPlayerRagdollController(const CPlayerRagdollController&) = delete;
	CPlayerRagdollController& operator=(const CPlayerRagdollController&) = delete;

public:
	void UpdateGUI();

	// [LSY] 랙돌 전환을 예약한다. 실제 활성화는 최신 애니메이션 포즈를 캐시한 뒤 수행한다.
	_bool RequestActivation(
		const _float3& vLinearVelocity = {},
		const _float3& vAngularVelocityRadians = {});
	// [LSY] CCT의 현재 속도를 제한된 초기 관성으로 변환하여 랙돌 전환을 예약한다.
	_bool RequestFromCurrentMotion();
	// [LSY] 랙돌을 키네마틱 포즈로 되돌리고 비활성화했던 플레이어 물리를 복원한다.
	_bool Reset();

	_bool IsActive() const;
	_bool IsTransitioning() const;
	// [LSY] 활성 랙돌의 카메라 추적용 Hips 월드 위치를 반환한다.
	_bool TryGetFollowPosition(_float3& OutPosition) const;

	// [LSY] true를 반환하면 해당 프레임의 일반 플레이어 로직을 중단한다.
	_bool PrePriorityUpdate();
	_bool PreFixedUpdate();
	// [LSY] 일반 CCT 물리 갱신 후 비활성 랙돌 바디를 최신 애니메이션 포즈에 맞춘다.
	void PostFixedUpdate();
	// [LSY] 비활성 상태에서는 애니메이션을 캐시하고 활성 상태에서는 물리 포즈를 본에 기록한다.
	void UpdatePoseBridge();

public:
	static UPtr<CPlayerRagdollController> Create(CPlayer& Owner);

private:
	HRESULT Initialize();
	_bool TryActivateRequested();
	void DisableGameplayPhysics();
	void RestoreGameplayPhysics();

private:
	// [LSY] 컨트롤러는 플레이어를 소유하지 않으며 플레이어보다 먼저 파괴되어야 한다.
	CPlayer& m_Owner;
	// [LSY] 컴포넌트의 실제 소유권은 CPlayer의 컴포넌트 컨테이너에 있다.
	CComPxRagdoll* m_pComPxRagdoll{};

	// [LSY] 전환 중 잠시 비활성화한 게임플레이 상태와 물리 필터를 복원하기 위한 캐시다.
	_bool m_bActivationRequested{};
	_bool m_bGameplayPhysicsDisabled{};
	_bool m_bMovementLockedBeforeRagdoll{};
	_bool m_bRootMotionRotationBeforeRagdoll{};
	_bool m_bRootMotionTranslationBeforeRagdoll{};
	_float3 m_vActivationLinearVelocity{};
	_float3 m_vActivationAngularVelocity{};
	PX_FILTER_DESC m_tCCTFilterBeforeRagdoll{};
	_bool m_bHurtBoxSimulationBeforeRagdoll{};
	_bool m_bHurtBoxQueryBeforeRagdoll{};

	// [LSY] 현재 랙돌 JSON의 첫 번째 바디는 Hips다. 데이터 명시 방식 도입 전까지 임시 사용한다.
	static constexpr size_t RAGDOLL_FOLLOW_BODY_INDEX = 0;
};

NS_END
