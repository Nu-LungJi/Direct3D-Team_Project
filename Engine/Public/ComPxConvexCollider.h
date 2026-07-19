#pragma once
#include "ComPxCollider.h"

NS_BEGIN(Engine)

class CResPhysXRTConvexGeometry;

class ENGINE_DLL CComPxConvexCollider final : public CComPxCollider
{
public:
	struct DESC : CComPxCollider::DESC
	{
		SPtr<CResPhysXRTConvexGeometry> pResConvex{};
		_float3 vScale{ 1.f, 1.f, 1.f };
	};

public:
	DECLARE_DERIVED_TYPE(CComPxConvexCollider, CComPxCollider)

private:
	CComPxConvexCollider();
	~CComPxConvexCollider() override;

public:
	HRESULT Initialize(void* pArg) override;
	void UpdateGUI() override;

private:
	SPtr<CResPhysXRTConvexGeometry> m_pResConvex{};

public:
	static UPtr<CComPxConvexCollider> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	void Free() override;
};

NS_END
