#pragma once
#include "Resource.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResVIBuffer: public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResVIBuffer, CResource)

protected:
	explicit CResVIBuffer(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResVIBuffer() override;

public:
	ComPtr<ID3D11Buffer> GetVertexBuffer() const { return m_pVB; }
	ComPtr<ID3D11Buffer> GetIndexBuffer() const { return m_pIB; }
	uint32_t GetNumVertices() const { return m_iNumVertices; }
	uint32_t GetNumIndices() const { return m_iNumIndices; }
	void SetNumVertices(uint32_t iNum) { m_iNumVertices = iNum; }
	void SetNumIndices(uint32_t iNum) { m_iNumIndices = iNum; }
	uint32_t GetVertexStride() const { return m_iVertexStride; }
	uint32_t GetIndexStride() const { return m_iIndexStride; }
	DXGI_FORMAT GetIndexFormat() const { return m_eIndexFormat; }
	D3D11_PRIMITIVE_TOPOLOGY GetPrimitiveType() const { return m_ePrimitiveType; }

protected:
	HRESULT CreateVertexBuffer(const D3D11_BUFFER_DESC& bufferDesc, const D3D11_SUBRESOURCE_DATA* subResData);
	HRESULT CreateIndexBuffer(const D3D11_BUFFER_DESC& bufferDesc, const D3D11_SUBRESOURCE_DATA* subResData);

protected:
	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};

protected:
	ComPtr<ID3D11Buffer> m_pVB{};
	ComPtr<ID3D11Buffer> m_pIB{};

protected:
	uint32_t m_iVertexStride{};
	uint32_t m_iIndexStride{};
	uint32_t m_iNumVertices{};
	uint32_t m_iNumIndices{};
	DXGI_FORMAT m_eIndexFormat{};
	D3D11_PRIMITIVE_TOPOLOGY m_ePrimitiveType = { D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST };
};

NS_END
