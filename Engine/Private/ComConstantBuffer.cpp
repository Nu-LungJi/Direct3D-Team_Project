#include "pch.h"
#include "GameInstance.h"
#include "ComConstantBuffer.h"

NS_USING(Engine)

void CComConstantBuffer::UpdateGUI()
{
}

CComConstantBuffer::CComConstantBuffer()
{
}

CComConstantBuffer::~CComConstantBuffer()
{
}

HRESULT CComConstantBuffer::MapDiscard(ID3D11DeviceContext* pContext, void* pData, size_t size)
{
    D3D11_MAPPED_SUBRESOURCE mappedSubResource;
    if (FAILED(pContext->Map(m_pResCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource)))
    {
        return E_FAIL;
    }
    memcpy(mappedSubResource.pData, pData, size);
    pContext->Unmap(m_pResCBuffer->GetCBuffer().Get(), 0);

    return S_OK;
}

HRESULT CComConstantBuffer::Initialize(void* pArg)
{
    if (FAILED(CComponent::Initialize(pArg)))
    {
        return E_FAIL;
    }

    auto* pDesc = static_cast<DESC*>(pArg);
    m_pResCBuffer = CGameInstance::Get().GetResourceFirst<CResCBuffer>(pDesc->cBufferId.first, pDesc->cBufferId.second);
    if (!m_pResCBuffer)
    {
        return E_FAIL;
    }

    return S_OK;
}

UPtr<CComConstantBuffer> CComConstantBuffer::Create()
{
    auto pInstance = ToUPtr(new CComConstantBuffer{});
    if (FAILED(pInstance->InitializePrototype()))
    {
        MSG_BOX("Failed to Created : CComConstantBuffer");
        return nullptr;
    }
    return pInstance;
}

UPtr<CPrototype> CComConstantBuffer::Clone(void* pArg)
{
    auto pInstance = ToUPtr(new CComConstantBuffer{ *this });
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned: CComConstantBuffer");
        return nullptr;
    }
    return pInstance;
}