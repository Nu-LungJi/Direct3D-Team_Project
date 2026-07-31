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

	if (m_UIINFO.Name == "64px")
	{
		CGameInstance::Get().FontAddLateDraw(
			RENDERGROUP::UI,
			"Pretendard_64px",
			m_textInfo.Text.c_str(),
			{ m_UIINFO.fX, m_UIINFO.fY },
			m_UIINFO.SizeX,
			XMVectorSet(m_UIINFO.Color.x, m_UIINFO.Color.y, m_UIINFO.Color.z, m_UIINFO.Alpha),
			0.f,
			{ m_UIINFO.SizeX * 0.5f,  m_UIINFO.SizeY * 0.5f }
		);
	}
	else
	{
		CGameInstance::Get().FontAddLateDraw(RENDERGROUP::UI, "Pretendard", m_textInfo.Text.c_str(),
			{ m_UIINFO.fX, m_UIINFO.fY }, m_UIINFO.SizeX, XMVectorSet(m_UIINFO.Color.x, m_UIINFO.Color.y, m_UIINFO.Color.z, m_UIINFO.Alpha),
			0.f, { m_UIINFO.SizeX * 0.5f,  m_UIINFO.SizeY * 0.5f });
	}
}

void CTextUI::PlayEffect(uint32_t uiState)
{
}
