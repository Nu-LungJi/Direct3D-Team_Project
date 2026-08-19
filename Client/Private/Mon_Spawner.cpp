#include "pch.h"
#include "Mon_Spawner.h"
#include "Spider.h"
NS_USING(Client)
CMon_Spawner::CMon_Spawner()
{
}

CMon_Spawner::CMon_Spawner(const CMon_Spawner& rhs)
{
}

CMon_Spawner::~CMon_Spawner()
{
}

void CMon_Spawner::UpdateGUI()
{
}

HRESULT CMon_Spawner::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CMon_Spawner::Initialize(void* pArg)
{
	if (FAILED(CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	}
	auto Desc = static_cast<MON_SPAWNER_DESC*>(pArg);

	
	{
		CSpider::SPIDER_DESC Spider{};
		Spider.sObjectTag = "Spider";
		Spider.TargetHandle = Desc->handle;
		Spider.LevelTag = MagicEnumToStringView(LEVEL::HOGWART_WORLD);
		Spider.ReSourceTag = "Model_Resource_Spider";
		Spider.resBeHaviorMajor = "BTJSON";
		Spider.resBeHaviorMinor = "SPIDER";
		Spider.MonType = MONSTER_TYPE::NORMAL;


		XMStoreFloat3(&Spider.vPos, XMVectorSet(390.f, 52.f, 283.657f, 1.f));
		m_Monsters.push_back(E::CGameInstance::Get().AddGameObjectToLayer(LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_Spider, "02_Spider", &Spider).value());

		XMStoreFloat3(&Spider.vPos, XMVectorSet(361.174f, 56.162f, 268.648f, 1.f));
		m_Monsters.push_back(E::CGameInstance::Get().AddGameObjectToLayer(LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_Spider, "02_Spider", &Spider).value());

		XMStoreFloat3(&Spider.vPos, XMVectorSet(318.072, 60.217f, 239.747f, 1.f));
		m_Monsters.push_back(E::CGameInstance::Get().AddGameObjectToLayer(LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_Spider, "02_Spider", &Spider).value());

		XMStoreFloat3(&Spider.vPos, XMVectorSet(254.278f, 51.272f, 215.513f, 1.f));
		m_Monsters.push_back(E::CGameInstance::Get().AddGameObjectToLayer(LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_Spider, "02_Spider", &Spider).value());

		XMStoreFloat3(&Spider.vPos, XMVectorSet(218.640f, 51.217f, 174.264f, 1.f));
		m_Monsters.push_back(E::CGameInstance::Get().AddGameObjectToLayer(LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_Spider, "02_Spider", &Spider).value());

		XMStoreFloat3(&Spider.vPos, XMVectorSet(226.097f, 48.242f, 122.760f, 1.f));
		m_Monsters.push_back(E::CGameInstance::Get().AddGameObjectToLayer(LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_Spider, "02_Spider", &Spider).value());

		XMStoreFloat3(&Spider.vPos, XMVectorSet(265.425f, 47.085f, 104.495f, 1.f));
		m_Monsters.push_back(E::CGameInstance::Get().AddGameObjectToLayer(LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_Spider, "02_Spider", &Spider).value());

		XMStoreFloat3(&Spider.vPos, XMVectorSet(338.362f, 49.f, 91.720f, 1.f));
		m_Monsters.push_back(E::CGameInstance::Get().AddGameObjectToLayer(LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_Spider, "02_Spider", &Spider).value());

	}

	return S_OK;
}

void CMon_Spawner::PriorityUpdate(E::_float fTimeDelta)
{
	if (CGameInstance::Get().KeyPressing(DIK_LSHIFT)&& CGameInstance::Get().KeyDown(DIK_F1))
	{
		for (auto& iter : m_Monsters)
		{
			auto pSrc = CGameInstance::Get().GetGameObjectByHandleT<CSpider>(iter);
			if (nullptr != pSrc)
				pSrc->Set_Spawn(true);

		}
	}
}

void CMon_Spawner::FixedUpdate(E::_float fTimeDelta)
{
}

void CMon_Spawner::Update(E::_float fTimeDelta)
{
}

void CMon_Spawner::LateUpdate(E::_float fTimeDelta)
{
}

E::UPtr<CMon_Spawner> CMon_Spawner::Create()
{
	auto pInstance = E::ToUPtr(new CMon_Spawner{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CMon_Spawner");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CMon_Spawner::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CMon_Spawner{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CMon_Spawner");
		return nullptr;
	}

	return pInstance;
}
