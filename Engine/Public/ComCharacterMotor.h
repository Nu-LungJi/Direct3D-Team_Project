#pragma once

#include "Component.h"
#include "ComPxCharacterController.h"

NS_BEGIN(Engine)

class CComCharacterMoveIntent;

class ENGINE_DLL CComCharacterMotor final : public CComponent
{
public:
	struct DESC : public CComponent::DESC
	{
		CComCharacterMoveIntent* pMoveIntent{};
		CComPxCharacterController* pCharacterController{};
		_float fGravity{ -9.81f };
		_float fJumpVelocity{ 5.f };
		_float fMinMoveDistance{};
		// GameObject 원점에서 CCT 중심까지의 월드 축 기준 오프셋.
		_float3 vControllerCenterOffset{};
		_bool bUseGravity{ true };
		_bool bSyncTransform{ true };
	};

public:
	DECLARE_DERIVED_TYPE(CComCharacterMotor, CComponent)

private:
	explicit CComCharacterMotor();
	CComCharacterMotor(const CComCharacterMotor& rhs);
	~CComCharacterMotor() override;

private:
	HRESULT Initialize(void* pArg) override;

public:
	void FixedUpdate(_float fFixedTimeDelta);
	void UpdateGUI() override;

	void SetVelocity(const _float3& vVelocity) { m_vVelocity = vVelocity; }
	const _float3& GetVelocity() const { return m_vVelocity; }
	void SetGravity(_float fGravity) { m_fGravity = fGravity; }
	_float GetGravity() const { return m_fGravity; }
	void SetUseGravity(_bool bUseGravity) { m_bUseGravity = bUseGravity; }
	_bool IsUsingGravity() const { return m_bUseGravity; }
	void SetPreserveHorizontalVelocity(_bool bPreserve) { m_bPreserveHorizontalVelocity = bPreserve; }
	_bool IsPreservingHorizontalVelocity() const { return m_bPreserveHorizontalVelocity; }
	_bool IsGrounded() const { return m_bGrounded; }
	PX_CCT_COLLISION_FLAG GetLastCollisionFlag() const { return m_eLastCollisionFlag; }

private:
	CComCharacterMoveIntent* m_pMoveIntent{};
	CComPxCharacterController* m_pCharacterController{};
	_float3 m_vVelocity{};
	_float m_fGravity{ -9.81f };
	_float m_fJumpVelocity{ 5.f };
	_float m_fMinMoveDistance{};
	_float3 m_vControllerCenterOffset{};
	_bool m_bUseGravity{ true };
	_bool m_bPreserveHorizontalVelocity{};
	_bool m_bSyncTransform{ true };
	_bool m_bGrounded{};
	PX_CCT_COLLISION_FLAG m_eLastCollisionFlag{ PX_CCT_COLLISION_FLAG::NONE };

public:
	static UPtr<CComCharacterMotor> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	void Free() override;
};

NS_END
