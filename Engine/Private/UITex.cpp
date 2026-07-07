#include "pch.h"
#include "GameInstance.h"
#include "UITex.h"

NS_USING(Engine)

CUITex::CUITex()
{

}

CUITex::~CUITex()
{
}

HRESULT CUITex::Initialize(void* pArg)
{
	auto		pDesc = static_cast<CUIObject::UIOBJECT_DESC*>(pArg);

	if (FAILED(CUIObject::Initialize(pDesc)))
		return E_FAIL;
}

void CUITex::Update(E::_float fTimeDelta)
{
	CUIObject::Update(fTimeDelta);
}

void CUITex::LateUpdate(_float fTimeDelta)
{
}

void CUITex::Creating()
{
}

void CUITex::StartHovering()
{
}

void CUITex::Hovering()
{
}

void CUITex::EndHovering()
{
}

void CUITex::Ending()
{
}
