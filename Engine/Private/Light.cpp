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

	CGameInstance::Get().Generate_CubeMap(m_pStaticShadowDSV.GetAddressOf(), m_pStaticShadowTexture.GetAddressOf(), m_pStaticShadowSRV.GetAddressOf(), 2560, 6);
	CGameInstance::Get().Generate_CubeMap(m_pDynamicShadowDSV.GetAddressOf(), m_pDynamicShadowTexture.GetAddressOf(), m_pDynamicShadowSRV.GetAddressOf(), 2560, 6);

	if (FAILED(CGameInstance::Get().Generate_ShadowMapOutput(m_pFinalShadowUAV.GetAddressOf(), m_pFinalShadowTexture.GetAddressOf(), m_pFinalShadowSRV.GetAddressOf(), 
		DynamicLight.LightType, 2560)))	return E_FAIL;

	return S_OK;
}

VOID CLight::PriorityUpdate(E::_float fTimeDelta) {

}

VOID CLight::Update(E::_float fTimeDelta) {
	m_pComTransform->Update();

	if (DynamicLight.LightType == ETOUI(LIGHT_TYPE::DIRECTIONAL) || DynamicLight.LightType == ETOUI(LIGHT_TYPE::SPOTLIGHT)) {
		_float	 fNearZ = 0.01f;

		DynamicLight.OuterAttanuation = DynamicLight.OuterAttanuation <= 0.f ? 1.f : DynamicLight.OuterAttanuation;
		DynamicLight.LightRange = DynamicLight.LightRange <= fNearZ ? fNearZ + 0.01f : DynamicLight.LightRange;

		XMVECTOR DynamicPos = m_pComTransform->GetState(STATE::POSITION);

		XMStoreFloat4x4(&LightView, XMMatrixLookAtLH(DynamicPos, XMVectorAdd(DynamicPos, XMVector3Normalize(XMLoadFloat3(&DynamicLight.LightDirection))), m_pComTransform->GetState(STATE::UP)));
		
		_float FOVAngle = DynamicLight.OuterAttanuation * 2.f * 1.3f;
		if (FOVAngle > 210.f) FOVAngle = 210.f;

		XMStoreFloat4x4(&LightProj, XMMatrixPerspectiveFovLH(XMConvertToRadians(FOVAngle), 1.f, fNearZ, DynamicLight.LightRange));//DynamicLight.LightRange));
		//XMStoreFloat4x4(&LightProj, XMMatrixOrthographicLH(1280.f * 2.f, 720.f * 2.f, fNearZ, DynamicLight.LightRange));
		XMStoreFloat4x4(&DynamicLight.g_LightViewProj[0], XMMatrixMultiply(XMLoadFloat4x4(&LightView), XMLoadFloat4x4(&LightProj)));
	}
	else {
		_float fNearZ = 0.01f;
		DynamicLight.LightRange = DynamicLight.LightRange <= fNearZ ? fNearZ + 0.01f : DynamicLight.LightRange;

		_float FOVAngle = 90.f;
		XMMATRIX HexaProjMat = XMMatrixPerspectiveFovLH(XMConvertToRadians(FOVAngle), 1.f, fNearZ, DynamicLight.LightRange);

		XMVECTOR PosVec = m_pComTransform->GetLoadedPostion() + XMVectorSet(0.f, 0.0001f, 0.f, 0.f);
		XMVECTOR DirectionVec[6] = {
		XMVectorSet(1.f, 0.f, 0.f, 0.f),   // +X (오른쪽)
		XMVectorSet(-1.f, 0.f, 0.f, 0.f),  // -X (왼쪽)
		XMVectorSet(0.f, 1.f, 0.f, 0.f),   // +Y (위)
		XMVectorSet(0.f, -1.f, 0.f, 0.f),  // -Y (아래)
		XMVectorSet(0.f, 0.f, 1.f, 0.f),   // +Z (앞)
		XMVectorSet(0.f, 0.f, -1.f, 0.f)   // -Z (뒤)
		};

		// DirectX 큐브맵 표준 Up 벡터 정의
		XMVECTOR UpVec[6] = {
			XMVectorSet(0.f, 1.f, 0.f, 0.f),   // +X (Up: +Y)
			XMVectorSet(0.f, 1.f, 0.f, 0.f),   // -X (Up: +Y)
			XMVectorSet(0.f, 0.f, -1.f, 0.f),  // +Y (Up: -Z)
			XMVectorSet(0.f, 0.f, 1.f, 0.f),   // -Y (Up: +Z)
			XMVectorSet(0.f, 1.f, 0.f, 0.f),   // +Z (Up: +Y)
			XMVectorSet(0.f, 1.f, 0.f, 0.f)    // -Z (Up: +Y)
		};

		for (int i = 0; i < MAX_LIGHT_MAPCOUNT; ++i) {
			XMMATRIX ViewMat = XMMatrixLookAtLH(PosVec, XMVectorAdd(PosVec, DirectionVec[i]), UpVec[i]);
			XMStoreFloat4x4(&DynamicLight.g_LightViewProj[i], XMMatrixMultiply(ViewMat, HexaProjMat));
		}
	}
	

	Update_Collider();
}
VOID CLight::LateUpdate(E::_float fTimeDelta) {
}
HRESULT CLight::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) {
	

	return S_OK;
}

