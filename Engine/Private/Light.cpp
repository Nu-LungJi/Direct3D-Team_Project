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
	{
		m_pDynamicLight.LightDirection = { 1.f, -1.f, 1.f };
		m_pDynamicLight.LightIntensity = { 10.f };
		m_pDynamicLight.LightColor = { 1.f, 1.f, 1.f };
		m_pDynamicLight.LightRange = { 100.f };

		m_pDynamicLight.Position = { 0.f, 0.f, 0.f };
		m_pDynamicLight.LightType = ETOUI(LIGHT_TYPE::DIRECTIONAL);

		m_pDynamicLight.InnerAttanuation = { 20.f };
		m_pDynamicLight.OuterAttanuation = { 30.f };

		for (uint32_t i = 0; i < MAX_LIGHT_MAPCOUNT; ++i)
			XMStoreFloat4x4(&m_pDynamicLight.g_LightViewProj[i], XMMatrixIdentity());
	}
	{
		// CubeMap 그림자 촬영 View의 Look벡터
		DirectionVec[0] = { XMVectorSet(+1.f, 0.f, 0.f, 0.f) }; // +X (Right)
		DirectionVec[1] = { XMVectorSet(-1.f, 0.f, 0.f, 0.f) }; // -X (Left)
		DirectionVec[2] = { XMVectorSet(0.f, +1.f, 0.f, 0.f) }; // +Y (Up)
		DirectionVec[3] = { XMVectorSet(0.f, -1.f, 0.f, 0.f) }; // -Y (Down)
		DirectionVec[4] = { XMVectorSet(0.f, 0.f, +1.f, 0.f) }; // +Z (Forward)
		DirectionVec[5] = { XMVectorSet(0.f, 0.f, -1.f, 0.f) }; // -Z (Backward)

		// CubeMap 그림자 촬영 View의 Up벡터
		BaseUpVec[0] = { XMVectorSet(0.f, +1.f, 0.f, 0.f) }; // +X (Up: +Y)
		BaseUpVec[1] = { XMVectorSet(0.f, +1.f, 0.f, 0.f) }; // -X (Up: +Y)
		BaseUpVec[2] = { XMVectorSet(0.f, 0.f, -1.f, 0.f) }; // +Y (Up: -Z)
		BaseUpVec[3] = { XMVectorSet(0.f, 0.f, +1.f, 0.f) }; // -Y (Up: +Z)
		BaseUpVec[4] = { XMVectorSet(0.f, +1.f, 0.f, 0.f) }; // +Z (Up: +Y)
		BaseUpVec[5] = { XMVectorSet(0.f, +1.f, 0.f, 0.f) }; // -Z (Up: +Y)
	}
	
	return S_OK;
}

HRESULT CLight::Initialize(VOID* pArg)
{
	if (FAILED(CGameObject::Initialize(pArg)))			return E_FAIL;

	{
		m_pColliderSphere = CCollSphere::Create(m_pComTransform->GetPosition(), 10.f);
		if (nullptr == m_pColliderSphere) return E_FAIL;

		m_pColliderFrustum = CCollFrustum::Create(XMMatrixIdentity());
		if (nullptr == m_pColliderFrustum) return E_FAIL;
	}
	{
		CComConstantBuffer::DESC PerObjectDesc{};
		PerObjectDesc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject", &PerObjectDesc, &m_pComCBufferPerObject)))	return E_FAIL;
		
		CComConstantBuffer::DESC PerPassDesc{};
		PerPassDesc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_PASS };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerPass", &PerPassDesc, &m_pComCBufferPerPass)))			return E_FAIL;
	}

	return S_OK;
}

VOID CLight::PriorityUpdate(E::_float fTimeDelta) {

}

