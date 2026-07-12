#include "pch.h"
#include "Monster.h"
#include "ComBeHavior.h"
NS_USING(Client)
CMonster::CMonster()
{
}

CMonster::~CMonster()
{
}

HRESULT CMonster::Initialize(void* pArg)
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

void CMonster::PriorityUpdate(E::_float fTimeDelta)
{
	if (m_iHp <= 0.f)
	{
		m_bDead = true;
	}
}

void CMonster::Update(E::_float fTimeDelta)
{
}

void CMonster::LateUpdate(E::_float fTimeDelta)
{
}

HRESULT CMonster::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	

	return S_OK;
}

E::UPtr<CMonster> CMonster::Create()
{
	auto pInstance = E::ToUPtr(new CMonster{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CMonster");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CMonster::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CMonster{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CMonster");
		return nullptr;
	}

	return pInstance;
}
