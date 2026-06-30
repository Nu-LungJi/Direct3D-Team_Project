#include "pch.h"
#include "ResDynamicBuffer.h"
#include "GameInstance.h"

NS_USING(Engine)

CResDynamicBuffer::CResDynamicBuffer(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
	: CResource {""}
    , m_pDevice { pDevice }
    , m_pContext {pContext}
{
}

CResDynamicBuffer::~CResDynamicBuffer()
{
}

HRESULT CResDynamicBuffer::Load(const std::any& arg)
{
    auto argDesc = std::any_cast<DESC>(&arg);
    if (!argDesc)
    {
        return E_FAIL;

    }
    if (m_eState == STATE::LOADED)
    {
        return S_OK;
    }
    m_eState = STATE::LOADING;


    {
        auto param2 = argDesc->subResource.pSysMem != nullptr ? &argDesc->subResource : nullptr;
        if (FAILED(m_pDevice->CreateBuffer(&argDesc->desc, param2, m_pBuffer.GetAddressOf())))
        {
            return E_FAIL;
        }
    }


    m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResDynamicBuffer::Unload(const std::any& arg)
{
	return S_OK;
}

SPtr<CResDynamicBuffer> CResDynamicBuffer::Create()
{
	return ToSPtr(new CResDynamicBuffer{ CGameInstance::Get().GetGraphicDevice() , CGameInstance::Get().GetGraphicDeviceContext() });
}
