#include "pch.h"
#include "ResViewPort.h"
#include "GameInstance.h"

NS_USING(Engine)

CResViewPort::CResViewPort()
	: CResource{ "" }
{
}

CResViewPort::~CResViewPort()
{
}

HRESULT CResViewPort::Load(const std::any& arg)
{
    auto desc = std::any_cast<D3D11_VIEWPORT>(&arg);
    if (!desc)
    {
        return E_FAIL;

    }
    if (m_eState == STATE::LOADED)
    {
        return S_OK;
    }
    m_eState = STATE::LOADING;


    {
        memcpy(&m_ViewPort, desc, sizeof(D3D11_VIEWPORT));
    }
 

    m_eState = STATE::LOADED;
    return S_OK;
}

HRESULT CResViewPort::Unload(const std::any& arg)
{
    return S_OK;
}

SPtr<CResViewPort> CResViewPort::Create()
{
	return ToSPtr(new CResViewPort{});
}
