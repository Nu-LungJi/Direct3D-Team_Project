#include "pch.h"
#include "LightManager.h"
#include "GameInstance.h"

CLightManager::CLightManager(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext) : m_pDevice(pDevice), m_pContext(pContext) {}
CLightManager::~CLightManager()	{}

HRESULT CLightManager::Initialize_LightManager(){

    if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "CB_Light", E::CResCBuffer::Create()))
    {
        if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_LIGHT) })))    return E_FAIL;
    }

    // MODEL
    if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "CB_PBR", E::CResCBuffer::Create()))
    {
        if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_OBJECT_PBR) })))    return E_FAIL;
    }

	return S_OK;
}

VOID CLightManager::Bind_EnviromentLight(){
    //m_pContext->PSSetShaderResources(4, 1, &m_IrridianceSRV);
    //m_pContext->PSSetShaderResources(5, 1, &m_PreFilterSRV);
    //m_pContext->PSSetShaderResources(6, 1, &m_LUTSRV);
}

VOID CLightManager::Bind_DynamicLight(){
    // 해당 함수(Bind_SceneLight)는 모델의 PBR 픽셀쉐이더를 Draw를 하기전에 CB_LIGHT_BUFFER를 채워주기 위한 용도. 그리기 연산은 수행하지 않음.

    CB_LIGHT LightBuffer{};
    uint32_t LightCount = 0;

    for (auto& Light : m_LightList) {
        if (LightCount >= MAX_LIGHT_COUNT) break;
    
        // Need Culling - Frustum & Distance
    
        LightBuffer.AffectedLight[LightCount].LightType         = ETOUI(Light->Get_LightType());
        LightBuffer.AffectedLight[LightCount].LightDirection    = Light->Get_LightDirection();
        LightBuffer.AffectedLight[LightCount].LightColor        = Light->Get_LightColor();
        LightBuffer.AffectedLight[LightCount].LightIntensity    = Light->Get_LightIntensity();
        LightBuffer.AffectedLight[LightCount].LightRange        = Light->Get_LightRange();
        LightBuffer.AffectedLight[LightCount].Position          = Light->Get_LightPosition();
        LightBuffer.AffectedLight[LightCount].InnerAttanuation  = Light->Get_LightInnerAttenuation();
        LightBuffer.AffectedLight[LightCount].OuterAttanuation  = Light->Get_LightOuterAttenuation();
    
        LightCount++;
    }
    LightBuffer.g_iLightCount = LightCount;

    auto LightConstantBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_Light");
    D3D11_MAPPED_SUBRESOURCE MRES;
    if (SUCCEEDED(m_pContext->Map(LightConstantBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
    {
        CB_LIGHT   CBL;
        CBL = LightBuffer;
        memcpy(MRES.pData, &CBL, sizeof(CB_LIGHT));
        m_pContext->Unmap(LightConstantBuffer->GetCBuffer().Get(), 0);
    }

    m_pContext->PSSetConstantBuffers(4, 1, LightConstantBuffer->GetCBuffer().GetAddressOf());


    // Model Loader
    //{
    //    auto PBRConstantBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_PBR");
    //    D3D11_MAPPED_SUBRESOURCE MRES;
    //    if (SUCCEEDED(m_pContext->Map(PBRConstantBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
    //    {
    //        CB_OBJECT_PBR   CBOPBR;
    //
    //        CBOPBR.AlbedoValue = ;
    //        CBOPBR.RoughnessValue = ;
    //        CBOPBR.MetallicValue = ;
    //
    //        memcpy(MRES.pData, &CBOPBR, sizeof(CB_OBJECT_PBR));
    //        m_pContext->Unmap(PBRConstantBuffer->GetCBuffer().Get(), 0);
    //    }
    //    m_pContext->PSSetConstantBuffers(3, 1, LightConstantBuffer->GetCBuffer().GetAddressOf());
    //
    //    m_pContext->PSSetShaderResources(0, 1, &AlbedoSRV);
    //    m_pContext->PSSetShaderResources(1, 1, &NormalSRV);
    //    m_pContext->PSSetShaderResources(2, 1, &RoughnessSRV);
    //    m_pContext->PSSetShaderResources(3, 1, &MetallicSRV);
    //
    //    // Draw On Model Buffer
    //    m_pContext->DrawIndexed(m_pFullscreenVIBuffer->GetNumIndices(), 0, 0);
    //
    //    ID3D11ShaderResourceView* NullSRV[1] = { nullptr };
    // 
    //    m_pContext->PSSetShaderResources(0, 1, NullSRV);
    //    m_pContext->PSSetShaderResources(1, 1, NullSRV);
    //    m_pContext->PSSetShaderResources(2, 1, NullSRV);
    //    m_pContext->PSSetShaderResources(3, 1, NullSRV);
    //    m_pContext->PSSetShaderResources(4, 1, NullSRV);
    //    m_pContext->PSSetShaderResources(5, 1, NullSRV);
    //    m_pContext->PSSetShaderResources(6, 1, NullSRV);
    //}
}

UPtr<CLightManager> CLightManager::Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext) {
    auto pInstance = ToUPtr(new CLightManager{ pDevice, pContext });
    if (FAILED(pInstance->Initialize_LightManager())) {
        MSG_BOX("Failed to Created : CLightManager");
        return nullptr;
    }
    return pInstance;
}