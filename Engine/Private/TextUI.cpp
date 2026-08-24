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
	m_textInfo.Alignment = pDesc->Alignment;

	m_textInfo.Text = L"한글";

	if (FAILED(CUIObject::Initialize(pDesc)))
		return E_FAIL;

	return S_OK;
}

void CTextUI::Update(_float fTimeDelta)
{
	CUIObject::Update(fTimeDelta);

	_float fontscale = m_UIINFO.SizeX;

	if (std::nullopt != m_pParent)
	{
		CUIObject* parentUI = E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_pParent);
		UI_INFO& parentInfo = parentUI->GetUIInfo();

		m_ScaleRatio = parentUI->GetScaleRatio();
		fontscale = m_UIINFO.SizeX * parentUI->GetScaleRatio();
	}

	const StringID fontName = m_UIINFO.Name == "64px" ?
		"Pretendard_64px" : "Pretendard";
	_float originX = m_UIINFO.SizeX * 0.5f;
	if (m_textInfo.Alignment != TEXT_ALIGN::LEFT)
	{
		const _float2 textSize = CGameInstance::Get().FontMeasureString(
			fontName,
			m_textInfo.Text.c_str());
		originX = m_textInfo.Alignment == TEXT_ALIGN::CENTER ?
			textSize.x * 0.5f : textSize.x;
	}

	const _float2 textOrigin = {
		originX,
		m_UIINFO.SizeY * 0.5f
	};
	const RENDERGROUP renderGroup = GetResolvedRenderGroup();

	if (m_UIINFO.Name == "64px")
	{
		CGameInstance::Get().FontAddLateDraw(
			renderGroup,
			fontName,
			m_textInfo.Text.c_str(),
			{ m_UIINFO.fX, m_UIINFO.fY },
			fontscale,
			XMVectorSet(m_UIINFO.Color.x, m_UIINFO.Color.y, m_UIINFO.Color.z, m_UIINFO.Alpha),
			0.f,
			textOrigin
		);
	}
	else
	{
		CGameInstance::Get().FontAddLateDraw(renderGroup, fontName, m_textInfo.Text.c_str(),
			{ m_UIINFO.fX, m_UIINFO.fY }, fontscale, XMVectorSet(m_UIINFO.Color.x, m_UIINFO.Color.y, m_UIINFO.Color.z, m_UIINFO.Alpha),
			0.f, textOrigin);

		CGameInstance::Get().FontAddLateDraw(renderGroup, fontName, m_textInfo.Text.c_str(),
			{ m_UIINFO.fX, m_UIINFO.fY }, fontscale, XMVectorSet(m_UIINFO.Color.x, m_UIINFO.Color.y, m_UIINFO.Color.z, m_UIINFO.Alpha),
			0.f, textOrigin);
	}
}

void CTextUI::PlayEffect(uint32_t uiState)
{
}
