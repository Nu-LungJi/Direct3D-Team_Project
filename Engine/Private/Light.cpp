#include "pch.h"
#include "Light.h"
#include "GameInstance.h"
#include "Engine_Base.h"
#include "Collider.h"
#include "CollSphere.h"
#include "CollFrustum.h"

CLight::CLight()	: CGameObject{}	{}
CLight::~CLight() {}


void	CLight::UpdateGUI()
{
    CGameObject::UpdateGUI();
}

HRESULT CLight::InitializePrototype(VOID* pArg) {

	
	return S_OK;
}

HRESULT CLight::Initialize(VOID* pArg)
{
    if (FAILED(CGameObject::Initialize(pArg)))			return E_FAIL;
	if (FAILED(Initialize_ShadowMap()))					return E_FAIL;

	m_pColliderSphere  = CCollSphere ::Create(m_pComTransform->GetPosition(), 10.f);
	m_pColliderFrustum = CCollFrustum::Create(XMMatrixIdentity());

    {
        CComConstantBuffer::DESC Desc{};
        Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
        if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject", &Desc, &m_pComCBufferPerObject)))	return E_FAIL;
    }
	
	return S_OK;
}
HRESULT CLight::Initialize_ShadowMap() {
	// 2K Resolution
	uint32_t ShadowMapResolutionX	= { 1280 * 2 };
	uint32_t ShadowMapResolutionY	= { 720  * 2 };

	m_pResDynTexDynamicShadowMap	= CGameInstance::Get().Generate_DepthStencil_RenderTarget("DynTex2D_DynamicShadow", DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_R24_UNORM_X8_TYPELESS, ShadowMapResolutionX, ShadowMapResolutionY);
	if (nullptr == m_pResDynTexDynamicShadowMap)	return E_FAIL;
	
	m_pResDynTexStaticShadowMap		= CGameInstance::Get().Generate_DepthStencil_RenderTarget("DynTex2D_StaticShadow", DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_R24_UNORM_X8_TYPELESS, ShadowMapResolutionX, ShadowMapResolutionY);
	if (nullptr == m_pResDynTexStaticShadowMap)		return E_FAIL;

	return S_OK;
}

VOID CLight::PriorityUpdate(E::_float fTimeDelta) {

}

VOID CLight::Update(E::_float fTimeDelta) {
	m_pComTransform->Update();

	Update_Collider();
}
VOID CLight::LateUpdate(E::_float fTimeDelta) {
}
HRESULT CLight::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) {

	return S_OK;
}

VOID CLight::Update_Collider() {
	XMVECTOR PosVec = XMLoadFloat3(&m_pComTransform->GetPosition());

	if (m_LightType == LIGHT_TYPE::SPOTLIGHT) {
		CGameInstance::Get().AddColliderGroup("Light_Collider", m_pColliderFrustum.get());

		XMVECTOR LookVec = XMVector3Normalize(XMLoadFloat3(&m_fLightDirection));

		XMVECTOR BaseUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
		if (fabsf(XMVectorGetY(XMVector3Dot(LookVec, BaseUp))) > 0.99f) {
			BaseUp = XMVectorSet(0.f, 0.f, 1.f, 0.f);
		}

		XMVECTOR RightVec = XMVector3Normalize(XMVector3Cross(XMVectorSet(0.f, 1.f, 0.f, 0.f), LookVec));
		XMVECTOR UpVec = XMVector3Normalize(XMVector3Cross(XMVector3Normalize(RightVec), LookVec));

		_float fNearZ = 0.01f;
		m_fOuterAttanuation = m_fOuterAttanuation <= 0.f ? 1.f : m_fOuterAttanuation;
		m_fLightRange = m_fLightRange <= fNearZ ? fNearZ + 0.01f : m_fLightRange;

		XMMATRIX LightView = XMMatrixLookAtLH(PosVec, XMVectorAdd(PosVec, LookVec), UpVec);
		XMMATRIX LightProj = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_fOuterAttanuation * 2.f), 1.f, fNearZ, m_fLightRange);

		static_pointer_cast<CCollFrustum>(m_pColliderFrustum)->SetLocalFrustum(LightProj);
		m_pColliderFrustum->Transform(XMMatrixInverse(nullptr, LightView));

		XMStoreFloat4x4(&LightViewProj, XMMatrixMultiply(LightView, LightProj));
	}
	else if (m_LightType == LIGHT_TYPE::POINT) {
		CGameInstance::Get().AddColliderGroup("Light_Collider", m_pColliderSphere.get());
		m_pColliderSphere->Transform(XMMatrixTranslationFromVector(PosVec));
	}

	
}

_bool CLight::Check_ObjectInArea() {

	return true;
}

VOID CLight::Render_StaticShadow(ID3D11DeviceContext* pContext) {
	for (auto& OBJ : m_pRenderable_StaticObjectList) {
		//OBJ->Render_Shadow(pContext);
	}
}
VOID CLight::Render_DynamicShadow(ID3D11DeviceContext* pContext) {
	for (auto& OBJ : m_pRenderable_DynamicObjectList) {
		//OBJ->Render_Shadow(pContext);
	}
}

VOID CLight::Bind_ShadowMapTarget(ID3D11DeviceContext* pContext, _bool _DrawStaticShadow){
	if (_DrawStaticShadow == true) {
		pContext->ClearDepthStencilView(m_pResDynTexStaticShadowMap->GetDSV().Get(), D3D11_CLEAR_DEPTH, 1.f, 0);
		pContext->OMSetRenderTargets(0, nullptr, m_pResDynTexStaticShadowMap->GetDSV().Get());
	}
	else {
		pContext->ClearDepthStencilView(m_pResDynTexDynamicShadowMap->GetDSV().Get(), D3D11_CLEAR_DEPTH, 1.f, 0);
		pContext->OMSetRenderTargets(0, nullptr, m_pResDynTexDynamicShadowMap->GetDSV().Get());
	}
}

UPtr<CLight>	 CLight::Create()
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
