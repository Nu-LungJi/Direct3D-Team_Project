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
	m_UIINFO.Color = { 1.f, 1.f, 1.f };

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
	if (m_bWorldSpace)
	{
		GetTransform().Update();

		auto pCamera = E::CGameInstance::Get().GetActiveCamera("FLY");
		
		_matrix matView = pCamera->GetView();
		_matrix matProj = pCamera->GetProj();

		_vector vWorldPos = GetTransform().GetLoadedPostion();
		_vector vCamPos = XMLoadFloat3(&pCamera->GetTransform().GetPosition());

		auto clientSize = CGameInstance::Get().GetClientScreenSize();

		_vector vScreenPos = XMVector3Project(
			vWorldPos,
			0.f, 0.f, (float)clientSize.x, (float)clientSize.y, 0.f, 1.f,
			matProj, matView, XMMatrixIdentity()
		);

		_float3 screenPos;
		XMStoreFloat3(&screenPos, vScreenPos);

		if (screenPos.z >= 0.f && screenPos.z <= 1.f)
		{
			float fDistance = XMVectorGetX(XMVector3Length(vWorldPos - vCamPos));

			float fPerspectiveScale = 5.0f / (fDistance + 0.001f);
			fPerspectiveScale = std::clamp(fPerspectiveScale, 0.1f, 3.0f);

			CGameInstance::Get().FontAddLateDraw(
				RENDERGROUP::UI,
				"Pretendard",
				m_textInfo.Text,
				{ screenPos.x, screenPos.y },
				fPerspectiveScale, 
				XMVectorSet(1.f, 1.f, 1.f, m_UIINFO.Alpha),
				0.f,
				{ m_UIINFO.SizeX * 0.5f, m_UIINFO.SizeY * 0.5f }
			);
		}
	}
	else
	{
		CGameInstance::Get().FontAddLateDraw(RENDERGROUP::UI, "Pretendard", m_textInfo.Text.c_str(),
			{ m_UIINFO.fX, m_UIINFO.fY }, m_UIINFO.SizeX, XMVectorSet(m_UIINFO.Color.x, m_UIINFO.Color.y, m_UIINFO.Color.z, m_UIINFO.Alpha), 0.f, { m_UIINFO.SizeX * 0.5f,  m_UIINFO.SizeY * 0.5f });
	}

	return S_OK;
}

void CTextBox::PlayEffect(uint32_t uiState)
{
	if (m_pComTween == nullptr)
		return;

	if (uiState & ETOUI(UI_STATE::APPEAR))
	{
		ClearEffectTweens();
		if (Appear) Appear(this);
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
