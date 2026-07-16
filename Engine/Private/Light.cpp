#include "pch.h"
#include "Light.h"
#include "GameInstance.h"
#include "Engine_Base.h"
#include "Collider.h"
#include "CollSphere.h"
#include "CollFrustum.h"

CLight::CLight() : CGameObject{} {}
CLight::~CLight() {}


void	CLight::UpdateGUI()
{
	CGameObject::UpdateGUI();
}

HRESULT CLight::InitializePrototype(VOID* pArg) {
	DynamicLight = {
		.g_LightViewProj = {},
		.LightDirection = { 1.f, -1.f, 1.f },
		.LightIntensity = { 10.f },
		.LightColor = { 1.f, 1.f, 1.f },
		.LightRange = { 5.f },

		.Position = {0.f, 0.f, 0.f},
		.LightType = ETOUI(LIGHT_TYPE::DIRECTIONAL),

		.InnerAttanuation = { 20.f },
		.OuterAttanuation = { 30.f }
	};

	for (uint32_t i = 0; i < MAX_LIGHT_MAPCOUNT; ++i) 
		XMStoreFloat4x4(&DynamicLight.g_LightViewProj[i], XMMatrixIdentity());
	
	

	return S_OK;
}

HRESULT CLight::Initialize(VOID* pArg)
{
	if (FAILED(CGameObject::Initialize(pArg)))			return E_FAIL;

	m_pColliderSphere = CCollSphere::Create(m_pComTransform->GetPosition(), 10.f);
	m_pColliderFrustum = CCollFrustum::Create(XMMatrixIdentity());

	{
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject", &Desc, &m_pComCBufferPerObject)))	return E_FAIL;
	}

	// 2K Resolution
	uint32_t ShadowMapResolutionX = { 1280 * 2 };
	uint32_t ShadowMapResolutionY = { 720 * 2 };

	CGameInstance::Get().Generate_CubeMap(m_pStaticShadowDSV.GetAddressOf(), m_pStaticShadowTexture.GetAddressOf(), m_pStaticShadowSRV.GetAddressOf(), 1280, 6);
	CGameInstance::Get().Generate_CubeMap(m_pDynamicShadowDSV.GetAddressOf(), m_pDynamicShadowTexture.GetAddressOf(), m_pDynamicShadowSRV.GetAddressOf(), 1280, 6);

	if (FAILED(CGameInstance::Get().Generate_ShadowMapOutput(m_pFinalShadowUAV.GetAddressOf(), m_pFinalShadowTexture.GetAddressOf(), m_pFinalShadowSRV.GetAddressOf(), 
		DynamicLight.LightType, 1280)))	return E_FAIL;

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

	if (DynamicLight.LightType == ETOUI(LIGHT_TYPE::SPOTLIGHT)) {
		CGameInstance::Get().AddColliderGroup("Light_Collider", m_pColliderFrustum.get());

		XMVECTOR LookVec = XMVector3Normalize(XMLoadFloat3(&DynamicLight.LightDirection));

		XMVECTOR BaseUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
		if (fabsf(XMVectorGetY(XMVector3Dot(LookVec, BaseUp))) > 0.99f) {
			BaseUp = XMVectorSet(0.f, 0.f, 1.f, 0.f);
		}

		XMVECTOR RightVec	= XMVector3Normalize(XMVector3Cross(XMVectorSet(0.f, 1.f, 0.f, 0.f), LookVec));
		XMVECTOR UpVec		= XMVector3Cross(RightVec, LookVec);

		_float fNearZ = 0.01f;
		DynamicLight.OuterAttanuation = DynamicLight.OuterAttanuation <= 0.f ? 1.f : DynamicLight.OuterAttanuation;
		DynamicLight.LightRange = DynamicLight.LightRange <= fNearZ ? fNearZ + 0.01f : DynamicLight.LightRange;

		XMStoreFloat4x4(&LightView, XMMatrixLookAtLH(PosVec, XMVectorAdd(PosVec, LookVec), UpVec));
		XMStoreFloat4x4(&LightProj, XMMatrixPerspectiveFovLH(XMConvertToRadians(DynamicLight.OuterAttanuation * 2.f), 1.f, fNearZ, DynamicLight.LightRange));

		static_pointer_cast<CCollFrustum>(m_pColliderFrustum)->SetLocalFrustum(XMLoadFloat4x4(&LightProj));
		m_pColliderFrustum->Transform(XMMatrixInverse(nullptr, XMLoadFloat4x4(&LightView)));

		XMStoreFloat4x4(&LightViewProj, XMMatrixMultiply(XMLoadFloat4x4(&LightView), XMLoadFloat4x4(&LightProj)));
	}
	else if (DynamicLight.LightType == ETOUI(LIGHT_TYPE::POINT)) {
		CGameInstance::Get().AddColliderGroup("Light_Collider", m_pColliderSphere.get());
		m_pColliderSphere->Transform(XMMatrixTranslationFromVector(PosVec));

		_float fNearZ = 0.01f;
		DynamicLight.LightRange = DynamicLight.LightRange <= fNearZ ? fNearZ + 0.01f : DynamicLight.LightRange;
		XMMATRIX HexaProjMat = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.f, fNearZ, DynamicLight.LightRange);

		XMVECTOR DirectionVec[6] = {
			XMVectorSet(1.f, 0.f, 0.f, 0.f),   // +X
			XMVectorSet(-1.f, 0.f, 0.f, 0.f),  // -X
			XMVectorSet(0.f, 1.f, 0.f, 0.f),   // +Y
			XMVectorSet(0.f, -1.f, 0.f, 0.f),  // -Y
			XMVectorSet(0.f, 0.f, 1.f, 0.f),   // +Z
			XMVectorSet(0.f, 0.f, -1.f, 0.f)   // -Z
		};

		XMVECTOR UpVec[6] = {
			XMVectorSet(0.f, 1.f, 0.f, 0.f),   // +X (Up)
			XMVectorSet(0.f, 1.f, 0.f, 0.f),   // -X (Up)
			XMVectorSet(0.f, 0.f, -1.f, 0.f),  // +Y (+Y축 Up벡터)
			XMVectorSet(0.f, 0.f, 1.f, 0.f),   // -Y (-Y축 Up벡터)
			XMVectorSet(0.f, 1.f, 0.f, 0.f),   // +Z (Up)
			XMVectorSet(0.f, 1.f, 0.f, 0.f)    // -Z (Up)
		};

		for (int i = 0; i < MAX_LIGHT_MAPCOUNT; ++i) {
			XMMATRIX ViewMat = XMMatrixLookAtLH(PosVec, XMVectorAdd(PosVec, DirectionVec[i]), UpVec[i]);
			XMStoreFloat4x4(&DynamicLight.g_LightViewProj[i], XMMatrixMultiply(ViewMat, HexaProjMat));
		}
	}
}

