#pragma once

#include "Collider.h"


NS_BEGIN(Engine)

class ENGINE_DLL CCollOrientedBox final: public CCollider
{
public:
	DECLARE_DERIVED_TYPE(CCollOrientedBox, CCollider)

private:
	explicit CCollOrientedBox();
	~CCollOrientedBox() override;

public:
	const BoundingOrientedBox& GetBoundingOrientedBox() const { return m_BoundingOrientedBoxWorld; }
	const BoundingOrientedBox& GetLocalBoundingOrientedBox() const { return m_BoundingOrientedBoxLocal; }
	void SetLocalBoundingOrientedBox(const _float3& vCenter, const _float3& vExtents, const _float4& vOrientation) { m_BoundingOrientedBoxLocal = BoundingOrientedBox{ vCenter, vExtents, vOrientation }; }
public:
	 void Transform(_fmatrix wordMatrix) override;
	 _bool Intersect(const CCollider& collider) const override;

private:
	HRESULT Initialize(const _float3& vCenter, const _float3& vExtents, const _float4& vOrientation);

private:
	BoundingOrientedBox m_BoundingOrientedBoxLocal{};
	BoundingOrientedBox m_BoundingOrientedBoxWorld{};

public:
	static UPtr<CCollOrientedBox> Create(const _float3& vCenter, const _float3& vExtents, const _float4& vOrientation);
};

NS_END