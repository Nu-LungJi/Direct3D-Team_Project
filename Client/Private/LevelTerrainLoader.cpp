#include "pch.h"
#include "LevelTerrainLoader.h"
#include "GameInstance.h"
#include "Level_Defines.h"
#include "Terrain.h"
#include "Client_Resources.h"
#include "OilBarrel.h"
#include "RagdollTest.h"

#include "Player.h"
#include "PlayerThirdPersonCamera.h"
#include "Player_Weapon.h"
#include "Player_Magic_Bullet.h"
#include "TmbGurdian.h"
#include "TmbGurdianDead.h"
#include "Mon_Weapon.h"
#include "BossTMB.h"
NS_USING(Client)

std::future<bool> CLevelTerrainLoader::Load()
{
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_TERRAIN", []()
		{
			if (FAILED(E::CGameInstance::Get().LoadCinematic("AcientThunderAttack")))
			{
				return false;
			}

			// oilbarrel
			{
				if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>(LEVEL::TERRAIN, "Static_OilBarrel_Resource",
					CResStaticModel::Create("./Resources/SampleClient/Models/Static/SM_oil_barrel_0001.bin"))) {

					E::CResStaticModel::DESC pDesc{};
					pDesc.PreTransformMatrix = XMMatrixScaling(300.f, 300.f, 300.f);

					if (FAILED(res->Load(pDesc)))
					{
						MSG_BOX("TERRAIN Failed Static_OilBarrel_Resource");
						//return false;
					}
				}
				if (FAILED(E::CGameInstance::Get().AddPrototype(
					LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_OilBarrel, COilBarrel::Create())))
				{
					MSG_BOX("TERRAIN Failed Prototype_GameObject_OilBarrel");
					return false;
				}
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::TERRAIN,
				PROTO_GAMEOBJECT::Prototype_GameObject_RagdollTest,
				CRagdollTest::Create())))
			{
				MSG_BOX("TERRAIN Failed Prototype_GameObject_RagdollTest");
				return false;
			}

			// terrain
			{
				if (auto res = CGameInstance::Get().AddResource(LEVEL::TERRAIN, "TEX2D_Terrain_Tile0", CResTexture2D::Create("./Resources/SampleClient/Textures/Terrain/Tile0.dds")))
				{
					if (FAILED(res->Load()))
					{
						MSG_BOX("");
						//return E_FAIL;
					}
				}
				if (auto res = CGameInstance::Get().AddResource(LEVEL::TERRAIN, "VIBUFFER_Terrain", CResTerrainVIBuffer::Create("./Resources/SampleClient/Textures/Terrain/Height.bmp")))
				{
					if (FAILED(res->Load(CResTerrainVIBuffer::DESC{})))
					{
						MSG_BOX("LEVEL::TERRAIN Failed VIBUFFER_Terrain ");
						//return false;
					}
				}

				if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_Terrain, CTerrain::Create())))
				{
					MSG_BOX("TERRAIN Failed Prototype_GameObject_Terrain");
					return false;
				}
			}

			//TombBos
			{
				if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(LEVEL::BOSS_CHARLES_ROOKWOOD, "Model_Resource_TombProtector",
					CResModel::Create("./Resources/SampleClient/Models/Skeleton/Tomb_Protector/SK_Tomb_Protector.bin")))
				{
					E::CResModel::DESC pDesc{};
					pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixRotationY(XMConvertToRadians(180.f));
					if (FAILED(res->Load(pDesc)))
					{
						MSG_BOX("LEVEL_CREATURE Failed Model_Resource_TombProtector");
						return false;
					}
				}

				if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::BOSS_CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_BossTMB, CBossTMB::Create())))
				{
					MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_BossTMB");
					return false;
				}
			}

			if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(LEVEL::TERRAIN, "PLAYER_MODEL_RESROUCE", CResModel::Create("./Resources/SampleClient/Models/Skeleton/professor/SK_professor.bin"))) {

				E::CResModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixScaling(3.f, 3.f, 3.f) * XMMatrixRotationY(XMConvertToRadians(180.f)) * XMMatrixTranslation(0.f, -1.5f, 0.f);
				if (FAILED(res->Load(pDesc))) {
					MSG_BOX("TERRAIN Failed PLAYER_MODEL_RESROUCE");
					return false;
				}
			}
		
			if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>(LEVEL::TERRAIN, "PLAYER_WEAPON_RESROUCE", CResStaticModel::Create("./Resources/SampleClient/Models/Static/SM_Wand.bin"))) {

				E::CResStaticModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f);
				if (FAILED(res->Load(pDesc))) {
					MSG_BOX("TERRAIN Failed PLAYER_WEAPON_RESROUCE");
					return false;
				}
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_Player, CPlayer::Create())))
			{
				MSG_BOX("TERRAIN Failed Prototype_GameObject_Player");
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_PlayerThirdPersonCamera, CPlayerThirdPersonCamera::Create())))
			{
				MSG_BOX("TERRAIN Failed Prototype_GameObject_PlayerThirdPersonCamera");
				return false;
			}
			if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_PlayerWeapon, CPlayer_Weapon::Create())))
			{
				MSG_BOX("TERRAIN Failed Prototype_GameObject_PlayerWeapon");
				return false;
			}
			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_PlayerMagicBullet, CPlayer_Magic_Bullet::Create())))
			{
				MSG_BOX("TERRAIN Failed Prototype_GameObject_PlayerMagicBullet");
				return false;
			}
			//TombGurDian
			{
				if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(LEVEL::CHARLES_ROOKWOOD, "Model_Resource_TMBGurdian",
					CResModel::Create("./Resources/SampleClient/Models/Skeleton/Tomb_Grunt/SK_Tomb_Grunt.bin")))
				{
					E::CResModel::DESC pDesc{};
					pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixRotationY(XMConvertToRadians(180.f));
					if (FAILED(res->Load(pDesc)))
					{
						MSG_BOX("LEVEL_CREATURE Failed Model_Resource_TMBGurdian");
						return false;
					}
				}

				if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_TMBGurdian, CTmbGurdian::Create())))
				{
					MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_TMBGurdian");
					return false;
				}

			}
			//TombGurDianDead
			{
				for (uint32_t i = 0; i < 13; ++i)
				{
					std::string path = "./Resources/SampleClient/Models/Static/SM_Med_" + std::to_string(i) + ".bin";
					StringID resTag = "Static_Med_Debris_" + std::to_string(i);
					if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>(LEVEL::CHARLES_ROOKWOOD, resTag,
						CResStaticModel::Create(path))) {

						E::CResStaticModel::DESC pDesc{};
						pDesc.PreTransformMatrix = XMMatrixIdentity();

						if (FAILED(res->Load(pDesc)))
						{
							MSG_BOX("LEVEL_CREATURE Failed Static_Med_Debris");
						}
					}
				}
				if (FAILED(E::CGameInstance::Get().AddPrototype(
					LEVEL::CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_TmbGurdianDead, CTmbGurdianDead::Create())))
				{
					MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_TmbGurdianDead");
					return false;
				}
				if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>(LEVEL::CHARLES_ROOKWOOD, "Model_Resource_Mace",
					CResStaticModel::Create("./Resources/SampleClient/Models/Static/SM_Tomb_Mace.bin"))) {

					E::CResStaticModel::DESC pDesc{};
					pDesc.PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f);

					if (FAILED(res->Load(pDesc)))
					{
						MSG_BOX("LEVEL_CREATURE Failed Static_Mace_Model_Resource");
						return false;
					}
				}
				if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_Mace, CMon_Weapon::Create())))
				{
					MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_Mace");
					return false;
				}
			}
			// 워커 스레드 종료
			return  true;
		});
}

std::future<bool> CLevelTerrainLoader::UnLoad()
{
	LOG_MEMORY("start");
	LOG_MEMORY("end");
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("UNLOADING_TERRAIN", []()
		{
			CGameInstance::Get().DelPrototype(LEVEL::TERRAIN);
			CGameInstance::Get().DelResource(LEVEL::TERRAIN);

			return true;
		});
}
