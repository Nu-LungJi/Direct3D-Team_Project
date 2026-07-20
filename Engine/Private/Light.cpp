#include "pch.h"
#include "Light.h"
#include "GameInstance.h"
#include "Engine_Base.h"
#include "Collider.h"
#include "CollSphere.h"
#include "CollFrustum.h"

CLight::CLight() : CGameObject{} {}
CLight::~CLight() {}

VOID	CLight::UpdateGUI() {
	CGameObject::UpdateGUI();
}

HRESULT CLight::InitializePrototype(VOID* pArg) {
	DynamicLight.LightDirection = { 1.f, -1.f, 1.f };
	DynamicLight.LightIntensity = { 10.f };
	DynamicLight.LightColor = { 1.f, 1.f, 1.f };
	DynamicLight.LightRange = { 100.f };

	DynamicLight.Position = { 0.f, 0.f, 0.f };
	DynamicLight.LightType = ETOUI(LIGHT_TYPE::DIRECTIONAL);
		
	DynamicLight.InnerAttanuation = { 20.f };
	DynamicLight.OuterAttanuation = { 30.f };


	for (uint32_t i = 0; i < MAX_LIGHT_MAPCOUNT; ++i)
		XMStoreFloat4x4(&DynamicLight.g_LightViewProj[i], XMMatrixIdentity());
	
	// CubeMap 그림자 촬영 View의 Look벡터
	DirectionVec[0] = { XMVectorSet(+1.f, 0.f, 0.f, 0.f) }; // +X (오른쪽)
	DirectionVec[1] = { XMVectorSet(-1.f, 0.f, 0.f, 0.f) }; // -X (왼쪽)
	DirectionVec[2] = { XMVectorSet(0.f, +1.f, 0.f, 0.f) }; // +Y (위)
	DirectionVec[3] = { XMVectorSet(0.f, -1.f, 0.f, 0.f) }; // -Y (아래)
	DirectionVec[4] = { XMVectorSet(0.f, 0.f, +1.f, 0.f) }; // +Z (앞)
	DirectionVec[5] = { XMVectorSet(0.f, 0.f, -1.f, 0.f) }; // -Z (뒤)

	// CubeMap 그림자 촬영 View의 Up벡터
	BaseUpVec[0] = { XMVectorSet(0.f, +1.f, 0.f, 0.f) }; // +X (Up: +Y)
	BaseUpVec[1] = { XMVectorSet(0.f, +1.f, 0.f, 0.f) }; // -X (Up: +Y)
	BaseUpVec[2] = { XMVectorSet(0.f, 0.f, -1.f, 0.f) }; // +Y (Up: -Z)
	BaseUpVec[3] = { XMVectorSet(0.f, 0.f, +1.f, 0.f) }; // -Y (Up: +Z)
	BaseUpVec[4] = { XMVectorSet(0.f, +1.f, 0.f, 0.f) }; // +Z (Up: +Y)
	BaseUpVec[5] = { XMVectorSet(0.f, +1.f, 0.f, 0.f) }; // -Z (Up: +Y)

	return S_OK;
}

HRESULT CLight::Initialize(VOID* pArg)
{
	if (FAILED(CGameObject::Initialize(pArg)))			return E_FAIL;

	m_pColliderSphere	= CCollSphere::Create(m_pComTransform->GetPosition(), 10.f);
	m_pColliderFrustum	= CCollFrustum::Create(XMMatrixIdentity());

	{
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject", &Desc, &m_pComCBufferPerObject)))	return E_FAIL;
	}

	CGameInstance::Get().Generate_CubeMap(m_pStaticShadowDSV.GetAddressOf(), m_pStaticShadowTexture.GetAddressOf(), m_pStaticShadowSRV.GetAddressOf(), 1024, 6);
	CGameInstance::Get().Generate_CubeMap(m_pDynamicShadowDSV.GetAddressOf(), m_pDynamicShadowTexture.GetAddressOf(), m_pDynamicShadowSRV.GetAddressOf(), 1024, 6);

	_float fNearZ = 0.01f;

	if (DynamicLight.LightRange <= fNearZ) DynamicLight.LightRange = fNearZ + 0.01f;
	ShadowMapProj_PointLight = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.f, fNearZ, DynamicLight.LightRange);

	return S_OK;
}

VOID CLight::PriorityUpdate(E::_float fTimeDelta) {

}