void CLight::Update_ObjectConstantBuffer(ID3D11DeviceContext* pContext){
	XMMATRIX LoadedWorldMat = XMLoadFloat4x4(m_pComTransform->GetWorldMatrix());
	XMMATRIX LoadedViewMat = XMLoadFloat4x4(&LightView);
	XMMATRIX LoadedProjMat = XMLoadFloat4x4(&LightProj);
	{
		E::CB_PER_OBJECT cbPerObject{};

		XMMATRIX LoadedWVPMat = XMMatrixMultiply(XMMatrixMultiply(LoadedWorldMat, LoadedViewMat), LoadedProjMat);

		cbPerObject.matWorld = *m_pComTransform->GetWorldMatrix();
		XMStoreFloat4x4(&cbPerObject.matWVP, LoadedWVPMat);
		if (FAILED(m_pComCBufferPerObject->MapDiscard(pContext, &cbPerObject, sizeof(cbPerObject))))	return;

		pContext->VSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	}

	auto pCbPerPass = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_PASS);
	D3D11_MAPPED_SUBRESOURCE mappedSubResource;
	if (SUCCEEDED(pContext->Map(pCbPerPass->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource)))
	{
		CB_PER_PASS cbPerPass{};
		XMStoreFloat4x4(&cbPerPass.matView, LoadedViewMat);
		XMStoreFloat4x4(&cbPerPass.matProj, LoadedProjMat);

		XMStoreFloat4x4(&cbPerPass.matViewProj, XMMatrixMultiply(LoadedViewMat, LoadedProjMat));

		XMStoreFloat4x4(&cbPerPass.matInvView, XMMatrixInverse(nullptr, LoadedViewMat));
		XMStoreFloat4x4(&cbPerPass.matInvProj, XMMatrixInverse(nullptr, LoadedProjMat));

		XMStoreFloat4x4(&cbPerPass.matInvViewProj, XMMatrixMultiply(XMLoadFloat4x4(&cbPerPass.matInvProj), XMLoadFloat4x4(&cbPerPass.matInvView)));

		cbPerPass.vCamPos = m_pComTransform->GetPosition();

		XMStoreFloat4x4(&cbPerPass.matShadowLightViewProj, XMMatrixMultiply(LoadedViewMat, LoadedProjMat));

		memcpy(mappedSubResource.pData, &cbPerPass, sizeof(cbPerPass));
		pContext->Unmap(pCbPerPass->GetCBuffer().Get(), 0);
	}
	pContext->VSSetConstantBuffers(1, 1, pCbPerPass->GetCBuffer().GetAddressOf());
	pContext->PSSetConstantBuffers(1, 1, pCbPerPass->GetCBuffer().GetAddressOf());
	pContext->CSSetConstantBuffers(1, 1, pCbPerPass->GetCBuffer().GetAddressOf());
}

