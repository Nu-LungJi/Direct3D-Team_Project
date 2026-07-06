#pragma once
#include "ComPxCollider.h"

NS_BEGIN(Engine)
class CResPhysXBoxGeometry;
class ENGINE_DLL CComPxBoxCollider : public CComPxCollider
{
public:
	struct DESC : CComPxCollider::DESC
	{
		SPtr<CResPhysXBoxGeometry> pResBoxGeo{};
	};
public:
	DECLARE_DERIVED_TYPE(CComPxBoxCollider, CComPxCollider)

public:
	void UpdateGUI() override;

private:
	explicit CComPxBoxCollider();
	~CComPxBoxCollider() override;

public:
	HRESULT Initialize(void* pArg) override;

public:
	static UPtr<CComPxBoxCollider> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	SPtr<CResPhysXBoxGeometry> m_pResBoxGeo{};

private:
	void Free() override;
};

NS_END
