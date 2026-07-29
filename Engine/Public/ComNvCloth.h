#pragma once
#include "Component.h"
#include "Engine_NvClothDefines.h"

NS_BEGIN(Engine)

class ENGINE_DLL CComNvCloth final : public CComponent
{
public:
	struct DESC : public CComponent::DESC
	{
		NVCLOTH_FABRIC_DESC tFabric{};

		// hFabric, vecPositions, vecInverseMasses are supplied from
		// tFabric by this component. The remaining fields configure
		// the runtime Cloth instance.
		NVCLOTH_CLOTH_DESC tCloth{};
	};

public:
	DECLARE_DERIVED_TYPE(CComNvCloth, CComponent)

private:
	explicit CComNvCloth();
	explicit CComNvCloth(const CComNvCloth& Prototype);
	~CComNvCloth() override;

public:
	_bool IsValid() const;
	NVCLOTH_FABRIC_HANDLE GetFabricHandle() const;
	NVCLOTH_CLOTH_HANDLE GetClothHandle() const;
	size_t GetParticleCount() const;
	_bool GetParticles(std::vector<_float3>& OutParticles) const;
	_bool GetGpuParticleView(
		NVCLOTH_GPU_PARTICLE_VIEW& OutView) const;
	_bool SetSimulationTransform(
		const _float3& vTranslation,
		const _float4& vRotation,
		_bool bTeleport = false);
	_bool SetCollisions(
		const NVCLOTH_COLLISION_DESC& Desc);
	_bool SetAnimationConstraints(
		const NVCLOTH_ANIMATION_CONSTRAINT_DESC& Desc);

	void UpdateGUI() override;

private:
	HRESULT Initialize(void* pArg) override;
	void ReleaseRuntime();

private:
	NVCLOTH_FABRIC_HANDLE m_hFabric{};
	NVCLOTH_CLOTH_HANDLE m_hCloth{};
	size_t m_iParticleCount{};

public:
	static UPtr<CComNvCloth> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	void Free() override;
};

NS_END
