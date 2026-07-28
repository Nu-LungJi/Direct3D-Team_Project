#include "pch.h"
#include "SpellTypeBT.h"
#include "GameInstance.h"
#include "CameraObject.h"
#include "Resources.h"
#include "UIManager.h"
#include "Client_Defines.h"
#include "EffectUI.h"
#include "ButtonComponent.h"
#include "TweenComponent.h"
#include "UIObject.h"
#include "Level_Defines.h"

NS_USING(Client)

CSpellTypeBT::~CSpellTypeBT()
{
}

HRESULT CSpellTypeBT::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CSpellTypeBT::Initialize(void* pArg)
{
	auto		pDesc = static_cast<CUIObject::UIOBJECT_DESC*>(pArg);

	if (FAILED(CUIObject::Initialize(pDesc)))
		return E_FAIL;

	{
		/* Buffer */
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, "CB_PerUI" };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerUI", &Desc, &m_pComCBufferPerUI)))
		{
			return E_FAIL;
		};

		/* Component */
		CComponent::DESC CDesc{};
		Desc.pGameObject = this;

		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::UI, "Prototype_Component_ButtonUI", "Com_Button", &CDesc, &m_pComCButton)))
		{
			return E_FAIL;
		};

		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::UI, "Prototype_Component_Tween", "Com_Tween", &CDesc, &m_pComTween)))
		{
			return E_FAIL;
		};
	}

	m_UIINFO.UIType = ETOUI(UI_TYPE::BUTTON);
	m_UIINFO.AlphaRatio = 0.f;

	return S_OK;
}

void CSpellTypeBT::PriorityUpdate(E::_float fTimeDelta)
{
}

void CSpellTypeBT::Update(E::_float fTimeDelta)
{
}

void CSpellTypeBT::LateUpdate(E::_float fTimeDelta)
{
}

HRESULT CSpellTypeBT::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	return E_NOTIMPL;
}


E::UPtr<CSpellTypeBT> CSpellTypeBT::Create()
{
	auto pInstance = E::ToUPtr(new CSpellTypeBT{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CSpellTypeBT");
		return nullptr;
	}
	return  pInstance;
}


E::UPtr<E::CPrototype> CSpellTypeBT::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CSpellTypeBT{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CButCSpellTypeBTton");
		return nullptr;
	}

	return pInstance;
}
