#include "pch.h"
#include "Light.h"
#include "GameInstance.h"
#include "Engine_Base.h"

CLight::CLight()
    : CGameObject{}
{
}
CLight::~CLight()
{
}

void CLight::UpdateGUI()
{
    CGameObject::UpdateGUI();
    if (ImGui::CollapsingHeader("Light Component", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // 1. Light Type (콤보 박스)
        // ※ LIGHT_TYPE의 실제 enum 값에 맞게 배열을 조정하세요 (예: DIRECTIONAL, POINT, SPOT)
        const char* lightTypes[] = { "Directional", "Point", "Spot" };
        int currentType = static_cast<int>(m_LightType);
        if (ImGui::Combo("Light Type", &currentType, lightTypes, IM_ARRAYSIZE(lightTypes)))
        {
            m_LightType = static_cast<LIGHT_TYPE>(currentType);
        }

        ImGui::Separator(); // 구분선

        // 2. Light Position (위치 - 주로 Point, Spot 구조에서 사용)
        ImGui::DragFloat3("Position", &m_fPosition.x, 0.1f, -1000.0f, 1000.0f, "%.3f");

        // 3. Light Direction (방향 - 주로 Directional, Spot 구조에서 사용)
        if (ImGui::SliderFloat3("Direction", &m_fLightDirection.x, -1.0f, 1.0f, "%.2f"))
        {
            // 방향 벡터는 보통 정규화(Normalize)가 필요합니다. 
            // 필요하다면 여기에 정규화 로직을 추가하세요. (예: m_fLightDirection.Normalize();)
        }

        ImGui::Separator();

        // 4. Light Color (색상 - Color Edit 툴 사용)
        ImGui::ColorEdit3("Light Color", &m_fLightColor.x);

        // 5. Light Intensity (광도/밝기)
        ImGui::DragFloat("Intensity", &m_fLightIntensity, 0.05f, 0.0f, 100.0f, "%.2f");

        // 6. Light Range (범위)
        ImGui::DragFloat("Range", &m_fLightRange, 0.1f, 0.0f, 500.0f, "%.1f");

        ImGui::Separator();

        // 7. Attenuation (감쇄 - 주로 Spot 라이트의 내부/외부 각도나 감쇄율)
        ImGui::DragFloat("Inner Attenuation", &m_fInnerAttanuation, 0.05f, 0.0f, 180.0f, "%.2f");
        ImGui::DragFloat("Outer Attenuation", &m_fOuterAttanuation, 0.05f, 0.0f, 180.0f, "%.2f");
    }
}
HRESULT CLight::InitializePrototype(void* pArg) {
    m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_QuadTex");
    if (FAILED(m_pResVertexShader->Load()))
    {
        return E_FAIL;
    }
    m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_QuadTex");
    if (FAILED(m_pResPixelShader->Load()))
    {
        return E_FAIL;
    }
    m_pResLightTexBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResQuadTexBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_QuadTex");
    if (!m_pResLightTexBuffer)
    {
        return E_FAIL;
    }
    if (auto res = CGameInstance::Get().AddResource("LIGHT", "TEX2D_LightIcon", CResTexture2D::Create("./Resources/Engine/Texture/HDE.png")))
    {
        res->Load();
    }
    m_pResSamplerState = CGameInstance::Get().GetResourceFirst<CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
    if (!m_pResSamplerState)
    {
        return E_FAIL;
    }
}
HRESULT CLight::Initialize(void* pArg)
{
    if (FAILED(CGameObject::Initialize(pArg)))
    {
        return E_FAIL;
    }
    {
        CComConstantBuffer::DESC Desc{};
        Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
        if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject", &Desc, &m_pComCBufferPerObject)))
        {
            return E_FAIL;
        };
    }

    m_pResLightTexture2D = CGameInstance::Get().GetResourceFirst<CResTexture2D>("LIGHT", "TEX2D_LightIcon");
    
	return S_OK;
}

void CLight::PriorityUpdate(E::_float fTimeDelta) {

}
void CLight::Update(E::_float fTimeDelta) {

}
void CLight::LateUpdate(E::_float fTimeDelta) {
    CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
}
HRESULT CLight::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) {
#ifdef _DEBUG
    {
        E::CB_PER_OBJECT cbPerObject{};
        cbPerObject.matWorld = *GetTransform().GetCombinedWorldMatrix();
        XMStoreFloat4x4(&cbPerObject.matWVP, GetTransform().GetLoadedCombinedWorldMatrix() * ctx.matViewProj);
        if (FAILED(m_pComCBufferPerObject->MapDiscard(pContext, &cbPerObject, sizeof(cbPerObject))))            return S_OK;

        pContext->VSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
        pContext->PSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
    }

    pContext->IASetInputLayout(m_pResVertexShader->GetInputLayout().Get());
    pContext->VSSetShader(m_pResVertexShader->GetVertexShader().Get(), nullptr, 0);
    pContext->PSSetShader(m_pResPixelShader->GetPixelShader().Get(), nullptr, 0);

    ID3D11Buffer* vertexBuffers[] = {
        m_pResLightTexBuffer->GetVertexBuffer().Get()
    };
    uint32_t strides[] = {
        m_pResLightTexBuffer->GetVertexStride()
    };
    uint32_t offsets[] = {
        0
    };
    pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
    pContext->IASetIndexBuffer(m_pResLightTexBuffer->GetIndexBuffer().Get(), m_pResLightTexBuffer->GetIndexFormat(), 0);
    pContext->IASetPrimitiveTopology(m_pResLightTexBuffer->GetPrimitiveType());
    {
        pContext->PSSetShaderResources(0, 1, m_pResLightTexture2D->GetSRV().GetAddressOf());
    }
    {
        const auto& sampler = m_pResSamplerState;
        pContext->PSSetSamplers(0, 1, sampler->GetSamplerState().GetAddressOf());
    }
    {
        const auto& rasterizer = E::CGameInstance::GetConst().GetResourceFirst<E::CResRasterizerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_NOCULL);
        pContext->RSSetState(rasterizer->GetRasterizerState().Get());
    }
    pContext->DrawIndexed(m_pResLightTexBuffer->GetNumIndices(), 0, 0);
#endif 

    return S_OK;
}
UPtr<CLight> CLight::Create()
{
    auto pInstance = ToUPtr(new CLight{});
    if (FAILED(pInstance->InitializePrototype(nullptr)))
    {
        MSG_BOX("Failed to Create: CLight");
        return nullptr;
    }

    return pInstance;
}

UPtr<CPrototype> CLight::Clone(void* pArg)
{
    auto pInstance = ToUPtr(new CLight{ *this });
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned: CLight");
        return nullptr;
    }

    return pInstance;
}