#pragma once
#include "ResPhysXGeometry.h"

NS_BEGIN(physx)
class PxBoxGeometry;
NS_END

NS_BEGIN(Engine)

class ENGINE_DLL CResPhysXBoxGeometry final : public CResPhysXGeometry
{
public:
	struct DESC
	{
		XMFLOAT3 vHalfExtents = { 0.5f, 0.5f, 0.5f };
	};
public:
	DECLARE_DERIVED_TYPE(CResPhysXBoxGeometry, CResPhysXGeometry)

private:
	explicit CResPhysXBoxGeometry(const _string& sPath);
	~CResPhysXBoxGeometry() override;

public:
	physx::PxBoxGeometry* GetBoxGeometry() const { return m_pBoxGeometry.get(); }
public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {})  override;

private:
	std::unique_ptr<physx::PxBoxGeometry> m_pBoxGeometry{};

public:
	static SPtr<CResPhysXBoxGeometry> Create();
	static SPtr<CResPhysXBoxGeometry> Create(const DESC& desc);

private:
	void Free() override;
};

NS_END
