#include "pch.h"
#include "ResVIBuffer.h"

NS_USING(Engine)

CResVIBuffer::CResVIBuffer(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
	: CResource{ sPath }
	, m_pDevice{ pDevice }
	, m_pContext{ pContext }
{
}

CResVIBuffer::~CResVIBuffer()
{
}

HRESULT CResVIBuffer::CreateVertexBuffer(const D3D11_BUFFER_DESC& bufferDesc, const D3D11_SUBRESOURCE_DATA* subResData)
{
    if (FAILED(m_pDevice->CreateBuffer(&bufferDesc, subResData, &m_pVB)))
    {
        return E_FAIL;
    }
    return S_OK;
}

HRESULT CResVIBuffer::CreateIndexBuffer(const D3D11_BUFFER_DESC& bufferDesc, const D3D11_SUBRESOURCE_DATA* subResData)
{
    if (FAILED(m_pDevice->CreateBuffer(&bufferDesc, subResData, &m_pIB)))
    {
        return E_FAIL;
    }
    return S_OK;
}
