#include "pch.h"
#include "VolumetricFog.h"
#include "GameInstance.h"

CVolumetricFog::CVolumetricFog()	{}
CVolumetricFog::CVolumetricFog(const CVolumetricFog& Prototype) : CGameObject{ Prototype } {}
CVolumetricFog::~CVolumetricFog()	{}

HRESULT CVolumetricFog::Initialize(VOID* pArg) {

	if (FAILED(CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	}
	return S_OK;
}

VOID CVolumetricFog::PriorityUpdate(_float fTimeDelta) {
	XMFLOAT4X4	FogTransform = *GetTransform().GetWorldMatrix();
	XMMATRIX	FogInverseTransform = XMMatrixInverse(nullptr, XMLoadFloat4x4(&FogTransform));
}

VOID CVolumetricFog::Update(_float fTimeDelta) {

}

VOID CVolumetricFog::LateUpdate(_float fTimeDelta) {
	GetTransform().Update();
	CGameInstance::Get().AddRenderObject(RENDERGROUP::LIGHT, this);
}

VOID CVolumetricFog::UpdateGUI() {
	CGameObject::UpdateGUI();
}

HRESULT CVolumetricFog::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) {

	return S_OK;
}

Engine::UPtr<CVolumetricFog> CVolumetricFog::Create() {
	auto pInstance = ToUPtr(new CVolumetricFog{});
	if (FAILED(pInstance->InitializePrototype())) {
		MSG_BOX("Failed to Create: CVolumetricFog");
		return nullptr;
	}

	return pInstance;
}
Engine::UPtr<CPrototype>	 CVolumetricFog::Clone(VOID* pArg) {
	auto pInstance = ToUPtr(new CVolumetricFog{ *this });
	if (FAILED(pInstance->Initialize(pArg))) {
		MSG_BOX("Failed to Cloned: CVolumetricFog");
		return nullptr;
	}

	return pInstance;
}
