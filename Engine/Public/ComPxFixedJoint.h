#pragma once
#include "ComPxJoint.h"

NS_BEGIN(Engine)

class ENGINE_DLL CComPxFixedJoint final : public CComPxJoint
{
public:
	struct DESC : public CComPxJoint::DESC
	{
		// 생성 시 현재 상대 자세를 고정 기준으로 자동 계산한다.
		_bool bPreserveCurrentPose{};
	};

public:
	DECLARE_DERIVED_TYPE(CComPxFixedJoint, CComPxJoint)

private:
	explicit CComPxFixedJoint();
	explicit CComPxFixedJoint(const CComPxFixedJoint& Prototype);
	~CComPxFixedJoint() override;

private:
	HRESULT Initialize(void* pArg) override;

public:
	static UPtr<CComPxFixedJoint> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	void Free() override;
};

NS_END
