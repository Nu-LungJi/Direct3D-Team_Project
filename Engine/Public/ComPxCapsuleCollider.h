#pragma once
#include "ComPxCollider.h"

NS_BEGIN(Engine)
class CResPhysXCapsuleGeometry;
class ENGINE_DLL CComPxCapsuleCollider : public CComPxCollider
{
public:
	struct DESC : CComPxCollider::DESC
	{
		SPtr<CResPhysXCapsuleGeometry> pResCapsuleGeo{};
	};
public:
	DECLARE_DERIVED_TYPE(CComPxCapsuleCollider, CComPxCollider)

public:
	void UpdateGUI() override;

private:
	explicit CComPxCapsuleCollider();
	~CComPxCapsuleCollider() override;

public:
	HRESULT Initialize(void* pArg) override;

public:
	static UPtr<CComPxCapsuleCollider> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	SPtr<CResPhysXCapsuleGeometry> m_pResCapsuleGeo{};

private:
	void Free() override;
};

NS_END