VOID CLight::Update(E::_float fTimeDelta) {
	m_pComTransform->Update();

	// Point Light
	if (m_pDynamicLight.LightType == ETOUI(LIGHT_TYPE::POINT)) {

		XMVECTOR LightPosition				= m_pComTransform->GetState(STATE::POSITION);
		XMVECTOR PositionOffset				= XMVectorSet(0.f, 0.0001f, 0.f, 0.f);

		XMMATRIX ProjMat = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.f, 0.01f, m_pDynamicLight.LightRange);

		for (uint32_t i = 0; i < MAX_LIGHT_MAPCOUNT; ++i) {
			XMMATRIX ViewMat = XMMatrixLookAtLH(LightPosition, XMVectorAdd(LightPosition + PositionOffset, DirectionVec[i]), BaseUpVec[i]);
			XMStoreFloat4x4(&m_pDynamicLight.g_LightViewProj[i], XMMatrixMultiply(ViewMat, ProjMat));
		}
	}
	// Directional & SpotLight 
	else {
		_float	 fNearZ = 0.01f;

		m_pDynamicLight.OuterAttanuation = std::clamp(m_pDynamicLight.OuterAttanuation, 1.f, 75.f);
		m_pDynamicLight.LightRange		 = std::clamp(m_pDynamicLight.LightRange, 0.01f + 0.01f, 100.f);

		XMVECTOR	LightPosition  = m_pComTransform->GetState(STATE::POSITION);
		XMVECTOR	LightDirection = XMVector3Normalize(XMLoadFloat3(&m_pDynamicLight.LightDirection));
		XMVECTOR	WorldUp		   = XMVectorSet(0.f, 1.f, 0.f, 0.f);

		_float2		ScreenSize	= CGameInstance::Get().GetClientScreenSize();

		_float		FOVAngle	= m_pDynamicLight.OuterAttanuation * 2.f * 1.2f;
		if (FOVAngle > 150.f) FOVAngle = 150.f;

		XMStoreFloat4x4(&LightView, XMMatrixLookAtLH(LightPosition, LightPosition + LightDirection, WorldUp));
		XMStoreFloat4x4(&LightProj, XMMatrixPerspectiveFovLH(XMConvertToRadians(FOVAngle), ScreenSize.x / ScreenSize.y, fNearZ, m_pDynamicLight.LightRange));
		XMStoreFloat4x4(&m_pDynamicLight.g_LightViewProj[0], XMMatrixMultiply(XMLoadFloat4x4(&LightView), XMLoadFloat4x4(&LightProj)));
	}

	Update_Collider();
}
VOID CLight::LateUpdate(E::_float fTimeDelta) {

}
HRESULT CLight::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) {

	return S_OK;
}

VOID CLight::Update_ObjectConstantBuffer(ID3D11DeviceContext* pContext){
	XMMATRIX LightWorldMatrix = XMLoadFloat4x4(m_pComTransform->GetWorldMatrix());
	XMMATRIX LightViewMatrix = XMLoadFloat4x4(&LightView);
	XMMATRIX LightProjMatrix = XMLoadFloat4x4(&LightProj);

	if (m_pDynamicLight.LightType == ETOUI(LIGHT_TYPE::POINT))
	{
		XMVECTOR PositionOffset = XMVectorSet(0.f, 0.0001f, 0.f, 0.f);

		XMVECTOR LightPosition  = LightWorldMatrix.r[3] + PositionOffset;
		XMVECTOR LightDirection = XMVectorSet(1.f, 0.f, 0.f, 0.f);
		XMVECTOR WorldUpVector  = XMVectorSet(0.f, 1.f, 0.f, 0.f);

		_float	 fNearZ = 0.01f;
		LightViewMatrix = XMMatrixLookAtLH(LightPosition, LightPosition + LightDirection, WorldUpVector);
		LightProjMatrix = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.f, fNearZ, m_pDynamicLight.LightRange);
	}
	{
		E::CB_PER_OBJECT cbPerObject{};

		XMMATRIX WorldViewProj = XMMatrixMultiply(XMMatrixMultiply(LightWorldMatrix, LightViewMatrix), LightProjMatrix);

		XMStoreFloat4x4(&cbPerObject.matWVP, WorldViewProj);

		cbPerObject.matWorld = *m_pComTransform->GetWorldMatrix();

		if (FAILED(m_pComCBufferPerObject->MapDiscard(pContext, &cbPerObject, sizeof(cbPerObject))))	return;

		pContext->VSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
		pContext->GSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
		pContext->CSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	}
	{
		E::CB_PER_PASS pCbPerPass{};

		XMMATRIX LightViewProj = XMMatrixMultiply(LightViewMatrix, LightProjMatrix);

		XMStoreFloat4x4(&pCbPerPass.matView, LightViewMatrix);
		XMStoreFloat4x4(&pCbPerPass.matProj, LightProjMatrix);
		XMStoreFloat4x4(&pCbPerPass.matViewProj, LightViewProj);
		XMStoreFloat4x4(&pCbPerPass.matInvView, XMMatrixInverse(nullptr, LightViewMatrix));
		XMStoreFloat4x4(&pCbPerPass.matInvProj, XMMatrixInverse(nullptr, LightProjMatrix));
		XMStoreFloat4x4(&pCbPerPass.matInvViewProj, XMMatrixInverse(nullptr, LightViewProj));
		XMStoreFloat4x4(&pCbPerPass.matShadowLightViewProj, LightViewProj);

		pCbPerPass.vCamPos = m_pComTransform->GetPosition();

		if (FAILED(m_pComCBufferPerPass->MapDiscard(pContext, &pCbPerPass, sizeof(pCbPerPass))))	return;

		pContext->VSSetConstantBuffers(1, 1, m_pComCBufferPerPass->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(1, 1, m_pComCBufferPerPass->GetAdressOfBuffer());
		pContext->GSSetConstantBuffers(1, 1, m_pComCBufferPerPass->GetAdressOfBuffer());
		pContext->CSSetConstantBuffers(1, 1, m_pComCBufferPerPass->GetAdressOfBuffer());
	}
}

