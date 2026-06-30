#pragma once

#include "Collider.h"

NS_BEGIN(Engine)

class ENGINE_DLL CCollFrustum final : public CCollider
{
public:
	DECLARE_DERIVED_TYPE(CCollFrustum, CCollider)

public:
	const BoundingFrustum& GetBoundingFrustum() const { return m_BoundingFrustumWorld; }

private:
	explicit CCollFrustum();
	~CCollFrustum() override;

public:
	 void Transform(_fmatrix wordMatrix) override;
	 _bool Intersect(const CCollider& collider) const override;

private:
	HRESULT Initialize(_fmatrix mat);

private:
	BoundingFrustum m_BoundingFrustumLocal{};
	BoundingFrustum m_BoundingFrustumWorld{};

public:
	static UPtr<CCollFrustum> Create(_fmatrix mat);
};

NS_END