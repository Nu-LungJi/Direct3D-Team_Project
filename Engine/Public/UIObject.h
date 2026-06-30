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
		_float			fX, fY, fSizeX, fSizeY;
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
	void SetSize(_float2 f) { m_fSizeX = f.x; m_fSizeY = f.y; CalcUICoord();}

protected:
	void CalcUICoord();

protected:
	_float m_fX{}, m_fY{}, m_fSizeX{}, m_fSizeY{};
};

NS_END
