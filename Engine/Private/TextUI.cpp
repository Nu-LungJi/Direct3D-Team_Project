#include "pch.h"
#include "GameInstance.h"
#include "TextUI.h"

CTextUI::CTextUI()
{
}

CTextUI::~CTextUI()
{
}

HRESULT CTextUI::Initialize(void* pArg)
{
	auto		pDesc = static_cast<CTextUI::TEXT_DESC*>(pArg);

	m_textInfo.Text = L"한글";

	if (FAILED(CUIObject::Initialize(pDesc)))
		return E_FAIL;

	return S_OK;
}

void CTextUI::Update(_float fTimeDelta)
{
	CUIObject::Update(fTimeDelta);

	auto clientSize = CGameInstance::Get().GetClientScreenSize();

	_float fontscale = m_UIINFO.SizeX;

	if (std::nullopt != m_pParent)
	{
		CUIObject* parentUI = E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_pParent);
		UI_INFO& parentInfo = parentUI->GetUIInfo();

		m_ScaleRatio = parentUI->GetScaleRatio();
		fontscale = m_UIINFO.SizeX * parentUI->GetScaleRatio();
	}

	if (m_UIINFO.Name == "64px")
	{
		CGameInstance::Get().FontAddLateDraw(
			RENDERGROUP::UI,
			"Pretendard_64px",
			m_textInfo.Text.c_str(),
			{ m_UIINFO.fX, m_UIINFO.fY },
			fontscale,
			XMVectorSet(m_UIINFO.Color.x, m_UIINFO.Color.y, m_UIINFO.Color.z, m_UIINFO.Alpha),
			0.f,
			{ m_UIINFO.SizeX * 0.5f,  m_UIINFO.SizeY * 0.5f }
		);
	}
	else
	{
		CGameInstance::Get().FontAddLateDraw(RENDERGROUP::UI, "Pretendard", m_textInfo.Text.c_str(),
			{ m_UIINFO.fX, m_UIINFO.fY }, fontscale, XMVectorSet(m_UIINFO.Color.x, m_UIINFO.Color.y, m_UIINFO.Color.z, m_UIINFO.Alpha),
			0.f, { m_UIINFO.SizeX * 0.5f,  m_UIINFO.SizeY * 0.5f });

		CGameInstance::Get().FontAddLateDraw(RENDERGROUP::UI, "Pretendard", m_textInfo.Text.c_str(),
			{ m_UIINFO.fX, m_UIINFO.fY }, fontscale, XMVectorSet(m_UIINFO.Color.x, m_UIINFO.Color.y, m_UIINFO.Color.z, m_UIINFO.Alpha),
			0.f, { m_UIINFO.SizeX * 0.5f,  m_UIINFO.SizeY * 0.5f });
	}
}

void CTextUI::PlayEffect(uint32_t uiState)
{
}
