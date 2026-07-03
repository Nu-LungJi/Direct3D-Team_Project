#include "pch.h"
#include "MyGFSDK_SSAO.h"


#include "GameInstance.h"

NS_USING(Engine)
CMyGFSDK_SSAO::CMyGFSDK_SSAO()
{
}
CMyGFSDK_SSAO::~CMyGFSDK_SSAO()
{
}

HRESULT CMyGFSDK_SSAO::Initialize()
{

    /*
INITIALIZE THE LIBRARY:

GFSDK_SSAO_CustomHeap CustomHeap;
CustomHeap.new_ = ::operator new;
CustomHeap.delete_ = ::operator delete;

GFSDK_SSAO_Status status;
GFSDK_SSAO_Context_D3D11* pAOContext;
status = GFSDK_SSAO_CreateContext_D3D11(pD3D11Device, &pAOContext, &CustomHeap);
assert(status == GFSDK_SSAO_OK); // HBAO+ requires feature level 11_0 or above
*/


#ifdef _DEBUG
// 라이브러리 설정 전후로 매크로 잠시 해제
#undef new
#endif

// 라이브러리 힙 설정
    //GFSDK_SSAO_CustomHeap CustomHeap;
    //CustomHeap.new_ = ::operator new;
    //CustomHeap.delete_ = ::operator delete;

    m_GFSDK_SSAO_CustomHeap.new_ = ::operator new;
    m_GFSDK_SSAO_CustomHeap.delete_ = ::operator delete;
#ifdef _DEBUG
#define new DBG_NEW
#endif

    GFSDK_SSAO_Status status;
    //GFSDK_SSAO_Context_D3D11* pAOContext;
    status = GFSDK_SSAO_CreateContext_D3D11(CGameInstance::Get().GetGraphicDevice().Get(), &m_pGFSDK_SSAO_Context, &m_GFSDK_SSAO_CustomHeap);
    assert(status == GFSDK_SSAO_OK); // HBAO+ requires feature level 11_0 or above
    if (status != GFSDK_SSAO_OK)
    {
        return E_FAIL;
    }
    return S_OK;
}

HRESULT CMyGFSDK_SSAO::RenderAO()
{
/*
status = pAOContext->RenderAO(pD3D11Context, Input, Params, Output);
assert(status == GFSDK_SSAO_OK);
*/
    auto status = m_pGFSDK_SSAO_Context->RenderAO(CGameInstance::Get().GetGraphicDeviceContext().Get(), m_GFSDK_SSAO_InputData, m_GFSDK_SSAO_Parameters, m_GFSDK_SSAO_Output);
    if (status != GFSDK_SSAO_OK)
    {
        return E_FAIL;
    }
    return S_OK;
}

UPtr<CMyGFSDK_SSAO> CMyGFSDK_SSAO::Create()
{
    auto pInstance = ToUPtr(new CMyGFSDK_SSAO{ });
    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX("Failed to Created : CMyGFSDK_SSAO");
        return nullptr;
    }
    return pInstance;
}

void CMyGFSDK_SSAO::Free()
{
    if (m_pGFSDK_SSAO_Context) { (m_pGFSDK_SSAO_Context)->Release(); (m_pGFSDK_SSAO_Context) = NULL; }
    CEngineBase::Free();
}
