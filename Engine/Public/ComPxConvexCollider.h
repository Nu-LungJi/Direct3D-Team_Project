#pragma once
#include "ComPxCollider.h"

NS_BEGIN(Engine)

class CResPhysXConvexGeometry;

class ENGINE_DLL CComPxConvexCollider final : public CComPxCollider
{
public:
	struct DESC : CComPxCollider::DESC
	{
		SPtr<CResPhysXConvexGeometry> pResConvex{};
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
	_bool SetMeshScale(const _float3& vScale);
	_float3 GetMeshScale() const;

private:
	SPtr<CResPhysXConvexGeometry> m_pResConvex{};

public:
	static UPtr<CComPxConvexCollider> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	void Free() override;
};

NS_END
