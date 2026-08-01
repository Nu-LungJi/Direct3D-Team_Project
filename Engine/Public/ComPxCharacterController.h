#pragma once
#include "Component.h"
#include "Handle.h"
#include "ResPhysXMaterial.h"
NS_BEGIN(physx)
class PxRigidActor;
class PxController;
//class PxControllerCollisionFlags;
NS_END


NS_BEGIN(Engine)

class CComPxJoint;

enum class PX_CCT_COLLISION_FLAG : uint8_t
{
	NONE = 0,
	SIDE = 1 << 0,
	UP = 1 << 1,
	DOWN = 1 << 2
};

class ENGINE_DLL CComPxCharacterController : public CComponent
{
public:
	struct DESC : public CComponent::DESC
	{
		SPtr<CResPhysXMaterial> pResMaterial{};
		float fHeight = 2.0f;
		float fRadius = 0.5f;
		float fStepOffset = 0.1f;      // 오를 수 있는 계단 높이
		float fSlopeLimit = 0.707f;    // 오를 수 있는 최대 경사 (cos각도, 0.707은 약 45도)
		XMFLOAT3 vPosition = { 0.f, 0.f, 0.f };
		PX_FILTER_DESC tFilter{};
		uint32_t iShapeSubIndex{
			std::numeric_limits<uint32_t>::max() };
	};
public:
	DECLARE_DERIVED_TYPE(CComPxCharacterController, CComponent)

public:
	void UpdateGUI() override;

public:
	physx::PxController* GetController() const { return m_pController; }
	physx::PxRigidActor* GetActor() const;
private:
	explicit CComPxCharacterController();
	CComPxCharacterController(const CComPxCharacterController& rhs);
	~CComPxCharacterController() override;

private:
	HRESULT Initialize(void* pArg) override;

public:
	PX_CCT_COLLISION_FLAG Move(const XMFLOAT3& vDisplacement, float fTimeStep, float fMinDistance = 0.f);
	bool IsGrounded() const;
	bool IsCollidingUp() const;
	bool IsCollidingSide() const;
	std::optional<CHandle> GetStandingGameObjectHandle() const;
	void SetPosition(const XMFLOAT3& vPosition);
	_float3 GetPosition() const;
	_float3 GetFootPosition() const;

	_bool Resize(_float fHeight);
	_bool SetRadius(_float fRadius);
	void SetStepOffset(_float fStepOffset);
	_float GetStepOffset() const;
	void SetSlopeLimit(_float fSlopeLimit);
	_float GetSlopeLimit() const;
	void SetContactOffset(_float fContactOffset);
	_float GetContactOffset() const;

	_bool SetFilter(const PX_FILTER_DESC& tFilter);
	const PX_FILTER_DESC& GetFilter() const { return m_tFilter; }
	uint32_t GetShapeSubIndex() const { return m_iShapeSubIndex; }
private:
	physx::PxController* m_pController{};
	PX_FILTER_DESC m_tFilter{};
	uint32_t m_iShapeSubIndex{
		std::numeric_limits<uint32_t>::max() };
	std::unordered_set<CComPxJoint*> m_Joints{};

	struct Impl;
	std::unique_ptr<Impl> m_pImpl;
	//uint8_t  m_CollisionFlags{}; // 이거 노출 안하려고

private:
	void RegisterJoint(CComPxJoint* pJoint);
	void UnregisterJoint(CComPxJoint* pJoint);
	void ReleaseConnectedJoints();

public:
	static UPtr<CComPxCharacterController> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	void Free() override;

	friend class CComPxJoint;
};

NS_END
