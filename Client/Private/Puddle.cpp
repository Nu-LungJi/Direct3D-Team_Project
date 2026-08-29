#include "pch.h"
#include "Puddle.h"

NS_USING(Client)

CPuddle::CPuddle() : CDecalVolume{} {}
CPuddle::CPuddle(const CPuddle& prototype)
	: CDecalVolume{ prototype }
	, m_pResVertexShader{ prototype.m_pResVertexShader }
	, m_pResPixelShader{ prototype.m_pResPixelShader }
{
}

HRESULT CPuddle::InitializePrototype(void* pArg) {

	m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim");
	if (!m_pResVertexShader || FAILED(m_pResVertexShader->Load()))	return E_FAIL;

	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_HogsmeadePuddle");
	if (!m_pResPixelShader || FAILED(m_pResPixelShader->Load()))	return E_FAIL;

    return S_OK;
}

HRESULT CPuddle::Initialize(void* pArg) {
	if (FAILED(__super::Initialize(pArg))) return E_FAIL;

	CDecalVolume::DECAL_VOLUME_DESC desc{};
	desc.vPosition = puddlePosition;
	desc.vRotation = { 0.f, 0.f, 0.f };
	desc.vScale = { puddleWidth, puddleHeight, puddleLength };
	desc.fOpacity = 1.f;
	desc.fNormalThreshold = 0.7f;
	desc.sMaterialPath = "HogsmeadePuddle Material JSON 경로";

    return S_OK;
}

void CPuddle::PriorityUpdate(E::_float fTimeDelta) {
	__super::PriorityUpdate(fTimeDelta);
}

void CPuddle::Update(E::_float fTimeDelta) {
	__super::Update(fTimeDelta);
}

void CPuddle::LateUpdate(E::_float fTimeDelta) {
	__super::LateUpdate(fTimeDelta);
}

HRESULT CPuddle::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) {

    return S_OK;
}

E::UPtr<CPuddle> CPuddle::Create() {
	auto pInstance = E::ToUPtr(new CPuddle{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CPuddle");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CPuddle::Clone(void* pArg) {
	auto	pInstance = E::ToUPtr(new CPuddle{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CPuddle");
		return nullptr;
	}

	return pInstance;
}