HRESULT CLight::Capture_ShadowMap(ID3D11DeviceContext* pContext) {
	RENDER_CTX RCTX{};
	RCTX.pass = RENDERPASS::SHADOW;
	RCTX.eye = XMLoadFloat3(&m_pComTransform->GetPosition());
	if (DynamicLight.LightType == ETOUI(LIGHT_TYPE::POINT)) {
		XMMATRIX Identity = XMMatrixIdentity();
		RCTX.matView = Identity;
		RCTX.matProj = Identity;
		RCTX.matViewProj = Identity;
	}
	else {
		RCTX.matView = XMLoadFloat4x4(&LightView);
		RCTX.matProj = XMLoadFloat4x4(&LightProj);
		RCTX.matViewProj = XMLoadFloat4x4(&LightViewProj);
	}

	for (auto& GOBJ : m_pRenderable_DynamicObjectList) {
		GOBJ->Render(pContext, RCTX);
	}
	for (auto& GOBJ : m_pRenderable_StaticObjectList) {
		GOBJ->Render(pContext, RCTX);
	}

	return S_OK;
}

_bool CLight::Check_ObjectInArea() {

	return true;
}
HRESULT CLight::Change_LightType(ID3D11DeviceContext* pContext, LIGHT_TYPE _LTYPE) {
	if (DynamicLight.LightType == ETOUI(_LTYPE)) return E_FAIL;
	DynamicLight.LightType = ETOUI(_LTYPE);

	Safe_Release(m_pStaticShadowTexture);	Safe_Release(m_pStaticShadowDSV);	Safe_Release(m_pStaticShadowSRV);
	Safe_Release(m_pDynamicShadowTexture);	Safe_Release(m_pDynamicShadowDSV);	Safe_Release(m_pDynamicShadowSRV);
	Safe_Release(m_pFinalShadowTexture);	Safe_Release(m_pFinalShadowUAV);	Safe_Release(m_pFinalShadowSRV);

	if (DynamicLight.LightType == ETOUI(LIGHT_TYPE::POINT)) {
		CGameInstance::Get().Generate_CubeMap(m_pStaticShadowDSV.GetAddressOf(), m_pStaticShadowTexture.GetAddressOf(), m_pStaticShadowSRV.GetAddressOf(), 1280, 6);
		CGameInstance::Get().Generate_CubeMap(m_pDynamicShadowDSV.GetAddressOf(), m_pDynamicShadowTexture.GetAddressOf(), m_pDynamicShadowSRV.GetAddressOf(), 1280, 6);
	}
	else {
		CGameInstance::Get().Generate_ShadowTexture(m_pStaticShadowDSV.GetAddressOf(), m_pStaticShadowTexture.GetAddressOf(), m_pStaticShadowSRV.GetAddressOf(), 1280);
		CGameInstance::Get().Generate_ShadowTexture(m_pDynamicShadowDSV.GetAddressOf(), m_pDynamicShadowTexture.GetAddressOf(), m_pDynamicShadowSRV.GetAddressOf(), 1280);
	}

	if (FAILED(CGameInstance::Get().Generate_ShadowMapOutput(m_pFinalShadowUAV.GetAddressOf(), m_pFinalShadowTexture.GetAddressOf(), m_pFinalShadowSRV.GetAddressOf(), DynamicLight.LightType, 1280)))	return E_FAIL;

	DirtyFlag = true;
	   
	return S_OK;
}

