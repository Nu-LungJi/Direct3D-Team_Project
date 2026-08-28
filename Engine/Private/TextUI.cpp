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
	m_textInfo.FontType = pDesc->FontType;

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

	StringID fontName = "Pretendard";
	switch (m_textInfo.FontType)
	{
	case TEXT_FONT_TYPE::BLUE_FOREST_BOLD_20:
		fontName = "BlueForestBold_20px";
		break;
	case TEXT_FONT_TYPE::BLUE_FOREST_BOLD_32:
		fontName = "BlueForestBold_32px";
		break;
	case TEXT_FONT_TYPE::HAKGYOANSIM_PUZZLE_OUTLINE_25:
		fontName = "HakgyoansimPuzzleOutline_25px";
		break;
	case TEXT_FONT_TYPE::PRETENDARD_64:
		fontName = "Pretendard_64px";
		break;
	case TEXT_FONT_TYPE::DEFAULT:
	default:
		break;
	}
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
	const _bool usePuzzleFillAndOutline = m_textInfo.FontType ==
		TEXT_FONT_TYPE::HAKGYOANSIM_PUZZLE_OUTLINE_25;
	const auto queueText = [&, this](
		const _wstring& text,
		const _float2& position,
		const _float2& origin,
		_fvector color,
		_bool strengthen)
	{
		if (usePuzzleFillAndOutline)
		{
			const _vector fillColor = XMVectorSet(
				1.f, 1.f, 1.f, m_UIINFO.Alpha);
			const _vector outlineColor = XMVectorSet(
				0.f, 0.f, 0.f, m_UIINFO.Alpha);
			CGameInstance::Get().FontAddLateDraw(
				renderGroup,
				"HakgyoansimPuzzleBlack_25px",
				text,
				position,
				fontscale,
				fillColor,
				0.f,
				origin);
			CGameInstance::Get().FontAddLateDraw(
				renderGroup,
				"HakgyoansimPuzzleOutline_25px",
				text,
				position,
				fontscale,
				outlineColor,
				0.f,
				origin);
			return;
		}

		CGameInstance::Get().FontAddLateDraw(
			renderGroup,
			fontName,
			text,
			position,
			fontscale,
			color,
			0.f,
			origin);
		if (strengthen)
		{
			CGameInstance::Get().FontAddLateDraw(
				renderGroup,
				fontName,
				text,
				position,
				fontscale,
				color,
				0.f,
				origin);
		}
	};

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
			const _wstring glyphText{ glyph };
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

			queueText(glyphText, glyphPosition, glyphOrigin, color, true);

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

		queueText(
			prefix,
			{ leftX, m_UIINFO.fY },
			segmentOrigin,
			prefixColor,
			true);
		queueText(
			m_ColoredSuffix,
			{ leftX + prefixSize.x * fontscale, m_UIINFO.fY },
			segmentOrigin,
			suffixColor,
			true);
		return;
	}

	queueText(
		m_textInfo.Text,
		{ m_UIINFO.fX, m_UIINFO.fY },
		textOrigin,
		XMVectorSet(
			m_UIINFO.Color.x,
			m_UIINFO.Color.y,
			m_UIINFO.Color.z,
			m_UIINFO.Alpha),
		m_textInfo.FontType != TEXT_FONT_TYPE::PRETENDARD_64);
}

void CTextUI::PlayEffect(uint32_t uiState)
{
}
