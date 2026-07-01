#include "pch.h"
#include "Gobline.h"

NS_USING(Client)
CGobline::CGobline()
{
}

CGobline::~CGobline()
{
}

HRESULT CGobline::Initialize(void* pArg)
{
	return E_NOTIMPL;
}

void CGobline::PriorityUpdate(E::_float fTimeDelta)
{
}

void CGobline::Update(E::_float fTimeDelta)
{
}

void CGobline::LateUpdate(E::_float fTimeDelta)
{
}

HRESULT CGobline::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	return S_OK;
}

E::UPtr<E::CPrototype> CGobline::Clone(void* pArg)
{
	return E::UPtr<E::CPrototype>();
}
