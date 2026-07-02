#include "pch.h"
#include "LightManager.h"
#include "GameInstance.h"

CLightManager::CLightManager(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext) : m_pDevice(pDevice), m_pContext(pContext) {}
CLightManager::~CLightManager()	{}

HRESULT CLightManager::Initialize_LightManager(){

	return S_OK;
}

VOID CLightManager::Render_SceneLight(){

    CB_LIGHT LightBuffer{};
    uint32_t LightCount = 0;

    for (auto& Light : m_LightList) {
        if (LightCount >= 8) break;
    
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


    
    //m_pContext->PSSetConstantBuffers(4, 1, &LightBuffer);
}

UPtr<CLightManager> CLightManager::Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext) {
    auto pInstance = ToUPtr(new CLightManager{ pDevice, pContext });
    if (FAILED(pInstance->Initialize_LightManager())) {
        MSG_BOX("Failed to Created : CLightManager");
        return nullptr;
    }
    return pInstance;
}