VOID CLight::Update_Collider() {
	XMVECTOR PosVec = XMLoadFloat3(&m_pComTransform->GetPosition());

	if		(DynamicLight.LightType == ETOUI(LIGHT_TYPE::SPOTLIGHT)) {
		CGameInstance::Get().AddColliderGroup("Light_Collider", m_pColliderFrustum.get());

		_float	 fNearZ = 0.01f;
		DynamicLight.OuterAttanuation = DynamicLight.OuterAttanuation <= 0.f ? 1.f : DynamicLight.OuterAttanuation;
		DynamicLight.LightRange = DynamicLight.LightRange <= fNearZ ? fNearZ + 0.01f : DynamicLight.LightRange;

		static_pointer_cast<CCollFrustum>(m_pColliderFrustum)->SetLocalFrustum(XMLoadFloat4x4(&LightProj));

		XMMATRIX InvViewMat = XMMatrixInverse(nullptr, XMLoadFloat4x4(&LightView));

		XMMATRIX CleanWorldMat{};
		XMVECTOR scale, rotation, translation;
		if (XMMatrixDecompose(&scale, &rotation, &translation, InvViewMat)) {
			rotation = XMQuaternionNormalize(rotation);
			CleanWorldMat = XMMatrixRotationQuaternion(rotation) * XMMatrixTranslationFromVector(translation);
		}
		m_pColliderFrustum->Transform(CleanWorldMat);

		//m_pColliderFrustum->Transform(XMMatrixInverse(nullptr, XMLoadFloat4x4(&LightView)));
	}
	else if (DynamicLight.LightType == ETOUI(LIGHT_TYPE::POINT)) {
		CGameInstance::Get().AddColliderGroup("Light_Collider", m_pColliderSphere.get());
		m_pColliderSphere->Transform(XMMatrixTranslationFromVector(PosVec));
	}
}

HRESULT CLight::Capture_ShadowMap(ID3D11DeviceContext* pContext, std::vector<CGameObject*>* _StaticList, std::vector<CGameObject*>* _DynamicList) {
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
		RCTX.matView	 = XMLoadFloat4x4(&LightView);
		RCTX.matProj	 = XMLoadFloat4x4(&LightProj);
		RCTX.matViewProj = RCTX.matView * RCTX.matProj;
	}

	if (_DynamicList) {
		for (auto& GOBJ : *_DynamicList) {
			GOBJ->Render(pContext, RCTX);
		}
	}
	if (_StaticList) {
		for (auto& GOBJ : *_StaticList) {
			GOBJ->Render(pContext, RCTX);
		}
	}

	return S_OK;
}

_bool CLight::Check_ObjectInArea() {

	return true;
}
HRESULT CLight::Change_LightType(ID3D11DeviceContext* pContext, LIGHT_TYPE _LTYPE) {
	if (DynamicLight.LightType == ETOUI(_LTYPE)) return E_FAIL;
	DynamicLight.LightType = ETOUI(_LTYPE);

	if (nullptr != m_pStaticShadowTexture)	{ m_pStaticShadowTexture.Reset();	 }
	if (nullptr != m_pStaticShadowDSV)		{ m_pStaticShadowDSV.Reset();		 }
	if (nullptr != m_pStaticShadowSRV)		{ m_pStaticShadowSRV.Reset();		 }
											  
	if (nullptr != m_pDynamicShadowTexture)	{ m_pDynamicShadowTexture.Reset();	 }
	if (nullptr != m_pDynamicShadowDSV)		{ m_pDynamicShadowDSV.Reset();		 }
	if (nullptr != m_pDynamicShadowSRV)		{ m_pDynamicShadowSRV.Reset();		 }
											  
	if (nullptr != m_pFinalShadowTexture)	{ m_pFinalShadowTexture.Reset();	 }
	if (nullptr != m_pFinalShadowUAV)		{ m_pFinalShadowUAV.Reset();		 }
	if (nullptr != m_pFinalShadowSRV)		{ m_pFinalShadowSRV.Reset();		 }

	if (DynamicLight.LightType == ETOUI(LIGHT_TYPE::POINT)) {
		CGameInstance::Get().Generate_CubeMap(m_pStaticShadowDSV.GetAddressOf(), m_pStaticShadowTexture.GetAddressOf(), m_pStaticShadowSRV.GetAddressOf(), 2560, 6);
		CGameInstance::Get().Generate_CubeMap(m_pDynamicShadowDSV.GetAddressOf(), m_pDynamicShadowTexture.GetAddressOf(), m_pDynamicShadowSRV.GetAddressOf(), 2560, 6);
	}
	else {
		CGameInstance::Get().Generate_ShadowTexture(m_pStaticShadowDSV.GetAddressOf(), m_pStaticShadowTexture.GetAddressOf(), m_pStaticShadowSRV.GetAddressOf(), 2560);
		CGameInstance::Get().Generate_ShadowTexture(m_pDynamicShadowDSV.GetAddressOf(), m_pDynamicShadowTexture.GetAddressOf(), m_pDynamicShadowSRV.GetAddressOf(), 2560);
	}

	if (FAILED(CGameInstance::Get().Generate_ShadowMapOutput(m_pFinalShadowUAV.GetAddressOf(), m_pFinalShadowTexture.GetAddressOf(), m_pFinalShadowSRV.GetAddressOf(), DynamicLight.LightType, 2560)))	return E_FAIL;

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
