#pragma once
#include "Component.h"

namespace physx
{
	class PxJoint;
	class PxRigidActor;
}

NS_BEGIN(Engine)

class CComPxCharacterController;
class CComPxRigidBody;

class ENGINE_DLL CComPxJoint abstract : public CComponent
{
public:
	enum class ACTOR
	{
		A,
		B
	};

	struct FRAME
	{
		// 연결 Actor의 로컬 공간에서 정의하는 Joint 기준점과 기준축.
		_float3 vPosition{};
		_float4 vRotation{ 0.f, 0.f, 0.f, 1.f };
	};

	struct DESC : public CComponent::DESC
	{
		// A/B 슬롯마다 RigidBody와 CCT 중 하나만 지정한다. nullptr 슬롯은 월드로 취급된다.
		CComPxRigidBody* pRigidBodyA{};
		CComPxRigidBody* pRigidBodyB{};
		CComPxCharacterController* pCharacterControllerA{};
		CComPxCharacterController* pCharacterControllerB{};

		// 각 Endpoint의 로컬 공간에 배치되는 Joint Frame.
		FRAME tLocalFrameA{};
		FRAME tLocalFrameB{};

		// Constraint가 견딜 수 있는 최대 힘/토크. 기본값은 사실상 파괴되지 않는 값이다.
		_float fBreakForce{ std::numeric_limits<_float>::max() };
		_float fBreakTorque{ std::numeric_limits<_float>::max() };

		// 이 Joint를 풀 때만 사용하는 Endpoint별 역질량/역관성 배율.
		_float fInvMassScaleA{ 1.f };
		_float fInvMassScaleB{ 1.f };
		_float fInvInertiaScaleA{ 1.f };
		_float fInvInertiaScaleB{ 1.f };

		// 한 오브젝트가 가진 여러 Joint를 파괴 콜백에서 구분하는 식별값.
		uint32_t iJointSubIndex{ std::numeric_limits<uint32_t>::max() };

		// 연결 Actor끼리의 충돌, 디버그 시각화, Constraint 활성화 여부.
		_bool bCollisionEnabled{};
		_bool bVisualizationEnabled{ true };
		_bool bEnabled{ true };
	};

public:
	DECLARE_DERIVED_TYPE(CComPxJoint, CComponent)

public:
	void UpdateGUI() override;

public:
	_bool IsValid() const { return m_pJoint != nullptr; }
	_bool IsBroken() const;

	_bool SetEnabled(_bool bEnabled);
	_bool IsEnabled() const;
	_bool SetCollisionEnabled(_bool bEnabled);
	_bool IsCollisionEnabled() const;
	_bool SetVisualizationEnabled(_bool bEnabled);
	_bool IsVisualizationEnabled() const;

	_bool SetBreakForce(_float fForce, _float fTorque);
	_bool GetBreakForce(_float& fOutForce, _float& fOutTorque) const;
	_bool SetLocalFrame(ACTOR eActor, const FRAME& tFrame);
	_bool SetInverseMassScale(ACTOR eActor, _float fScale);
	_bool SetInverseInertiaScale(ACTOR eActor, _float fScale);

protected:
	explicit CComPxJoint();
	explicit CComPxJoint(const CComPxJoint& Prototype);
	~CComPxJoint() override;

protected:
	HRESULT Initialize(void* pArg) override;
	_bool AttachJoint(physx::PxJoint* pJoint);

	physx::PxRigidActor* GetActorA() const;
	physx::PxRigidActor* GetActorB() const;
	physx::PxJoint* GetJoint() const { return m_pJoint; }
	const FRAME& GetLocalFrameA() const { return m_tLocalFrameA; }
	const FRAME& GetLocalFrameB() const { return m_tLocalFrameB; }

private:
	void ReleaseJoint();
	void OnRigidBodyReleased(CComPxRigidBody* pRigidBody);
	void OnCharacterControllerReleased(
		CComPxCharacterController* pCharacterController);

private:
	physx::PxJoint* m_pJoint{};
	CComPxRigidBody* m_pRigidBodyA{};
	CComPxRigidBody* m_pRigidBodyB{};
	CComPxCharacterController* m_pCharacterControllerA{};
	CComPxCharacterController* m_pCharacterControllerB{};
	FRAME m_tLocalFrameA{};
	FRAME m_tLocalFrameB{};
	DESC m_tSettings{};
	PX_JOINT_USER_DATA m_tUserData{};

protected:
	void Free() override;

private:
	friend class CComPxRigidBody;
	friend class CComPxCharacterController;
};

NS_END
