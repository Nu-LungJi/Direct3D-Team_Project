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
	DECLARE_DERIVED_TYPE(CUITex, CUIObject)

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
public:
	TEXT_INFO& GetTextInfo() { return m_textInfo; }
	const TEXT_INFO& GetTextInfo() const { return m_textInfo; }

protected:
	TEXT_INFO m_textInfo{};
};

NS_END
