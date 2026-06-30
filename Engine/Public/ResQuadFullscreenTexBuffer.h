#pragma once

#include "ResQuadBuffer.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResQuadFullscreenTexBuffer final : public CResQuadBuffer
{
public:
	DECLARE_DERIVED_TYPE(CResQuadFullscreenTexBuffer, CResQuadBuffer)

private:
	explicit CResQuadFullscreenTexBuffer(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResQuadFullscreenTexBuffer() override;

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {})  override;

public:
	static SPtr<CResQuadFullscreenTexBuffer> Create();
};

NS_END