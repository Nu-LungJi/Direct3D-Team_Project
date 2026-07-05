#include "pch.h"
#include "TestCollider.h"
#include "CollBox.h"
#include "ComCollider.h"
NS_USING(Client)

CTestCollider::CTestCollider()
{
}
CTestCollider::~CTestCollider()
{
}

HRESULT CTestCollider::Initialize(void* pArg)
{
	auto		pDesc = static_cast<DESC*>(pArg);
	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	m_CollGroupID = pDesc->CollGroupID;
	m_bController = pDesc->bIsController;

	//m_pCollider = CCollBox::Create({}, { 1.f, 1.f, 1.f });

	{
		//CComCollider::DESC Desc{};
		//Desc.eCollType = CollType::Box;
		//Desc.vCenter = {};
		//Desc.vExtents = { 1.f, 2.f, 1.f };
		m_vecComCollider.resize(pDesc->collInfos.size());
		uint32_t i = 0;
		for (const auto& comColl : pDesc->collInfos)
		{
			auto Desc = comColl.second;
			if (FAILED(AddComponentFromProto("COLLIDER", "Prototype_Component_Collider", comColl.first, &Desc, &m_vecComCollider[i++])))
			{
				return E_FAIL;
			};
		}
		
	}
    return S_OK;
}

void CTestCollider::PriorityUpdate(E::_float fTimeDelta)
{
	if (m_bController)
	{
		if (CGameInstance::Get().KeyPressing(DIK_LEFT))
		{
			GetTransform().AddPosition(XMVectorSet(-0.1f, 0.f, 0.f, 0.f));
		}
		if (CGameInstance::Get().KeyPressing(DIK_RIGHT))
		{
			GetTransform().AddPosition(XMVectorSet(+0.1f, 0.f, 0.f, 0.f));
		}
		if (CGameInstance::Get().KeyPressing(DIK_UP))
		{
			GetTransform().AddPosition(XMVectorSet(0.f, 0.f, 0.1f, 0.f));
		}
		if (CGameInstance::Get().KeyPressing(DIK_DOWN))
		{
			GetTransform().AddPosition(XMVectorSet(0.f, 0.f, -0.1f, 0.f));
		}
	}
	
}

void CTestCollider::Update(E::_float fTimeDelta)
{
	if (m_bController)
	{
		GetTransform().AddRotation(XMVectorSet(0.f, 1.f, 0.f, 0.f), 90.f * fTimeDelta);
	}

	GetTransform().Update();
	for (auto& pColl : m_vecComCollider)
	{
		CGameInstance::Get().AddColliderGroup(m_CollGroupID, pColl->Get());
		pColl->Get()->Transform(GetTransform().GetLoadedCombinedWorldMatrix());
	}
}

void CTestCollider::LateUpdate(E::_float fTimeDelta)
{
	if (m_bController)
	{
		if (auto* pVecColls = CGameInstance::Get().GetColliderGroup(m_CollGroupID))
		{
			for (auto& myColl : m_vecComCollider)
			{
				for (auto& coll : *pVecColls)
				{
					if (coll == myColl->Get())
					{
						continue;
					}
					if (CGameInstance::Get().IntersectColl(myColl->Get(), coll))
					{
						//
						int x = 0;
					}
				}
			}
			
		}
	}
}

HRESULT CTestCollider::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
    return S_OK;
}

E::UPtr<CTestCollider> CTestCollider::Create()
{
	auto pInstance = E::ToUPtr(new CTestCollider{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CTestCollider");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CTestCollider::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CTestCollider{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTestCollider");
		return nullptr;
	}

	return pInstance;
}
