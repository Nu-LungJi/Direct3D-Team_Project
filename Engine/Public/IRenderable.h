#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)

class ENGINE_DLL IRenderable
{
public:
	DECLARE_RUNTIME_TYPE(IRenderable)

	virtual ~IRenderable() = default;
	virtual HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) = 0;
	virtual HRESULT Render_Shadow(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) { return S_OK; }
	virtual HRESULT Render_ShadowInstanced(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx, const MODEL_INSTANCE_BATCH& batch) { return S_OK; }
	virtual bool HasRenderPass(RENDERPASS ePass) const = 0;

	// 오클루전 컬링 대상인애들만 true // UI/Particle/Light/Skybox 같은 건 기본 false라서 컬링 대상에서 자연스럽게 빠짐
	virtual bool IsOcclusionCullable() const { return false; }
	// 모든 애들한테 BoundingBox 달아주는것보다, 오클루전 컬링 하고싶은 애들한테만 BoundingBox넘겨줌, 필요한 애들은 오버라이딩 할 것.
	virtual bool GetOcclusionBounds(BoundingBox& outBounds) const { return false; }

	virtual bool GetShadowBounds(BoundingBox& _OutBound) const { return GetOcclusionBounds(_OutBound); }
};

NS_END
