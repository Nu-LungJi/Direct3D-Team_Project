#pragma once
#include "ComPxCollider.h"

NS_BEGIN(Engine)
class CResPhysXSphereGeometry;
class ENGINE_DLL CComPxSphereCollider : public CComPxCollider
{
public:
	struct DESC : CComPxCollider::DESC
	{
		SPtr<CResPhysXSphereGeometry> pResSphereGeo{};
	};
public:
	DECLARE_DERIVED_TYPE(CComPxSphereCollider, CComPxCollider)

public:
	void UpdateGUI() override;

private:
	explicit CComPxSphereCollider();
	~CComPxSphereCollider() override;

public:
	HRESULT Initialize(void* pArg) override;

public:
	static UPtr<CComPxSphereCollider> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	SPtr<CResPhysXSphereGeometry> m_pResSphereGeo{};

private:
	void Free() override;
};

NS_END
