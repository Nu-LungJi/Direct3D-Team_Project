#pragma once

#include "ResVIBuffer.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResDynamicVIBuffer final : public CResVIBuffer
{
public:
	DECLARE_DERIVED_TYPE(CResDynamicVIBuffer, CResVIBuffer)

public:
	typedef struct tagDesc
	{
		uint32_t iNumVertices{};
		uint32_t iVertexStride{};
		D3D11_BUFFER_DESC vertexDesc{};
		D3D11_SUBRESOURCE_DATA vertexSubResource{};

		uint32_t iNumIndices{};
		uint32_t iIndexStride{};
		DXGI_FORMAT eIndexFormat{};
		D3D11_BUFFER_DESC IndexDesc{};
		D3D11_SUBRESOURCE_DATA indexSubResource{};

		D3D11_PRIMITIVE_TOPOLOGY ePrimitiveType{ D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST };
	} DESC;

private:
	explicit CResDynamicVIBuffer(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResDynamicVIBuffer() override;

public:

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {})  override;

public:
	static SPtr<CResDynamicVIBuffer> Create();
};

NS_END