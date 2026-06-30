#pragma once

#include "ResQuadBuffer.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResQuadTexBuffer final: public CResQuadBuffer
{
public:
	DECLARE_DERIVED_TYPE(CResQuadTexBuffer, CResQuadBuffer)

private:
	explicit CResQuadTexBuffer(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResQuadTexBuffer() override;

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {})  override;

public:
	static SPtr<CResQuadTexBuffer> Create();
};

NS_END