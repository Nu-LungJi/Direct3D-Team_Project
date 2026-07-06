#pragma once
#include "ResPhysXGeometry.h"

NS_BEGIN(physx)
class PxSphereGeometry;
NS_END

NS_BEGIN(Engine)

class ENGINE_DLL CResPhysXSphereGeometry final : public CResPhysXGeometry
{
public:
	struct DESC
	{
		float fRadius = 0.5f;
	};
public:
	DECLARE_DERIVED_TYPE(CResPhysXSphereGeometry, CResPhysXGeometry)

private:
	explicit CResPhysXSphereGeometry(const _string& sPath);
	~CResPhysXSphereGeometry() override;

public:
	physx::PxSphereGeometry* GetSphereGeometry() const { return m_pSphereGeometry.get(); }
public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {})  override;

private:
	std::unique_ptr<physx::PxSphereGeometry> m_pSphereGeometry{};

public:
	static SPtr<CResPhysXSphereGeometry> Create();
	static SPtr<CResPhysXSphereGeometry> Create(const DESC& desc);
};

NS_END