VOID CLight::Update(E::_float fTimeDelta) {
	m_pComTransform->Update();

	if (DynamicLight.LightType == ETOUI(LIGHT_TYPE::DIRECTIONAL) || DynamicLight.LightType == ETOUI(LIGHT_TYPE::SPOTLIGHT)) {
		_float	 fNearZ = 0.01f;

		DynamicLight.OuterAttanuation = DynamicLight.OuterAttanuation <= 0.f ? 1.f : (DynamicLight.OuterAttanuation >= 75.f ? 75.f : DynamicLight.OuterAttanuation);
		DynamicLight.LightRange = DynamicLight.LightRange <= fNearZ ? fNearZ + 0.01f : DynamicLight.LightRange;

		XMVECTOR DynamicPos			= m_pComTransform->GetState(STATE::POSITION);
		XMVECTOR DynamicDirection	= XMLoadFloat3(&DynamicLight.LightDirection);
		XMVECTOR WorldUpVec			= XMVectorSet(0.f, 1.f, 0.f, 0.f);

		_float CameraOffset = 0.2f;
		XMVECTOR ShadowCameraPos = XMVectorSubtract(DynamicPos, XMVectorScale(DynamicDirection, CameraOffset));
		XMStoreFloat4x4(&LightView, XMMatrixLookAtLH(ShadowCameraPos, XMVectorAdd(DynamicPos, XMVector3Normalize(DynamicDirection)), WorldUpVec));
		
		_float FOVAngle = DynamicLight.OuterAttanuation * 2.f * 1.2f;
		if (FOVAngle > 150.f) FOVAngle = 150.f;

		XMStoreFloat4x4(&LightProj, XMMatrixPerspectiveFovLH(XMConvertToRadians(FOVAngle), 1280.f / 720.f, fNearZ, DynamicLight.LightRange + CameraOffset));//DynamicLight.LightRange));
		//XMStoreFloat4x4(&LightProj, XMMatrixOrthographicLH(1280.f * 2.f, 720.f * 2.f, fNearZ, DynamicLight.LightRange));
		XMStoreFloat4x4(&DynamicLight.g_LightViewProj[0], XMMatrixMultiply(XMLoadFloat4x4(&LightView), XMLoadFloat4x4(&LightProj)));
	}
	else {
		XMVECTOR PosVec = m_pComTransform->GetLoadedPostion() + XMVectorSet(0.f, 0.0001f, 0.f, 0.f);

		for (int i = 0; i < MAX_LIGHT_MAPCOUNT; ++i) {
			XMMATRIX ViewMat = XMMatrixLookAtLH(PosVec, XMVectorAdd(PosVec, DirectionVec[i]), BaseUpVec[i]);
			XMStoreFloat4x4(&DynamicLight.g_LightViewProj[i], XMMatrixMultiply(ViewMat, ShadowMapProj_PointLight));
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
	XMMATRIX LoadedViewMat, LoadedProjMat; XMMATRIX PassViewMat, PassProjMat;
	if (DynamicLight.LightType == ETOUI(LIGHT_TYPE::POINT))
	{
		LoadedViewMat = XMMatrixIdentity();
		LoadedProjMat = XMMatrixIdentity();
		PassViewMat = XMMatrixLookAtLH(
			m_pComTransform->GetLoadedPostion() + XMVectorSet(0.f, 0.0001f, 0.f, 0.f),
			m_pComTransform->GetLoadedPostion() + XMVectorSet(0.f, 0.0001f, 0.f, 0.f) + XMVectorSet(1.f, 0.f, 0.f, 0.f),
			XMVectorSet(0.f, 1.f, 0.f, 0.f)
		);

		_float fNearZ = 0.01f, CameraOffset = 0.2f;
		PassProjMat = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.f, fNearZ, DynamicLight.LightRange);
	}
	else
	{
		LoadedViewMat = XMLoadFloat4x4(&LightView);
		LoadedProjMat = XMLoadFloat4x4(&LightProj);

		PassViewMat = LoadedViewMat;
		PassProjMat = LoadedProjMat;
	}
	{
		E::CB_PER_OBJECT cbPerObject{};

		XMMATRIX LoadedWVPMat = XMMatrixMultiply(XMMatrixMultiply(LoadedWorldMat, LoadedViewMat), LoadedProjMat);

		cbPerObject.matWorld = *m_pComTransform->GetWorldMatrix();
		XMStoreFloat4x4(&cbPerObject.matWVP, LoadedWVPMat);
		if (FAILED(m_pComCBufferPerObject->MapDiscard(pContext, &cbPerObject, sizeof(cbPerObject))))	return;

		pContext->VSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
		pContext->GSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
		pContext->CSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	}

	auto pCbPerPass = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_PASS);
	D3D11_MAPPED_SUBRESOURCE mappedSubResource;
	if (SUCCEEDED(pContext->Map(pCbPerPass->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource)))
	{
		CB_PER_PASS cbPerPass{};
		XMStoreFloat4x4(&cbPerPass.matView, PassViewMat);
		XMStoreFloat4x4(&cbPerPass.matProj, PassProjMat);
		XMStoreFloat4x4(&cbPerPass.matViewProj, XMMatrixMultiply(PassViewMat, PassProjMat));
		XMStoreFloat4x4(&cbPerPass.matInvView , XMMatrixInverse(nullptr, PassViewMat));
		XMStoreFloat4x4(&cbPerPass.matInvProj , XMMatrixInverse(nullptr, PassProjMat));
		XMStoreFloat4x4(&cbPerPass.matInvViewProj, XMMatrixMultiply(XMMatrixInverse(nullptr, PassProjMat), XMMatrixInverse(nullptr, PassViewMat)));
		XMStoreFloat4x4(&cbPerPass.matShadowLightViewProj, XMMatrixMultiply(PassViewMat, PassProjMat));

		cbPerPass.vCamPos = m_pComTransform->GetPosition();

		memcpy(mappedSubResource.pData, &cbPerPass, sizeof(cbPerPass));
		pContext->Unmap(pCbPerPass->GetCBuffer().Get(), 0);
	}
	{
		pContext->VSSetConstantBuffers(1, 1, pCbPerPass->GetCBuffer().GetAddressOf());
		pContext->PSSetConstantBuffers(1, 1, pCbPerPass->GetCBuffer().GetAddressOf());
		pContext->GSSetConstantBuffers(1, 1, pCbPerPass->GetCBuffer().GetAddressOf());
		pContext->CSSetConstantBuffers(1, 1, pCbPerPass->GetCBuffer().GetAddressOf());
	}
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

		XMMATRIX WorldMatrix{};
		XMVECTOR scale{}, rotation{}, translation{};
		if (XMMatrixDecompose(&scale, &rotation, &translation, InvViewMat)) {
			rotation = XMQuaternionNormalize(rotation);
			WorldMatrix = XMMatrixRotationQuaternion(rotation) * XMMatrixTranslationFromVector(translation);
		}
		m_pColliderFrustum->Transform(WorldMatrix);
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

		//for (uint32_t IDX = 0; IDX < MAX_LIGHT_MAPCOUNT; ++IDX) {
		//	RCTX.matViewProj = XMLoadFloat4x4(&DynamicLight.g_LightViewProj[IDX]);

		if (_DynamicList) { for (auto& GOBJ : *_DynamicList) GOBJ->Render(pContext, RCTX); }
		if (_StaticList)  { for (auto& GOBJ : *_StaticList)  GOBJ->Render(pContext, RCTX); }
		//}
	}
	else {
		RCTX.matView = XMLoadFloat4x4(&LightView);
		RCTX.matProj = XMLoadFloat4x4(&LightProj);
		RCTX.matViewProj = RCTX.matView * RCTX.matProj;

		if (_DynamicList) { for (auto& GOBJ : *_DynamicList) GOBJ->Render(pContext, RCTX); }
		if (_StaticList)  { for (auto& GOBJ : *_StaticList)  GOBJ->Render(pContext, RCTX); }
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
		CGameInstance::Get().Generate_CubeMap(m_pStaticShadowDSV.GetAddressOf(), m_pStaticShadowTexture.GetAddressOf(), m_pStaticShadowSRV.GetAddressOf(), 1024, 6);
		CGameInstance::Get().Generate_CubeMap(m_pDynamicShadowDSV.GetAddressOf(), m_pDynamicShadowTexture.GetAddressOf(), m_pDynamicShadowSRV.GetAddressOf(), 1024, 6);
	}
	else {
		CGameInstance::Get().Generate_ShadowTexture(m_pStaticShadowDSV.GetAddressOf(), m_pStaticShadowTexture.GetAddressOf(), m_pStaticShadowSRV.GetAddressOf(), 1280, 720);
		CGameInstance::Get().Generate_ShadowTexture(m_pDynamicShadowDSV.GetAddressOf(), m_pDynamicShadowTexture.GetAddressOf(), m_pDynamicShadowSRV.GetAddressOf(), 1280, 720);
	}

	DirtyFlag = true;
	   
	return S_OK;
}

VOID CLight::Set_LightRange(_float _Range){
	DynamicLight.LightRange = _Range;
	DirtyFlag = true;
	Update_PointLight_ProjectionMatrix(_Range);
}

VOID CLight::Update_PointLight_ProjectionMatrix(_float _Range) {
	
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
