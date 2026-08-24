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
	ImGui::Text(m_bPick == true ? "Pick : TRUE" : "Pick FALSE");
	
	if (ImGui::Button("Save"))
	{
		nlohmann::json j;
		JsonSaveLoadManager::SaveJsonTypeFloat3list(j, "SPIDERSPAWN", m_SpawnPos);
		std::ofstream path("./Resources/json/Spawn/SPIDERSPAWN.json");
		path << j.dump(4);
		path.close();
	}

	
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
	m_LeveTag = Desc->LevelTag;
	if (Desc->LevelTag != "TERRAIN")
	{
		CSpider::SPIDER_DESC Spider{};
		Spider.sObjectTag = "Spider";
		m_Handle = Spider.TargetHandle = Desc->handle;
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
	
	if (Desc->LevelTag != "TERRAIN")
	{

		auto pRes = CGameInstance::Get().GetResourceFirst<CResJson>("SPAWNER", "SPIDERSPAWN");
		if (nullptr == pRes)
		{
			MSG_BOX("Load Failed Json To SPIDER SPAWN");
			return E_FAIL;
		}
		auto json = pRes->Get_Json();
		JsonSaveLoadManager::LoadJsonTypeFloat3list(json, "SPIDERSPAWN", m_SpawnPos);
		{
			CSpider::SPIDER_DESC Spider{};
			Spider.sObjectTag = "Spider";
			Spider.TargetHandle = Desc->handle;
			Spider.LevelTag = MagicEnumToStringView(LEVEL::HOGWART_WORLD);
			Spider.ReSourceTag = "Model_Resource_Spider";
			Spider.resBeHaviorMajor = "BTJSON";
			Spider.resBeHaviorMinor = "RUNSPIDER";
			Spider.MonType = MONSTER_TYPE::NORMAL;
			Spider.vScale = _float3(0.1f, 0.1f, 0.1f);
			for (auto& iter : m_SpawnPos)
			{
				Spider.vPos = iter;
				Spider.vPatrollStart = iter;
				Spider.vPatrollEnd = _float3(72.475, -0.696, -108.795);
				Spider.bSpawn = true;
				E::CGameInstance::Get().AddGameObjectToLayer(LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_Spider, "02_Spider", &Spider);

			}

		}
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
	if (CGameInstance::Get().KeyPressing(DIK_LCONTROL) && CGameInstance::Get().KeyPressing(DIK_LSHIFT) &&
		CGameInstance::Get().KeyDown(DIK_B))
		m_bPick = !m_bPick;
	if (m_bPick)
		Picking();

	if (m_bPick)
		Picking_TerrainMon();
}

void CMon_Spawner::LateUpdate(E::_float fTimeDelta)
{
}

void CMon_Spawner::Picking()
{
	if (CGameInstance::Get().MouseDown(MOUSEKEYSTATE::LB))
	{
		if (auto pObj = CGameInstance::Get().GetActiveCamera())
		{
			const _float2 vMousePosition = CGameInstance::Get().GetMousePos();
			const _float2 vViewportSize = CGameInstance::Get().GetClientScreenSize();
			const auto& [ori, dir] = pObj->GetRayFromScreenPixel(vMousePosition, vViewportSize);
			PX_RAYCAST_RESULT rayResult{};
			if (CGameInstance::Get().GetPhysXManager()
				->RayCast(
					{
						.vOrigin = ori,
						.vDirection = dir,
						.fMaxDistance = 1000.f,
						.tFilter = {.iQueryMask = ETOUI(COLLISION_LAYER::WORLD_STATIC)}
					}
					, rayResult))
			{
				rayResult.vHitpos.y += 0.5f;
				m_SpawnPos.push_back(rayResult.vHitpos);
			}
		}
	}
	
}
void CMon_Spawner::Picking_TerrainMon()
{
	if (m_LeveTag == MagicEnumToStringView(LEVEL::TERRAIN) && CGameInstance::Get().MouseDown(MOUSEKEYSTATE::LB))
	{
		if (auto pObj = CGameInstance::Get().GetActiveCamera())
		{
			const _float2 vMousePosition = CGameInstance::Get().GetMousePos();
			const _float2 vViewportSize = CGameInstance::Get().GetClientScreenSize();
			const auto& [ori, dir] = pObj->GetRayFromScreenPixel(vMousePosition, vViewportSize);
			PX_RAYCAST_RESULT rayResult{};
			if (CGameInstance::Get().GetPhysXManager()
				->RayCast(
					{
						.vOrigin = ori,
						.vDirection = dir,
						.fMaxDistance = 1000.f,
						.tFilter = {.iQueryMask = ETOUI(COLLISION_LAYER::WORLD_STATIC)}
					}
					, rayResult))
			{
				rayResult.vHitpos.y += 0.5f;

				CSpider::SPIDER_DESC Spider{};
				Spider.sObjectTag = "Spider";
				Spider.TargetHandle = m_Handle;
				Spider.LevelTag = MagicEnumToStringView(LEVEL::TERRAIN);
				Spider.ReSourceTag = "Model_Resource_Spider";
				Spider.resBeHaviorMajor = "BTJSON";
				Spider.resBeHaviorMinor = "SPIDER";
				Spider.MonType = MONSTER_TYPE::NORMAL;
				Spider.bSpawn = true;
				Spider.vPos = rayResult.vHitpos;
				m_Monsters.push_back(E::CGameInstance::Get().AddGameObjectToLayer(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_Spider, "02_Spider", &Spider).value());

			}
		}
	}
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
