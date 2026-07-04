#include "pch.h"
#include "Gobline.h"
#include "ComBeHavior.h"
NS_USING(Client)
CGobline::CGobline()
{
}

CGobline::~CGobline()
{
}

HRESULT CGobline::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;
	{
		CComBeHavior::BEHAVIOR_DESC Desc{};
		if (FAILED(AddComponentFromProto("BEHAVIOR", "Prototype_Component_BeHavior", "Com_BT", &Desc, &m_pComBT)))
		{
			return E_FAIL;
		};
	}
	return S_OK;
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

E::UPtr<CGobline> CGobline::Create()
{
	auto pInstance = E::ToUPtr(new CGobline{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CGobline");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CGobline::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CGobline{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CGobline");
		return nullptr;
	}

	return pInstance;
}
