#include "pch.h"
#include "LightManager.h"
#include "GameInstance.h"
#include "ComCollider.h"
#include "CollSphere.h"
#include "CollFrustum.h"

CLightManager::CLightManager(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext) : m_pDevice(pDevice), m_pContext(pContext) {}
CLightManager::~CLightManager()	{}

HRESULT CLightManager::Initialize_LightManager(){

    if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "CB_Light", E::CResCBuffer::Create()))
    {
        if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_LIGHT) })))    return E_FAIL;
    }

	return S_OK;
}

VOID CLightManager::UpdateGUI() {
    ImGui::Begin("Light Manager");

    if (ImGui::Button("Generate Light")) {
        Add_PointLight({ 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }, 10.f, 10.f);
    }

    if (m_LightHandleList.empty()) {
        ImGui::End();
        return;
    }

    static int selectedLightIdx = 0;

    if (selectedLightIdx >= static_cast<int>(m_LightHandleList.size()))
        selectedLightIdx = 0;

    ImGui::Text("Light List");
    if (ImGui::BeginListBox("##Lights", ImVec2(-FLT_MIN, 100)))
    {
        int i = 0;
        for (auto iter = m_LightHandleList.begin(); iter != m_LightHandleList.end();)
        {
            auto LightObject = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>(*iter);
            if (nullptr == LightObject) {
                iter = m_LightHandleList.erase(iter);
                continue;
            }
            auto pComCollider = LightObject->GetComponent<CComCollider>("ComCollider_Sphere");
            if (pComCollider)
            {
                auto ColliderType = pComCollider->Get()->GetCollType();

                if      (ColliderType == CollType::Sphere)  {
                    static_cast<CCollSphere*>((pComCollider->Get()))->SetLocalBoundingSphere({}, LightObject->Get_LightRange());
                }
            }
            auto pComCollider_FR = LightObject->GetComponent<CComCollider>("ComCollider_Frustum");
            if (pComCollider_FR)
            {
                auto ColliderType = pComCollider->Get()->GetCollType();

                if (ColliderType == CollType::Frustum) {
                    auto LightPos = LightObject->Get_LightPosition();
                    static_cast<CCollFrustum*>((pComCollider->Get()))->SetLocalFrustum(
                        XMMatrixLookAtLH(XMLoadFloat3(&LightPos),
                            LightObject->GetComponent<CComTransform>("Com_Transform")->GetState(STATE::LOOK),
                            LightObject->GetComponent<CComTransform>("Com_Transform")->GetState(STATE::UP)));
                }
            }
            std::string lightName = "Light" + std::to_string(i);
            LIGHT_TYPE type = LightObject->Get_LightType();
            if      (type == LIGHT_TYPE::DIRECTIONAL)    lightName += " [Directional]";
            else if (type == LIGHT_TYPE::POINT)          lightName += " [Point]";
            else                                         lightName += " [Spot]";

            const bool isSelected = (selectedLightIdx == i);
            if (ImGui::Selectable(lightName.c_str(), isSelected))
            {
                selectedLightIdx = i;
            }

            if (isSelected)
                ImGui::SetItemDefaultFocus();

            i++;
            iter++;
        }
        ImGui::EndListBox();
    }

    ImGui::Separator();

    if (m_LightHandleList.size() == 0) {
        ImGui::End();
        return;
    }
    auto pSelectedLight = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>(m_LightHandleList[selectedLightIdx]);
    ImGui::Text("Selected Light Details (Index: %d)", selectedLightIdx);

    // --- Getter로 현재 값들 가져오기 ---
    LIGHT_TYPE lightType = pSelectedLight->Get_LightType();
    XMFLOAT3 direction = pSelectedLight->Get_LightDirection();
    XMFLOAT3 color = pSelectedLight->Get_LightColor();
    float intensity = pSelectedLight->Get_LightIntensity();
    float range = pSelectedLight->Get_LightRange();
    XMFLOAT3 position = pSelectedLight->Get_LightPosition();
    float innerAttn = pSelectedLight->Get_LightInnerAttenuation();
    float outerAttn = pSelectedLight->Get_LightOuterAttenuation();

    const char* lightTypeNames[] = { "Directional", "Point", "Spot" };
    int currentTypeIdx = static_cast<int>(lightType);
    if (ImGui::Combo("Light Type", &currentTypeIdx, lightTypeNames, IM_ARRAYSIZE(lightTypeNames)))
    {
        pSelectedLight->Set_LightType(static_cast<LIGHT_TYPE>(currentTypeIdx));
    }

    if (ImGui::ColorEdit3("Color", &color.x))
    {
        pSelectedLight->Set_LightColor(color);
    }

    if (ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f, 100.0f, "%.2f"))
    {
        pSelectedLight->Set_LightIntensity(intensity);
    }

    // 타입별 가변 속성 노출
    if (lightType == LIGHT_TYPE::DIRECTIONAL || lightType == LIGHT_TYPE::SPOTLIGHT)
    {
        // 방향 벡터 조절 (DragFloat3)
        if (ImGui::DragFloat3("Direction", &direction.x, 0.01f, -1.0f, 1.0f, "%.2f"))
        {
            pSelectedLight->Set_LightDirection(direction);
        }
    }

    if (lightType == LIGHT_TYPE::POINT || lightType == LIGHT_TYPE::SPOTLIGHT)
    {
        // 위치 조절
        if (ImGui::DragFloat3("Position", &position.x, 0.1f, -100.0f, 100.0f, "%.2f"))
        {
            pSelectedLight->Set_LightPosition(position);
        }
        // 범위 조절
        if (ImGui::DragFloat("Range", &range, 0.1f, 0.0f, 1000.0f, "%.2f"))
        {
            pSelectedLight->Set_LightRange(range);
        }
    }

    if (lightType == LIGHT_TYPE::SPOTLIGHT) {
        if (ImGui::SliderFloat("Inner Attenuation", &innerAttn, 0.0f, 180.0f, "%.1f도"))
        {
            pSelectedLight->Set_LightInnerAttenuation(innerAttn);
        }
        if (ImGui::SliderFloat("Outer Attenuation", &outerAttn, 0.0f, 180.0f, "%.1f도"))
        {
            pSelectedLight->Set_LightOuterAttenuation(outerAttn);
        }
    }

    ImGui::End();
}
VOID CLightManager::Update(_float fTimeDelta){

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

    for (auto& LightHandle : m_LightHandleList) {
        if (LightCount >= MAX_LIGHT_COUNT) break;

        // Need Culling - Frustum & Distance
        auto LightOBJ = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle);

        LightBuffer.AffectedLight[LightCount].LightType         = ETOUI(LightOBJ->Get_LightType());
        LightBuffer.AffectedLight[LightCount].LightDirection    = LightOBJ->Get_LightDirection();
        LightBuffer.AffectedLight[LightCount].LightColor        = LightOBJ->Get_LightColor();
        LightBuffer.AffectedLight[LightCount].LightIntensity    = LightOBJ->Get_LightIntensity();
        LightBuffer.AffectedLight[LightCount].LightRange        = LightOBJ->Get_LightRange();
        LightBuffer.AffectedLight[LightCount].Position          = LightOBJ->Get_LightPosition();
        LightBuffer.AffectedLight[LightCount].InnerAttanuation  = cosf(XMConvertToRadians(LightOBJ->Get_LightInnerAttenuation()));
        LightBuffer.AffectedLight[LightCount].OuterAttanuation  = cosf(XMConvertToRadians(LightOBJ->Get_LightOuterAttenuation()));
    
        LightCount++;
    }
    LightBuffer.g_iLightCount = LightCount;

    auto LightConstantBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_Light");
    D3D11_MAPPED_SUBRESOURCE MRES;
    if (SUCCEEDED(m_pContext->Map(LightConstantBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
    {
        CB_LIGHT   CBL = LightBuffer;
        memcpy(MRES.pData, &CBL, sizeof(CB_LIGHT));
        m_pContext->Unmap(LightConstantBuffer->GetCBuffer().Get(), 0);
    }

    m_pContext->PSSetConstantBuffers(4, 1, LightConstantBuffer->GetCBuffer().GetAddressOf());
}

VOID CLightManager::Add_DirectionalLight(XMFLOAT3 _Direction, XMFLOAT3 _Color, _float _Intensity) {
    CLight::DESC LDesc{};
    if      (m_LightHandleList.size() < 10)     LDesc.sObjectTag = "Light_Clone00"  + m_LightHandleList.size();
    else if (m_LightHandleList.size() < 100)    LDesc.sObjectTag = "Light_Clone0"   + m_LightHandleList.size();
    else if (m_LightHandleList.size() < 1000)   LDesc.sObjectTag = "Light_Clone"    + m_LightHandleList.size();

    auto LightHandle = E::CGameInstance::Get().AddGameObjectToLayer("LIGHT", "Prototype_GameObject_Light", "LightLayer", &LDesc);
    if (!(LightHandle))	return;
    
    auto LightOBJ = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());
    LightOBJ->Set_LightType(LIGHT_TYPE::DIRECTIONAL);
    LightOBJ->Set_LightDirection(_Direction);
    LightOBJ->Set_LightColor(_Color);
    LightOBJ->Set_LightIntensity(_Intensity);

    m_LightHandleList.push_back(LightHandle.value());
}
VOID CLightManager::Add_PointLight(XMFLOAT3 _Position, XMFLOAT3 _Color, _float _Intensity, _float _Range) {
    CLight::DESC LDesc{};
    if      (m_LightHandleList.size() < 10)     LDesc.sObjectTag = "Light_Clone00"  + m_LightHandleList.size();
    else if (m_LightHandleList.size() < 100)    LDesc.sObjectTag = "Light_Clone0"   + m_LightHandleList.size();
    else if (m_LightHandleList.size() < 1000)   LDesc.sObjectTag = "Light_Clone"    + m_LightHandleList.size();

    auto LightHandle = E::CGameInstance::Get().AddGameObjectToLayer("LIGHT", "Prototype_GameObject_Light", "LightLayer", &LDesc);
    if (!(LightHandle))	return;

    auto LightOBJ = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());

    LightOBJ->Set_LightType(LIGHT_TYPE::POINT);

    LightOBJ->Set_LightPosition(_Position);
    LightOBJ->Set_LightColor(_Color);
    LightOBJ->Set_LightIntensity(_Intensity);
    LightOBJ->Set_LightRange(_Range);

    m_LightHandleList.push_back(LightHandle.value());
}
VOID CLightManager::Add_SpotLight(XMFLOAT3 _Position, XMFLOAT3 _Color, _float _Intensity, _float _Range, _float _InnerAtt, _float _OuterAtt) {
    CLight::DESC LDesc{};
    if      (m_LightHandleList.size() < 10)     LDesc.sObjectTag = "Light_Clone00"  + m_LightHandleList.size();
    else if (m_LightHandleList.size() < 100)    LDesc.sObjectTag = "Light_Clone0"   + m_LightHandleList.size();
    else if (m_LightHandleList.size() < 1000)   LDesc.sObjectTag = "Light_Clone"    + m_LightHandleList.size();

    auto LightHandle = E::CGameInstance::Get().AddGameObjectToLayer("LIGHT", "Prototype_GameObject_Light", "LightLayer", &LDesc);
    if (!(LightHandle))	return;

    auto LightOBJ = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());
    LightOBJ->Set_LightType(LIGHT_TYPE::SPOTLIGHT);

    LightOBJ->Set_LightPosition(_Position);
    LightOBJ->Set_LightColor(_Color);
    LightOBJ->Set_LightIntensity(_Intensity);
    LightOBJ->Set_LightRange(_Range);

    LightOBJ->Set_LightInnerAttenuation(_InnerAtt);
    LightOBJ->Set_LightOuterAttenuation(_OuterAtt);

    m_LightHandleList.push_back(LightHandle.value());
}

UPtr<CLightManager> CLightManager::Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext) {
    auto pInstance = ToUPtr(new CLightManager{ pDevice, pContext });
    if (FAILED(pInstance->Initialize_LightManager())) {
        MSG_BOX("Failed to Created : CLightManager");
        return nullptr;
    }
    return pInstance;
}