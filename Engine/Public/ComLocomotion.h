#pragma once

#include "Component.h"

NS_BEGIN(Engine)

class ENGINE_DLL CComLocomotion final : public CComponent
{
public:
	struct DESC : public CComponent::DESC
	{
	};

	struct OUTPUT
	{
		_float3 vMoveDirection{};
		_float fMoveSpeed{};
		_bool bMoveRequested{};

		_float3 vFacingDirection{};
		_float fTurnSpeed{};
		_bool bFacingRequested{};
		_bool bImmediateFacing{};
	};

public:
	DECLARE_DERIVED_TYPE(CComLocomotion, CComponent)

private:
	explicit CComLocomotion();
	CComLocomotion(const CComLocomotion& rhs);
	~CComLocomotion() override;

private:
	HRESULT Initialize(void* pArg) override;

public:
	void SetMoveIntent(const _float3& vDirection, _float fSpeed);
	void ClearMoveIntent();
	void SetFacingIntent(const _float3& vDirection, _float fTurnSpeed);
	void SetFacingIntentImmediate(const _float3& vDirection);
	void ClearFacingIntent();
	const OUTPUT& GetOutput() const { return m_tOutput; }

	void RequestJump() { m_bJumpRequested = true; }
	_bool HasJumpRequest() const { return m_bJumpRequested; }
	_bool ConsumeJumpRequest();
	void ClearJumpRequest() { m_bJumpRequested = false; }

public:
	void UpdateGUI() override;

private:
	OUTPUT m_tOutput{};
	_bool m_bJumpRequested{};

public:
	static UPtr<CComLocomotion> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	void Free() override;
};

NS_END
