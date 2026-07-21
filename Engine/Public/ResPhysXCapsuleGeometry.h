#pragma once
#include "ResPhysXGeometry.h"

NS_BEGIN(physx)
class PxCapsuleGeometry;
NS_END

NS_BEGIN(Engine)

class ENGINE_DLL CResPhysXCapsuleGeometry final : public CResPhysXGeometry
{
public:
	struct DESC
	{
		_float  fRadius = 0.5f;
		_float  fHalfHeight = 0.5f;
	};
public:
	DECLARE_DERIVED_TYPE(CResPhysXCapsuleGeometry, CResPhysXGeometry)

private:
	explicit CResPhysXCapsuleGeometry(const _string& sPath);
	~CResPhysXCapsuleGeometry() override;

public:
	physx::PxCapsuleGeometry* GetCapsuleGeometry() const { return m_pCapsuleGeometry.get(); }
public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {})  override;

private:
	std::unique_ptr<physx::PxCapsuleGeometry> m_pCapsuleGeometry{};

public:
	static SPtr<CResPhysXCapsuleGeometry> Create();
	static SPtr<CResPhysXCapsuleGeometry> CreateAndLoad(const DESC& desc);
};

NS_END