VOID CLight::Add_ShadowRenderGroup(ACTORTYPE _ATYPE, CGameObject* pRenderObject) {
	if (_ATYPE == ACTORTYPE::DYNAMIC) {
		m_pRenderable_DynamicObjectList.push_back(pRenderObject);
	}
	else {
		m_pRenderable_StaticObjectList.push_back(pRenderObject);
	}
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

VOID CLight::Bind_ShadowMapTarget(ID3D11DeviceContext* pContext, _bool _DrawStaticShadow) {
	//if (_DrawStaticShadow == true) {
	//	pContext->ClearDepthStencilView(m_pResDynTexStaticShadowMap->GetDSV().Get(), D3D11_CLEAR_DEPTH, 1.f, 0);
	//	pContext->OMSetRenderTargets(0, nullptr, m_pResDynTexStaticShadowMap->GetDSV().Get());
	//}
	//else {
	//	pContext->ClearDepthStencilView(m_pResDynTexDynamicShadowMap->GetDSV().Get(), D3D11_CLEAR_DEPTH, 1.f, 0);
	//	pContext->OMSetRenderTargets(0, nullptr, m_pResDynTexDynamicShadowMap->GetDSV().Get());
	//}
}

UPtr<CLight>	 CLight::Create()
{
	auto pInstance = ToUPtr(new CLight{});
	if (FAILED(pInstance->InitializePrototype(nullptr))) {
		MSG_BOX("Failed to Create: CLight");
		return nullptr;
	}

	return pInstance;
}
UPtr<CPrototype> CLight::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CLight{ *this });
	if (FAILED(pInstance->Initialize(pArg))) {
		MSG_BOX("Failed to Cloned: CLight");
		return nullptr;
	}

	return pInstance;
}
