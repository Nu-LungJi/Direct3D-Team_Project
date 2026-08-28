#pragma once

#include "GameObject.h"
#include "UIObject.h"

NS_BEGIN(Engine)

enum class TEXT_ALIGN : uint32_t
{
	LEFT = 0,
	CENTER,
	RIGHT
};

typedef struct tagTextInfo
{
	std::wstring Text{ L"" };
	TEXT_ALIGN Alignment{ TEXT_ALIGN::LEFT };
}TEXT_INFO;

class ENGINE_DLL CTextUI : public CUIObject
{
public:
	DECLARE_DERIVED_TYPE(CTextUI, CUIObject)

public:
	typedef struct tagTextDesc : public E::CUIObject::UIOBJECT_DESC
	{
		std::wstring Text{ L"" };
		TEXT_ALIGN Alignment{ TEXT_ALIGN::LEFT };
	}TEXT_DESC;

protected:
	CTextUI();
	~CTextUI() override;

public:
	HRESULT Initialize(void* pArg) override;
	virtual void Update(_float fTimeDelta);

protected:
	virtual void PlayEffect(uint32_t uiState);
public:
	void SetwText(std::wstring text) { m_textInfo.Text = text; }
	std::wstring GetwText() { return m_textInfo.Text; }
	void SetTextAlignment(TEXT_ALIGN alignment) { m_textInfo.Alignment = alignment; }
	TEXT_ALIGN GetTextAlignment() const { return m_textInfo.Alignment; }
	void SetFixedDigitLayout(_bool enabled) { m_bFixedDigitLayout = enabled; }
	void SetColoredSuffix(const std::wstring& suffix, const _float3& color)
	{
		m_ColoredSuffix = suffix;
		m_ColoredSuffixColor = color;
	}
	void ClearColoredSuffix() { m_ColoredSuffix.clear(); }
public:
	TEXT_INFO& GetTextInfo() { return m_textInfo; }
	const TEXT_INFO& GetTextInfo() const { return m_textInfo; }

protected:
	TEXT_INFO m_textInfo{};
	_bool m_bFixedDigitLayout{ false };
	std::wstring m_ColoredSuffix{};
	_float3 m_ColoredSuffixColor{ 1.f, 1.f, 1.f };
};

NS_END
