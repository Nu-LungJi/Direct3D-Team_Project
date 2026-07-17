#pragma once
#include "Component.h"
#include "ResPhysXMaterial.h"
NS_BEGIN(physx)
class PxRigidActor;
class PxController;
//class PxControllerCollisionFlags;
NS_END


NS_BEGIN(Engine)

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
	};
public:
	DECLARE_DERIVED_TYPE(CComPxCharacterController, CComponent)

public:
	void UpdateGUI() override;

public:
	physx::PxController* GetController() const { return m_pController; }
private:
	explicit CComPxCharacterController();
	CComPxCharacterController(const CComPxCharacterController& rhs);
	~CComPxCharacterController() override;

private:
	HRESULT Initialize(void* pArg) override;

public:
	// 핵심 이동 함수 (FixedUpdate에서 호출)
	void Move(const XMFLOAT3& vDisplacement, float fTimeStep);

	// 바닥에 닿아있는지 여부
	bool IsGrounded() const;
	void SetPosition(const XMFLOAT3& vPosition);
private:
	physx::PxController* m_pController{};

	struct Impl;
	std::unique_ptr<Impl> m_pImpl;
	//uint8_t  m_CollisionFlags{}; // 이거 노출 안하려고

public:
	static UPtr<CComPxCharacterController> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	void Free() override;
};

NS_END