VOID CLight::Update_Collider() {
	XMVECTOR PosVec = XMLoadFloat3(&m_pComTransform->GetPosition());

	if		(m_pDynamicLight.LightType == ETOUI(LIGHT_TYPE::SPOTLIGHT)) {
		if (nullptr == m_pColliderFrustum) return;
		
		static_pointer_cast<CCollFrustum>(m_pColliderFrustum)->SetLocalFrustum(XMLoadFloat4x4(&LightProj));

		XMMATRIX InvViewMat = XMMatrixInverse(nullptr, XMLoadFloat4x4(&LightView));

		//XMMATRIX WorldMatrix{};
		//XMVECTOR scale{}, rotation{}, translation{};
		//if (XMMatrixDecompose(&scale, &rotation, &translation, InvViewMat)) {
		//	rotation = XMQuaternionNormalize(rotation);
		//	WorldMatrix = XMMatrixRotationQuaternion(rotation) * XMMatrixTranslationFromVector(translation);
		//}
		//m_pColliderFrustum->Transform(WorldMatrix);
		m_pColliderFrustum->Transform(InvViewMat);

		CGameInstance::Get().AddColliderGroup("Light_Collider", m_pColliderFrustum.get());
	}
	else if (m_pDynamicLight.LightType == ETOUI(LIGHT_TYPE::POINT)) {
		if (nullptr == m_pColliderSphere) return;

		m_pColliderSphere->Transform(XMMatrixTranslationFromVector(PosVec));

		CGameInstance::Get().AddColliderGroup("Light_Collider", m_pColliderSphere.get());
	}
}

HRESULT CLight::Capture_ShadowMap(ID3D11DeviceContext* pContext, const std::vector<CGameObject*>& _ObjectList) {
	RENDER_CTX RCTX{};
	RCTX.pass = RENDERPASS::SHADOW;
	RCTX.eye  = XMLoadFloat3(&m_pComTransform->GetPosition());

	if (m_pDynamicLight.LightType == ETOUI(LIGHT_TYPE::POINT)) {
		XMMATRIX Identity = XMMatrixIdentity();
		RCTX.matView = Identity;
		RCTX.matProj = Identity;
		RCTX.matViewProj = Identity;
	}
	else {
		RCTX.matView = XMLoadFloat4x4(&LightView);
		RCTX.matProj = XMLoadFloat4x4(&LightProj);
		RCTX.matViewProj = RCTX.matView * RCTX.matProj;
	}

	for (auto& GOBJ : _ObjectList) {
		if (nullptr == GOBJ) continue;
		GOBJ->Render(pContext, RCTX);
	}
	
	return S_OK;
}

_bool	CLight::Check_ObjectInArea() {

	return true;
}

HRESULT CLight::Change_LightType(LIGHT_TYPE _LTYPE) {
	if (m_pDynamicLight.LightType == ETOUI(_LTYPE)) return E_FAIL;
	m_pDynamicLight.LightType = ETOUI(_LTYPE);

	DirtyFlag = true;
	   
	return S_OK;
}

VOID	CLight::Add_ShadowRenderGroup(ACTORTYPE _ATYPE, CGameObject* pRenderObject) {
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
