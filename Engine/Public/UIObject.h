#pragma once

#include "GameObject.h"
NS_BEGIN(Engine)


class ENGINE_DLL CUIObject : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CUIObject, CGameObject)

public:
	typedef struct tagUIObjectDesc : public CGameObject::GAMEOBJECT_DESC
	{
		_float			fX, fY, fSizeX, fSizeY, fAlpha;
		std::string		ResTag;
		uint32_t		ResWeight;
	}UIOBJECT_DESC;

protected:
	CUIObject();
	~CUIObject() override;

public:
	void UpdateGUI() override;

public:
	HRESULT Initialize(void* pArg) override;

public:
	_float2 GetOrigin() const { return { m_fX , m_fY }; }
	_float2 GetSize() const { return{ m_fSizeX , m_fSizeY }; }
	void SetOrigin(_float2 f) { m_fX = f.x; m_fY = f.y; CalcUICoord(); }
	void SetSize(_float2 f) { m_fSizeX = f.x; m_fSizeY = f.y; CalcUICoord(); }
	void SetAlpha(_float fAlpha) { m_fAlpha = fAlpha; }
	_float GetAlpha() { return m_fAlpha; }

	uint32_t	GetWeight() const { return m_iWeight; }
	void		SetWeight(uint32_t weight) { m_iWeight = weight; }

	const char* GetName()				{ return m_cName; }
	void		SetName(_string name)	{ strcpy_s(m_cName, name.c_str()); }

protected:
	void CalcUICoord();

protected:
	_float m_fX{}, m_fY{}, m_fSizeX{}, m_fSizeY{}, m_fAlpha{};
	uint32_t m_iWeight{};
	char m_cName[256] = "";

protected:
	SPtr<CUIObject> m_pParent = nullptr;

	std::vector<SPtr<CUIObject>> m_vChildren;
};

NS_END
