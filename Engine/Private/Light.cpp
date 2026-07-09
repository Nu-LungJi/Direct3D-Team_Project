#include "pch.h"
#include "Light.h"
#include "GameInstance.h"
#include "Engine_Base.h"
#include "Collider.h"
#include "CollSphere.h"

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
}

HRESULT CLight::InitializePrototype(void* pArg) {
    m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_QuadTex");
    if (FAILED(m_pResVertexShader->Load(CResShader::DESC{ .sEntryPoint = "VSMain", .sTarget = "vs_5_0" })))
    {
        return E_FAIL;
    }
    m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_QuadTex");
    if (FAILED(m_pResPixelShader->Load(CResShader::DESC{ .sEntryPoint = "PSMain", .sTarget = "ps_5_0" })))
    {
        return E_FAIL;
    }
    m_pResLightTexBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResQuadTexBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_QuadTex");
    if (!m_pResLightTexBuffer)
    {
        return E_FAIL;
    }
    m_pResSamplerState = CGameInstance::Get().GetResourceFirst<CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
    if (!m_pResSamplerState)
    {
        return E_FAIL;
    }
    
    if (auto res = CGameInstance::Get().AddResource("LIGHT", "TEX2D_Icon_DirectionalLight", CResTexture2D::Create("./Resources/Engine/Texture/Debugging/Icon_DirectionalLight.png"))) {
        res->Load();
    }
    if (auto res = CGameInstance::Get().AddResource("LIGHT", "TEX2D_Icon_PointLight", CResTexture2D::Create("./Resources/Engine/Texture/Debugging/Icon_PointLight.png"))){
        res->Load();
    }
    if (auto res = CGameInstance::Get().AddResource("LIGHT", "TEX2D_Icon_SpotLight", CResTexture2D::Create("./Resources/Engine/Texture/Debugging/Icon_SpotLight.png"))) {
        res->Load();
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
        if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject", &Desc, &m_pComCBufferPerObject))) return E_FAIL;

    }
    {
        CComCollider::DESC Desc{};
        Desc.eCollType = CollType::Sphere;
        if (FAILED(AddComponentFromProto("COLLIDER", "Prototype_Component_Collider", "ComCollider_Sphere", &Desc, &m_pComColliderSphere)))  return E_FAIL;
    }
    {
        CComCollider::DESC Desc{};
        Desc.eCollType = CollType::Frustum;
        if (FAILED(AddComponentFromProto("COLLIDER", "Prototype_Component_Collider", "ComCollider_Frustum", &Desc, &m_pComColliderFrustum)))  return E_FAIL;
    }

	// DEBUG : Light Center Icon
    m_pResDirectionalLightTexture2D = CGameInstance::Get().GetResourceFirst<CResTexture2D>("LIGHT", "TEX2D_Icon_DirectionalLight");
    m_pResPointLightTexture2D       = CGameInstance::Get().GetResourceFirst<CResTexture2D>("LIGHT", "TEX2D_Icon_PointLight");
    m_pResSpotLightTexture2D        = CGameInstance::Get().GetResourceFirst<CResTexture2D>("LIGHT", "TEX2D_Icon_SpotLight");
    
	return S_OK;
}

