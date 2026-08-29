#include "pch.h"
#include "Mon_Spawner.h"
#include "Spider.h"
#include "Troll.h"
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
	if (ImGui::Button("Reset"))
		m_SpawnPos.clear();

	if (ImGui::TreeNode("Spawn"))
	{
		if (ImGui::Button("Save EventSpider"))
		{
			nlohmann::json j;
			JsonSaveLoadManager::SaveJsonTypeFloat3list(j, "EVENTSPIDER", m_SpawnPos);
			std::ofstream path("./Resources/json/Spawn/EVENTSPIDER.json");
			path << j.dump(4);
			path.close();
		}
		Debug_Point();

		if (ImGui::Button("Undo"))
		{
			if (!m_SpawnPos.empty())
				m_SpawnPos.pop_back();
		}
		ImGui::TreePop();
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
	m_OnBeforeTrollSpawn = Desc->OnBeforeTrollSpawn;
	if (Desc->LevelTag != "TERRAIN")
	{
		m_SpawnPos.clear();
		CSpider::SPIDER_DESC Spider{};
		Spider.sObjectTag = "Spider";
		m_Handle = Spider.TargetHandle = Desc->handle;
		Spider.LevelTag = MagicEnumToStringView(LEVEL::HOGWART_WORLD);
		Spider.ReSourceTag = "Model_Resource_Spider";
		Spider.resBeHaviorMajor = "BTJSON";
		Spider.resBeHaviorMinor = "SPIDER";
		Spider.MonType = MONSTER_TYPE::NORMAL;
		Spider.bSpawn = false;

		auto pRes = CGameInstance::Get().GetResourceFirst<CResJson>("SPAWNER", "EVENTSPIDER");
		if (nullptr == pRes)
		{
			MSG_BOX("Load Failed Json To EVENTSPIDER SPAWN");
			return E_FAIL;
		}
		auto json = pRes->Get_Json();
		JsonSaveLoadManager::LoadJsonTypeFloat3list(json, "EVENTSPIDER", m_SpawnPos);
		for (auto& iter : m_SpawnPos)
		{
			Spider.vPos = iter;
			m_Monsters.push_back(E::CGameInstance::Get().AddGameObjectToLayer(LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_Spider, "02_Spider", &Spider).value());
		}
		
	}
	return S_OK;
}

void CMon_Spawner::PriorityUpdate(E::_float fTimeDelta)
{
	if (CGameInstance::Get().KeyPressing(DIK_LSHIFT)&& CGameInstance::Get().KeyDown(DIK_F1))
	{
		FCinematicPlayOptions option{};
		option.eStartMode = ECinematicStartMode::Immediate;
		option.fStartBlendDuration = 0.f;
		option.eReturnMode = ECinematicReturnMode::Blend;
		option.fReturnBlendDuration = 1.f;

		CGameInstance::Get().PlayCinematic("SpiderSpawn", option);

		for (auto& iter : m_Monsters)
		{
			auto pSrc = CGameInstance::Get().GetGameObjectByHandleT<CSpider>(iter);
			if (nullptr != pSrc)
				pSrc->Set_Spawn(true);

		}
	}
	if (!m_bTroll)
	{
		if (m_Monsters.empty())
		{
			// 트롤은 Initialize -> Spawn State::Enter에서 즉시 컷신을 재생한다.
			// 배럴을 먼저 등록하고 한 프레임을 넘겨 첫 컷신 프레임부터
			// 렌더/물리 씬에 확실히 존재하도록 한다.
			if (!m_bTrollSpawnPrepared)
			{
				if (m_OnBeforeTrollSpawn && FAILED(m_OnBeforeTrollSpawn()))
				{
					DEBUG_LOG("[MonSpawner] Failed to prepare the troll encounter.\n");
					return;
				}

				m_OnBeforeTrollSpawn = {};
				m_bTrollSpawnPrepared = true;
				return;
			}

			CTroll::TROLL_DESC Troll{};
			Troll.sObjectTag = "Troll";
			Troll.TargetHandle = m_Handle;
			Troll.LevelTag = MagicEnumToStringView(LEVEL::HOGWART_WORLD);
			Troll.vPos = _float3(260.353f, 40.679f, 138.799f);
			Troll.ReSourceTag = "Model_Resource_Troll";
			Troll.WeaponProtoName = MagicEnumToStringView(PROTO_GAMEOBJECT::Prototype_GameObject_TrollWeapon);
			Troll.WeaponResourceName = "Model_Resource_TrollWeapon";
			Troll.resBeHaviorMajor = "BTJSON";
			Troll.resBeHaviorMinor = "TROLL";
			Troll.MonType = MONSTER_TYPE::BOSS;
			if (CGameInstance::Get().AddGameObjectToLayer(
				LEVEL::HOGWART_WORLD,
				PROTO_GAMEOBJECT::Prototype_GameObject_Troll,
				"02.Troll",
				&Troll))
			{
				m_bTroll = true;
			}
			else
			{
				DEBUG_LOG("[MonSpawner] Failed to spawn the troll.\n");
			}
		}
		for (auto iter = m_Monsters.begin(); iter != m_Monsters.end();)
		{
			if (nullptr == CGameInstance::Get().GetGameObjectByHandle(*iter))
			{
				iter = m_Monsters.erase(iter);
				continue;
			}
			++iter;
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

void CMon_Spawner::Debug_Point()
{
	int32_t i = 0;
	for (auto& iter : m_SpawnPos)
	{
		auto pDbgLineRender = CGameInstance::Get().GetDbgLineRender();

		const auto vPreviousColor = pDbgLineRender->GetColor();
		const auto ePreviousDepthMode = pDbgLineRender->GetDepthMode();
		pDbgLineRender->SetColor({ 0.f, 1.f, 1.f, 1.f });
		pDbgLineRender->SetDepthTest(true);
		pDbgLineRender->AddSphere(1.2f, XMMatrixTranslation(iter.x, iter.y, iter.z));
		pDbgLineRender->SetColor(vPreviousColor);
		pDbgLineRender->SetDepthMode(ePreviousDepthMode);
	}
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
