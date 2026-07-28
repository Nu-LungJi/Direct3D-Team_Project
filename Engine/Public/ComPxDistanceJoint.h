#pragma once
#include "ComPxJoint.h"

namespace physx
{
	class PxDistanceJoint;
}

NS_BEGIN(Engine)

class ENGINE_DLL CComPxDistanceJoint final : public CComPxJoint
{
public:
	struct DESC : public CComPxJoint::DESC
	{
		// 두 Joint Frame 사이에서 허용할 최소/최대 거리와 오차 허용량.
		_float fMinDistance{};
		_float fMaxDistance{ 1.f };
		_float fTolerance{ 0.025f };

		// Spring 활성화 시 사용하는 복원 강도와 진동 감쇠값.
		_float fStiffness{};
		_float fDamping{};

		// 사용할 거리 경계와 Spring Constraint를 선택한다.
		_bool bMinDistanceEnabled{};
		_bool bMaxDistanceEnabled{ true };
		_bool bSpringEnabled{};
	};

public:
	DECLARE_DERIVED_TYPE(CComPxDistanceJoint, CComPxJoint)

private:
	explicit CComPxDistanceJoint();
	explicit CComPxDistanceJoint(
		const CComPxDistanceJoint& Prototype);
	~CComPxDistanceJoint() override;

public:
	_float GetDistance() const;
	_float GetMinDistance() const;
	_float GetMaxDistance() const;
	_float GetTolerance() const;
	_float GetStiffness() const;
	_float GetDamping() const;

	_bool SetDistanceRange(
		_float fMinDistance,
		_float fMaxDistance);
	_bool SetMinDistance(_float fDistance);
	_bool SetMaxDistance(_float fDistance);
	_bool SetTolerance(_float fTolerance);
	_bool SetSpring(_float fStiffness, _float fDamping);

	_bool SetMinDistanceEnabled(_bool bEnabled);
	_bool IsMinDistanceEnabled() const;
	_bool SetMaxDistanceEnabled(_bool bEnabled);
	_bool IsMaxDistanceEnabled() const;
	_bool SetSpringEnabled(_bool bEnabled);
	_bool IsSpringEnabled() const;

	void UpdateGUI() override;

private:
	HRESULT Initialize(void* pArg) override;
	physx::PxDistanceJoint* GetDistanceJoint() const;

public:
	static UPtr<CComPxDistanceJoint> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	void Free() override;
};

NS_END