void CLight::PriorityUpdate(E::_float fTimeDelta) {

}
void CLight::Update(E::_float fTimeDelta) {
    m_pComTransform->Update();     
    auto CurrentCamera = CGameInstance::Get().GetActiveCamera();
    if (nullptr == CurrentCamera) return;

    // DEBUG : Render By Distance
    if (XMVectorGetX(XMVector3Length(CurrentCamera->GetTransform().GetLoadedPostion() - m_pComTransform->GetLoadedPostion())) > 20.f) {
        Debug_RenderFlag = false;
        return;
    }
    else {
        Debug_RenderFlag = true;
    }
    XMVECTOR LightPosition = m_pComTransform->GetLoadedPostion();
    // DEBUG : Light Range Line
    if (m_LightType == LIGHT_TYPE::SPOTLIGHT) {
        CGameInstance::Get().AddColliderGroup("Collider_DEBUG", m_pComColliderFrustum->Get());
        XMMatrixLookAtLH(LightPosition, m_pComTransform->GetState(STATE::LOOK), m_pComTransform->GetState(STATE::UP));
        m_pComColliderFrustum->Get()->Transform(XMMatrixLookAtLH(LightPosition, m_pComTransform->GetState(STATE::LOOK), m_pComTransform->GetState(STATE::UP)));
    }
    else if (m_LightType == LIGHT_TYPE::POINT){
        CGameInstance::Get().AddColliderGroup("Collider_DEBUG", m_pComColliderSphere->Get());
        XMFLOAT3 Position = m_pComTransform->GetPosition();
        m_pComColliderSphere->Get()->Transform(XMMatrixTranslation(Position.x, Position.y, Position.z));
    }

    // DEBUG : Debug Icon BillBoard
    XMVECTOR determinant;
    XMMATRIX CameraInvViewMat = XMMatrixInverse(&determinant, CurrentCamera->GetView());

    CameraInvViewMat.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    XMMATRIX CameraTransMat = XMMatrixTranslationFromVector(XMLoadFloat3(&m_pComTransform->GetPosition()));
    XMMATRIX BBDMat = CameraInvViewMat * CameraTransMat;

	m_pComTransform->SetQuaternion(XMQuaternionRotationMatrix({
		XMVectorSetW(BBDMat.r[0], 0.f), // RIGHT
		XMVectorSetW(BBDMat.r[1], 0.f), // UP
		XMVectorSetW(BBDMat.r[2], 0.f), // LOOK
		XMVectorSet(0.f, 0.f, 0.f, 1.f)
		}));
	m_pComTransform->SetPosition(XMVectorSetW(BBDMat.r[3], 1.f));

}
void CLight::LateUpdate(E::_float fTimeDelta) {    
    if (Debug_RenderFlag)
        CGameInstance::Get().AddRenderObject(RENDERGROUP::BLEND, this);    
}
HRESULT CLight::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) {
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

    {   // DEBUG : Light Position Icon
        if      (m_LightType == LIGHT_TYPE::DIRECTIONAL) {
            pContext->PSSetShaderResources(0, 1, m_pResDirectionalLightTexture2D->GetSRV().GetAddressOf());
        }
        else if (m_LightType == LIGHT_TYPE::POINT) {
            pContext->PSSetShaderResources(0, 1, m_pResPointLightTexture2D->GetSRV().GetAddressOf());
        }
        else if (m_LightType == LIGHT_TYPE::SPOTLIGHT) {
            pContext->PSSetShaderResources(0, 1, m_pResSpotLightTexture2D->GetSRV().GetAddressOf());
        }
    }
    ID3D11ShaderResourceView* pSRVs[1] = { nullptr };
    pContext->PSSetShaderResources(1, 1, pSRVs);
    pContext->PSSetShaderResources(2, 1, pSRVs);
    pContext->PSSetShaderResources(3, 1, pSRVs);

    pContext->DrawIndexed(m_pResLightTexBuffer->GetNumIndices(), 0, 0);

    pContext->VSSetShader(nullptr, nullptr, 0);
    pContext->PSSetShader(nullptr, nullptr, 0);

    ID3D11ShaderResourceView* pNullSRVs[1] = { nullptr };
    pContext->PSSetShaderResources(0, 1, pSRVs);
    pContext->PSSetShaderResources(1, 1, pSRVs);
    pContext->PSSetShaderResources(2, 1, pSRVs);
    pContext->PSSetShaderResources(3, 1, pSRVs);

    return S_OK;
}

VOID CLight::Set_LightType(LIGHT_TYPE _LTYPE) {
    m_LightType = _LTYPE;
    // 내일 구현 - 타입 바뀌면 콜라이더도 
}
UPtr<CLight> CLight::Create()
{
    auto pInstance = ToUPtr(new CLight{});
    if (FAILED(pInstance->InitializePrototype(nullptr)))    {
        MSG_BOX("Failed to Create: CLight");
        return nullptr;
    }

    return pInstance;
}
UPtr<CPrototype> CLight::Clone(void* pArg)
{
    auto pInstance = ToUPtr(new CLight{ *this });
    if (FAILED(pInstance->Initialize(pArg)))    {
        MSG_BOX("Failed to Cloned: CLight");
        return nullptr;
    }

    return pInstance;
}
