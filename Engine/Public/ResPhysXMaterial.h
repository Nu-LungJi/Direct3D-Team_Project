#pragma once
#include "Resource.h"

NS_BEGIN(physx)
class PxMaterial;
NS_END

NS_BEGIN(Engine)

class ENGINE_DLL CResPhysXMaterial final : public CResource
{
public:
	struct DESC
	{
		float fStaticFriction = 0.5f;
		float fDynamicFriction = 0.5f;
		float fRestitution = 0.0f;

		//physx::PxCombineMode::Enum 보셈
		uint32_t eFrictionCombine{};
		uint32_t eRestitutionCombine{};
	};
public:
	DECLARE_DERIVED_TYPE(CResPhysXMaterial, CResource)

private:
	explicit CResPhysXMaterial(const _string& sPath);
	~CResPhysXMaterial() override;

public:
	physx::PxMaterial* GetMaterial() const { return m_pMaterial; }

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {})  override;

private:
	physx::PxMaterial* m_pMaterial{};

public:
	static SPtr<CResPhysXMaterial> Create();
	static SPtr<CResPhysXMaterial> CreateAndLoad(const DESC& desc);

private:
	void Free() override;
};

NS_END
