#pragma once

#include "Engine_Defines.h"
#include "Engine_NvClothDefines.h"

NS_BEGIN(Engine)

class CDbgLineRender;
class CNvClothCollisionEditorGUI;

class ENGINE_DLL CNvClothManager final : public CEngineBase
{
private:
	struct IMPLEMENTATION;

private:
	CNvClothManager();
	~CNvClothManager() override;

	HRESULT Initialize(
		ID3D11Device* pDevice,
		ID3D11DeviceContext* pContext,
		NVCLOTH_BACKEND eBackend);

public:
	_bool IsInitialized() const;
	NVCLOTH_BACKEND GetBackend() const;
	HRESULT CreateFabric(
		const NVCLOTH_FABRIC_DESC& Desc,
		NVCLOTH_FABRIC_HANDLE& OutHandle,
		NVCLOTH_FABRIC_INFO* pOutInfo = nullptr);
	_bool ReleaseFabric(NVCLOTH_FABRIC_HANDLE Handle);
	_bool GetFabricInfo(
		NVCLOTH_FABRIC_HANDLE Handle,
		NVCLOTH_FABRIC_INFO& OutInfo) const;
	HRESULT CreateCloth(
		const NVCLOTH_CLOTH_DESC& Desc,
		NVCLOTH_CLOTH_HANDLE& OutHandle);
	_bool ReleaseCloth(NVCLOTH_CLOTH_HANDLE Handle);
	_bool GetClothParticles(
		NVCLOTH_CLOTH_HANDLE Handle,
		std::vector<_float3>& OutPositions) const;
	_bool GetClothRenderParticleView(
		NVCLOTH_CLOTH_HANDLE Handle,
		NVCLOTH_RENDER_PARTICLE_VIEW& OutView);
	_bool SetClothTransform(
		NVCLOTH_CLOTH_HANDLE Handle,
		const _float3& vTranslation,
		const _float4& vRotation,
		_bool bTeleport);
	_bool SetClothCollisions(
		NVCLOTH_CLOTH_HANDLE Handle,
		const NVCLOTH_COLLISION_DESC& Desc);
	_bool SetClothAnimationConstraints(
		NVCLOTH_CLOTH_HANDLE Handle,
		const NVCLOTH_ANIMATION_CONSTRAINT_DESC& Desc);
	_bool SetClothVirtualParticles(
		NVCLOTH_CLOTH_HANDLE Handle,
		_bool bEnabled);
	void StepSimulation(_float fFixedTimeDelta);
	void RenderDebug(CDbgLineRender& DbgLineRender) const;
	void UpdateGUI();

	static UPtr<CNvClothManager> Create(
		ID3D11Device* pDevice,
		ID3D11DeviceContext* pContext,
		NVCLOTH_BACKEND eBackend =
			NVCLOTH_BACKEND::DX11);

private:
	std::unique_ptr<IMPLEMENTATION> m_pImpl{};
	UPtr<CNvClothCollisionEditorGUI>
		m_pCollisionEditor{};

	void Free() override;
};

NS_END
