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

	if (m_bFixedDigitLayout && !m_textInfo.Text.empty())
	{
		// SpriteFont uses proportional glyph advances. Draw each digit inside a
		// fixed-width cell so values such as 1:59:99 and 0:00:00 never shift the
		// minute/second/centisecond columns. Separators keep their natural width.
		_float maxDigitWidth = 0.f;
		for (wchar_t digit = L'0'; digit <= L'9'; ++digit)
		{
			const wchar_t glyph[] = { digit, L'\0' };
			maxDigitWidth = std::max(
				maxDigitWidth,
				CGameInstance::Get().FontMeasureString(fontName, glyph).x);
		}

		_float totalUnscaledWidth = 0.f;
		for (const wchar_t character : m_textInfo.Text)
		{
			if (character >= L'0' && character <= L'9')
			{
				totalUnscaledWidth += maxDigitWidth;
			}
			else
			{
				const wchar_t glyph[] = { character, L'\0' };
				totalUnscaledWidth += CGameInstance::Get().
					FontMeasureString(fontName, glyph).x;
			}
		}

		_float cursorUnscaled = 0.f;
		if (m_textInfo.Alignment == TEXT_ALIGN::CENTER)
			cursorUnscaled = -totalUnscaledWidth * 0.5f;
		else if (m_textInfo.Alignment == TEXT_ALIGN::RIGHT)
			cursorUnscaled = -totalUnscaledWidth;
		else
			cursorUnscaled = -m_UIINFO.SizeX * 0.5f;
		for (const wchar_t character : m_textInfo.Text)
		{
			const wchar_t glyph[] = { character, L'\0' };
			const _float2 glyphSize = CGameInstance::Get().
				FontMeasureString(fontName, glyph);
			const _float cellWidth =
				(character >= L'0' && character <= L'9') ?
				maxDigitWidth : glyphSize.x;
			const _float2 glyphOrigin = {
				glyphSize.x * 0.5f,
				m_UIINFO.SizeY * 0.5f
			};
			const _float2 glyphPosition = {
				m_UIINFO.fX +
					(cursorUnscaled + cellWidth * 0.5f) * fontscale,
				m_UIINFO.fY
			};
			const _vector color = XMVectorSet(
				m_UIINFO.Color.x,
				m_UIINFO.Color.y,
				m_UIINFO.Color.z,
				m_UIINFO.Alpha);

			CGameInstance::Get().FontAddLateDraw(
				renderGroup,
				fontName,
				glyph,
				glyphPosition,
				fontscale,
				color,
				0.f,
				glyphOrigin);
			// Preserve the existing TextUI double draw used by ordinary text.
			CGameInstance::Get().FontAddLateDraw(
				renderGroup,
				fontName,
				glyph,
				glyphPosition,
				fontscale,
				color,
				0.f,
				glyphOrigin);

			cursorUnscaled += cellWidth;
		}
		return;
	}

	const _bool hasColoredSuffix =
		!m_ColoredSuffix.empty() &&
		m_textInfo.Text.size() >= m_ColoredSuffix.size() &&
		m_textInfo.Text.compare(
			m_textInfo.Text.size() - m_ColoredSuffix.size(),
			m_ColoredSuffix.size(),
			m_ColoredSuffix) == 0;
	if (hasColoredSuffix)
	{
		const std::wstring prefix = m_textInfo.Text.substr(
			0,
			m_textInfo.Text.size() - m_ColoredSuffix.size());
		const _float2 prefixSize = CGameInstance::Get().FontMeasureString(
			fontName, prefix.c_str());
		const _float2 suffixSize = CGameInstance::Get().FontMeasureString(
			fontName, m_ColoredSuffix.c_str());
		const _float totalWidth = prefixSize.x + suffixSize.x;

		_float leftX = m_UIINFO.fX;
		if (m_textInfo.Alignment == TEXT_ALIGN::CENTER)
			leftX -= totalWidth * fontscale * 0.5f;
		else if (m_textInfo.Alignment == TEXT_ALIGN::RIGHT)
			leftX -= totalWidth * fontscale;
		else
			leftX -= m_UIINFO.SizeX * fontscale * 0.5f;

		const _float2 segmentOrigin = { 0.f, m_UIINFO.SizeY * 0.5f };
		const _vector prefixColor = XMVectorSet(
			m_UIINFO.Color.x,
			m_UIINFO.Color.y,
			m_UIINFO.Color.z,
			m_UIINFO.Alpha);
		const _vector suffixColor = XMVectorSet(
			m_ColoredSuffixColor.x,
			m_ColoredSuffixColor.y,
			m_ColoredSuffixColor.z,
			m_UIINFO.Alpha);

		for (uint32_t drawIndex = 0; drawIndex < 2u; ++drawIndex)
		{
			CGameInstance::Get().FontAddLateDraw(
				renderGroup, fontName, prefix.c_str(),
				{ leftX, m_UIINFO.fY }, fontscale, prefixColor,
				0.f, segmentOrigin);
			CGameInstance::Get().FontAddLateDraw(
				renderGroup, fontName, m_ColoredSuffix.c_str(),
				{ leftX + prefixSize.x * fontscale, m_UIINFO.fY },
				fontscale, suffixColor, 0.f, segmentOrigin);
		}
		return;
	}

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
