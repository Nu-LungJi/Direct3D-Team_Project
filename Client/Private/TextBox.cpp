#include "pch.h"
#include "TextBox.h"
#include "GameInstance.h"
#include "CameraObject.h"
#include "Resources.h"
#include "Level_Defines.h"
#include "FlyCamera.h"

NS_USING(Client)

CTextBox::CTextBox()
{

}

CTextBox::~CTextBox()
{
}

HRESULT CTextBox::Initialize(void* pArg)
{
	auto		pDesc = static_cast<CTextUI::UIOBJECT_DESC*>(pArg);

	if (FAILED(CTextUI::Initialize(pDesc)))
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
	
		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::UI, "Prototype_Component_Tween", "Com_Tween", &CDesc, &m_pComTween)))
		{
			return E_FAIL;
		};
	
	
		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::UI, "Prototype_Component_ButtonUI", "Com_Button", &CDesc, &m_pComCButton)))
		{
			return E_FAIL;
		};
	}

	m_UIINFO.UIType = ETOUI(UI_TYPE::TEXT);
	m_UIINFO.AlphaRatio = 1.f;
	m_UIINFO.Color = {1.f, 1.f, 1.f};

	return S_OK;
}

void CTextBox::PriorityUpdate(E::_float fTimeDelta)
{
	return;
}

void CTextBox::Update(E::_float fTimeDelta)
{
	_float2 mousePos = E::CGameInstance::Get().GetMousePos();

	if (!m_isActive)
		return;

	if (m_pComCButton != nullptr)
		m_pComCButton->CheckPixelPerfectCollision(mousePos, true);

	if (m_pComTween != nullptr)
	{
		m_pComTween->Tick(fTimeDelta);
	}

	CTextUI::Update(fTimeDelta);



	if (m_bMouseTracking)
	{
		m_UIINFO.fX = mousePos.x;
		m_UIINFO.fY = mousePos.y;
		CalcUICoord();
	}
}

void CTextBox::LateUpdate(E::_float fTimeDelta)
{
	if (!m_isActive)
		return;

	CUIObject::LateUpdate(fTimeDelta);
}

HRESULT CTextBox::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{

	return S_OK;
}

void CTextBox::PlayEffect(uint32_t uiState)
{
	if (m_pComTween == nullptr)
		return;

	if (uiState & ETOUI(UI_STATE::APPEAR))
	{
		if (Appear)
		{
			ClearEffectTweens();
			Appear(this);
		}
	}

	if (uiState & ETOUI(UI_STATE::DISAPPEAR))
	{
		ClearEffectTweens();
		if (Disappear) Disappear(this);
	}

	if (m_bInputLocked)
		return;
}

E::UPtr<CTextBox> CTextBox::Create()
{
	auto pInstance = E::ToUPtr(new CTextBox{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CTextBox");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CTextBox::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CTextBox{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTextBox");
		return nullptr;
	}

	return pInstance;
}
