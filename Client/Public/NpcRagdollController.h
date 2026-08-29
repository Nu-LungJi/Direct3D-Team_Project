#pragma once

#include "Engine_Base.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CComPxRagdoll;
NS_END

NS_BEGIN(Client)
class CWorldAgent;

// CComPxRagdoll의 범용 물리 기능과 CWorldAgent 계열의 AI/CCT 생명주기를 연결한다.
// 랙돌 컴포넌트의 소유권은 각 CWorldAgent 파생 객체의 컴포넌트 컨테이너에 있다.
class CNpcRagdollController final : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CNpcRagdollController, CEngineBase)

private:
	explicit CNpcRagdollController(CWorldAgent& Owner);
	~CNpcRagdollController() override = default;
	CNpcRagdollController(const CNpcRagdollController&) = delete;
	CNpcRagdollController& operator=(const CNpcRagdollController&) = delete;

public:
	void UpdateGUI();

	// 최신 애니메이션 포즈가 캐시되는 Update 끝에서 실제 랙돌로 전환한다.
	_bool RequestActivation(
		const _float3& vLinearVelocity = {},
		const _float3& vAngularVelocityRadians = {});
	_bool RequestFromCurrentMotion();
	_bool Reset();

	_bool IsActive() const;
	_bool IsTransitioning() const;

	// true면 호출자가 일반 AI/CCT 루프를 건너뛰어야 한다.
	_bool PrePriorityUpdate();
	_bool PreFixedUpdate();
	void PostFixedUpdate();
	void UpdatePoseBridge();

public:
	static UPtr<CNpcRagdollController> Create(CWorldAgent& Owner);

private:
	HRESULT Initialize();
	_bool EnsureInitialized();
	_bool TryActivateRequested();
	void DisableGameplayPhysics();
	void RestoreGameplayPhysics();

private:
	CWorldAgent& m_Owner;
	CComPxRagdoll* m_pComPxRagdoll{};
	_bool m_bInitializationAttempted{};
	_bool m_bInitialized{};

	_bool m_bActivationRequested{};
	_bool m_bGameplayPhysicsDisabled{};
	_bool m_bRootMotionRotationBeforeRagdoll{};
	_bool m_bRootMotionTranslationBeforeRagdoll{};
	_float3 m_vActivationLinearVelocity{};
	_float3 m_vActivationAngularVelocity{};
	PX_FILTER_DESC m_tCCTFilterBeforeRagdoll{};
	_bool m_bHurtBoxSimulationBeforeRagdoll{};
	_bool m_bHurtBoxQueryBeforeRagdoll{};
};

NS_END
