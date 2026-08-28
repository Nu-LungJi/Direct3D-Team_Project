#include "pch.h"
#include "LevelTerrainLoader.h"
#include "GameInstance.h"
#include "Level_Defines.h"
#include "Terrain.h"
#include "Client_Resources.h"
#include "OilBarrel.h"
#include "PhysicsDoor.h"
#include "WiggenweldPotion.h"
#include "TestPathPlaybackObject.h"
#include "LuaTestObject.h"
#include "RagdollTest.h"
#include "TombBossBullet.h"
#include "NvClothCape.h"
#include "ResNvClothMesh.h"

#include "Player.h"
#include "PlayerThirdPersonCamera.h"
#include "Player_Weapon.h"
#include "Player_Broom.h"
#include "Player_Magic_Bullet.h"
#include "Player_Bombarda_Bullet.h"
#include "Player_Confringo_Bullet.h"
#include "Player_Stupefy_Bullet.h"
#include "TmbGurdian.h"
#include "TmbGurdianDead.h"
#include "GurdianWeapon.h"
#include "BossTMB.h"
#include "StarBurst.h"
#include "EnderDragon.h"
#include "BossMace.h"
#include "EnderDragon_State.h"
#include "EdgFireBall.h"
#include "EdgBreath.h"
#include "EdgPulse.h"
#include "EdgRandomBall.h"
#include "EdgGasi.h"
#include "Mon_State.h"
#include "Spider.h"
#include "WorldNpc.h"
#include "InteractiveNpc.h"
// UI
#include "UIController.h"
#include "EffectUI.h"
#include "TextureUI.h"
#include "Button.h"
#include "GeneralButton.h"
#include "TextBox.h"
#include "SpellMeter.h"
#include "HPBar.h"
#include "MiniMap.h"
#include "GameOverMask.h"
#include "VideoObject.h"
#include "Cursor.h"
#include "SpellMiniGame.h"
#include "Mon_Spawner.h"
#include "UITextureResourceLoader.h"
#include "Coin.h"
#include "AccioBall.h"
#include "AccioActivity_Base.h"
#include "AccioActivity_Platform.h"
#include "AccioActivity_BumperA.h"
#include "AccioActivity_BumperB.h"
#include "AccioActivity_RampLarge.h"
#include "AccioActivity_LampSmall.h"
#include "AccioActivity_NpcController.h"
#include "AccioActivity_NpcCharacter.h"

NS_USING(Client)

std::future<bool> CLevelTerrainLoader::Load()
{
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_TERRAIN", []()
		{
			if (FAILED(E::CGameInstance::Get().LoadCinematic("AcientThunderAttack")))
			{
				return false;
			}
			if (FAILED(E::CGameInstance::Get().LoadCinematic("InteractiveNpcDialogue")))
			{
				return false;
			}
			if (FAILED(E::CGameInstance::Get().LoadCinematic("Lightning")))
			{
				return false;
			}
			// [LSY] 테스트 레벨에서도 플레이어 스킬 컷씬을 사용할 수 있도록 미리 등록한다.
			if (FAILED(E::CGameInstance::Get().LoadCinematic("AvadaKedavra")))
			{
				return false;
			}

			UILoad();
			// Accio 물리 상호작용 테스트용 Quaffle 공.
			{
				const auto loadAccioBallResource = [](
					const StringID& resourceTag, const _char* modelPath)
				{
					auto resource = CGameInstance::Get().AddResourceT<CResStaticModel>(
						LEVEL::TERRAIN, resourceTag, CResStaticModel::Create(modelPath));
					if (!resource)
						return false;

					CResStaticModel::DESC desc{};
					desc.PreTransformMatrix = XMMatrixIdentity();
					return SUCCEEDED(resource->Load(desc));
				};

				if (!loadAccioBallResource(
					"Static_AccioBall_Blue_Resource",
					"./Resources/SampleClient/Models/Static/SM_SM_HM_Quid_BallBox_Quaffle_RoundA_Blue.bin") ||
					!loadAccioBallResource(
						"Static_AccioBall_Red_Resource",
						"./Resources/SampleClient/Models/Static/SM_SM_HM_Quid_BallBox_Quaffle_RoundA_Red.bin"))
					return false;

				if (FAILED(CGameInstance::Get().AddPrototype(
					LEVEL::TERRAIN,
					PROTO_GAMEOBJECT::Prototype_GameObject_AccioBall,
					CAccioBall::Create())))
				{
					return false;
				}
			}

			// Accio Activity 경기장 파츠. 충돌체 없이 정적 MapMesh 모델만 등록한다.
			{
				struct ACCIO_ACTIVITY_RESOURCE
				{
					const _char* pTag;
					const _char* pPath;
					_matrix PreTransformMatrix;
				};

				const ACCIO_ACTIVITY_RESOURCE resources[] =
				{
					{ "Static_AccioActivity_Resource", "./Resources/SampleClient/Models/Static/SM_SM_HW_AccioActivity.bin", XMMatrixScaling(500, 500, 500) * XMMatrixRotationRollPitchYaw(XMConvertToRadians(90.f), 0.f, 0.f)},
					{ "Static_AccioActivity_Platform_Resource", "./Resources/SampleClient/Models/Static/SM_SM_HW_AccioActivity_Platform.bin", XMMatrixScaling(600, 600, 600	) * XMMatrixRotationRollPitchYaw(XMConvertToRadians(90.f), 0.f, 0.f) },
					{ "Static_AccioActivity_Bumper_Resource", "./Resources/SampleClient/Models/Static/SM_SM_HW_AccioActivity_Bumper.bin", XMMatrixIdentity() },
					{ "Static_AccioActivity_BumperA_Resource", "./Resources/SampleClient/Models/Static/SM_SM_HW_AccioActivity_Bumper_A.bin", XMMatrixIdentity() },
					{ "Static_AccioActivity_RampLarge_Resource", "./Resources/SampleClient/Models/Static/SM_SM_HW_AccioActivity_RampLarge.bin", XMMatrixIdentity() },
					{ "Static_AccioActivity_RampSmall_Resource", "./Resources/SampleClient/Models/Static/SM_SM_HW_AccioActivity_RampSmall.bin", XMMatrixIdentity() },
				};

				for (const auto& entry : resources)
				{
					auto resource = CGameInstance::Get().AddResourceT<CResStaticModel>(
						LEVEL::TERRAIN, entry.pTag, CResStaticModel::Create(entry.pPath));
					if (!resource)
						return false;

					CResStaticModel::DESC desc{};
					desc.PreTransformMatrix = entry.PreTransformMatrix;
					if (FAILED(resource->Load(desc)))
						return false;
				}

				// [LSY] 학생 모델과 동일한 Skeleton에서 변환된 전용 애니메이션만 사용한다.
				// 다른 여성 모델의 공용 Clip은 본 매핑과 루트 축 차이로 자세가 깨질 수 있다.
				if (auto studentModel = CGameInstance::Get().AddResourceT<CResModel>(
					LEVEL::TERRAIN,
					"ACCIO_ACTIVITY_STUDENT_MODEL_RESOURCE",
					CResModel::Create(
						"./Resources/SampleClient/Models/Skeleton/"
						"ElegantStudent_PrettyGirl2_RigCorrectedFinal/"
						"SK_ElegantStudent_PrettyGirl2_RigCorrectedFinal.bin")))
				{
					CResModel::DESC desc{};
					desc.PreTransformMatrix =
						XMMatrixScaling(3.f, 3.f, 3.f) *
						XMMatrixRotationY(XMConvertToRadians(180.f)) *
						XMMatrixTranslation(0.f, -1.8f, 0.f);
					if (FAILED(studentModel->Load(desc)))
						return false;
				}
				else
				{
					return false;
				}

				if (FAILED(CGameInstance::Get().AddPrototype(
					LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_AccioActivity_Base,
					CAccioActivity_Base::Create())) ||
					FAILED(CGameInstance::Get().AddPrototype(
						LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_AccioActivity_Platform,
						CAccioActivity_Platform::Create())) ||
					FAILED(CGameInstance::Get().AddPrototype(
						LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_AccioActivity_BumperA,
						CAccioActivity_BumperA::Create())) ||
					FAILED(CGameInstance::Get().AddPrototype(
						LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_AccioActivity_BumperB,
						CAccioActivity_BumperB::Create())) ||
					FAILED(CGameInstance::Get().AddPrototype(
						LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_AccioActivity_RampLarge,
						CAccioActivity_RampLarge::Create())) ||
					FAILED(CGameInstance::Get().AddPrototype(
						LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_AccioActivity_LampSmall,
						CAccioActivity_LampSmall::Create())) ||
					FAILED(CGameInstance::Get().AddPrototype(
						LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_AccioActivity_NpcController,
						CAccioActivity_NpcController::Create())) ||
					FAILED(CGameInstance::Get().AddPrototype(
						LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_AccioActivity_NpcCharacter,
						CAccioActivity_NpcCharacter::Create())))
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
						MSG_BOX("TERRAIN Failed Static_Oil	Resource");
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

			// [LSY] D6 물리 문의 모델 원점을 문짝 중심으로 옮기고
			// 원본 센티미터 단위를 엔진 월드 단위로 변환한다.
			{
				auto resource = CGameInstance::Get().AddResourceT<CResStaticModel>(
					LEVEL::TERRAIN,
					"Static_PhysicsDoor_Resource",
					CResStaticModel::Create(
						"./Resources/SampleClient/Models/Static/LCJ_ObjecMap/"
						"SM_BP_Door_Template64_1295.bin"));
				if (!resource)
					return false;

				CResStaticModel::DESC desc{};
				desc.PreTransformMatrix =
					XMMatrixScaling(0.03f, 0.03f, 0.03f) *
					XMMatrixTranslation(-1.875f, -3.73479f, 0.031791f);
				if (FAILED(resource->Load(desc)))
					return false;

				if (FAILED(CGameInstance::Get().AddPrototype(
					LEVEL::TERRAIN,
					PROTO_GAMEOBJECT::Prototype_GameObject_PhysicsDoor,
					CPhysicsDoor::Create())))
				{
					MSG_BOX("TERRAIN Failed Prototype_GameObject_PhysicsDoor");
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

			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::TERRAIN,
				PROTO_GAMEOBJECT::Prototype_GameObject_TestPathPlayback,
				CTestPathPlaybackObject::Create())))
			{
				MSG_BOX("TERRAIN Failed Prototype_GameObject_TestPathPlayback");
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::TERRAIN,
				PROTO_GAMEOBJECT::Prototype_GameObject_LuaTest,
				CLuaTestObject::Create())))
			{
				MSG_BOX("TERRAIN Failed Prototype_GameObject_LuaTest");
				return false;
			}

			{
				constexpr char CAPE_MODEL_PATH[] =
					"./Resources/SampleClient/Models/Skeleton/clothes/SK_clothes.bin";
				const _matrix CapePreTransform =
					XMMatrixScaling(3.f, 3.f, 3.f) *
					XMMatrixRotationY(
						XMConvertToRadians(180.f)) *
					XMMatrixTranslation(0.f, -1.5f, 0.f);

				if (auto res =
					CGameInstance::Get().
					AddResourceT<CResModel>(
						LEVEL::TERRAIN,
						"PLAYER_CAPE_MODEL_RESOURCE",
						CResModel::Create(
							CAPE_MODEL_PATH)))
				{
					CResModel::DESC Desc{};
					Desc.PreTransformMatrix =
						CapePreTransform;
					if (FAILED(res->Load(Desc)))
					{
						MSG_BOX(
							"TERRAIN Failed PLAYER_CAPE_MODEL_RESOURCE");
						return false;
					}
				}

				if (auto res =
					CGameInstance::Get().
					AddResourceT<CResNvClothMesh>(
						LEVEL::TERRAIN,
						"PLAYER_CAPE_CLOTH_RESOURCE",
						CResNvClothMesh::Create(
							CAPE_MODEL_PATH)))
				{
					CResNvClothMesh::DESC Desc{};
					Desc.PreTransformMatrix =
						CapePreTransform;
					Desc.sSimulationAnchorBone =
						"Spine3";
					Desc.iSimulationMeshIndex = 0;
					Desc.iRenderMeshIndex = 1;
					Desc.fWeldTolerance = 1.e-5f;
					Desc.fFixedTopRatio = 0.1f;
					if (FAILED(res->Load(Desc)))
					{
						MSG_BOX(
							"TERRAIN Failed PLAYER_CAPE_CLOTH_RESOURCE");
						return false;
					}
				}

				if (FAILED(
					E::CGameInstance::Get().
					AddPrototype(
						LEVEL::TERRAIN,
						PROTO_GAMEOBJECT::
							Prototype_GameObject_NvClothCape,
						CNvClothCape::Create())))
				{
					MSG_BOX(
						"TERRAIN Failed Prototype_GameObject_NvClothCape");
					return false;
				}
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


			if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(LEVEL::TERRAIN, "PLAYER_MODEL_RESROUCE", CResModel::Create("./Resources/SampleClient/Models/Skeleton/professor/SK_professor.bin"))) {

				E::CResModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixScaling(3.f, 3.f, 3.f) * XMMatrixRotationY(XMConvertToRadians(180.f)) * XMMatrixTranslation(0.f, -1.8f, 0.f);
				if (FAILED(res->Load(pDesc))) {
					MSG_BOX("TERRAIN Failed PLAYER_MODEL_RESROUCE");
					return false;
				}
			}
		
			if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>(LEVEL::TERRAIN, "PLAYER_WEAPON_RESROUCE", CResStaticModel::Create("./Resources/SampleClient/Models/Static/SM_Wand.bin"))) {

				E::CResStaticModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixRotationX(XMConvertToRadians(-90.f)) * XMMatrixScaling(1.f, 1.f, 1.f);
				if (FAILED(res->Load(pDesc))) {
					MSG_BOX("TERRAIN Failed PLAYER_WEAPON_RESROUCE");
					return false;
				}
			}
			if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(LEVEL::TERRAIN, "PLAYER_BROOM_RESOURCE", CResModel::Create("./Resources/SampleClient/Models/Skeleton/professor/Broom/SK_FlyingClassBroom_01.bin"))) {
				E::CResModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixIdentity();
				if (FAILED(res->Load(pDesc))) {
					MSG_BOX("TERRAIN Failed PLAYER_BROOM_RESOURCE");
					return false;
				}
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_Player, CPlayer::Create())))
			{
				MSG_BOX("TERRAIN Failed Prototype_GameObject_Player");
				return false;
			}

			if (auto resource = CGameInstance::Get().AddResourceT<CResStaticModel>(
				LEVEL::TERRAIN, "Static_WiggenweldPotion_Resource",
				CResStaticModel::Create("./Resources/SampleClient/Models/Static/Potion_Wiggenweld/SM_Potion_Wiggenweld.bin")))
			{
				CResStaticModel::DESC desc{};
				desc.PreTransformMatrix = XMMatrixScaling(2.f, 2.f, 2.f);
				if (FAILED(resource->Load(desc))) return false;
			}
			else return false;
			if (FAILED(CGameInstance::Get().AddPrototype(
				LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_WiggenweldPotion,
				CWiggenweldPotion::Create()))) return false;

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
			if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_PlayerBroom, CPlayer_Broom::Create())))
			{
				MSG_BOX("TERRAIN Failed Prototype_GameObject_PlayerBroom");
				return false;
			}
			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_PlayerMagicBullet, CPlayer_Magic_Bullet::Create())))
			{
				MSG_BOX("TERRAIN Failed Prototype_GameObject_PlayerMagicBullet");
				return false;
			}
			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::TERRAIN,
				PROTO_GAMEOBJECT::Prototype_GameObject_PlayerConfringoBullet,
				CPlayer_Confringo_Bullet::Create())))
			{
				MSG_BOX("TERRAIN Failed Prototype_GameObject_PlayerConfringoBullet");
				return false;
			}
			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::TERRAIN,
				PROTO_GAMEOBJECT::Prototype_GameObject_PlayerBombardaBullet,
				CPlayer_Bombarda_Bullet::Create())))
			{
				MSG_BOX("TERRAIN Failed Prototype_GameObject_PlayerBombardaBullet");
				return false;
			}
			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::TERRAIN,
				PROTO_GAMEOBJECT::Prototype_GameObject_PlayerStupefyBullet,
				CPlayer_Stupefy_Bullet::Create())))
				return false;
			MonsterLoad_InWorker();
			// 워커 스레드 종료
			return  true;
		});
}

std::future<bool> CLevelTerrainLoader::UnLoad()
{
	LOG_MEMORY("start");
	E::CGameInstance::Get().ClearAllRunningEffect();
	LOG_MEMORY("end");
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("UNLOADING_TERRAIN", []()
		{
			CGameInstance::Get().DelPrototype(LEVEL::TERRAIN);
			CGameInstance::Get().DelResource(LEVEL::TERRAIN);

			return true;
		});
}

_bool CLevelTerrainLoader::UILoad()
{
	/**********************UI********************/
	{
		{
			const char* targetDirectories[] = {
				"./Resources/SampleClient/Textures/UI/UITexture/PlayScreen",
				"./Resources/SampleClient/Textures/UI/UITexture/SpellSlot",
				"./Resources/SampleClient/Textures/UI/UITexture/DeadScene",
				"./Resources/SampleClient/Textures/UI/UITexture/Cursor",
				"./Resources/SampleClient/Textures/UI/UITexture/SpellMiniGame",
				"./Resources/SampleClient/Textures/UI/UITexture/MiniGame",
				"./Resources/SampleClient/Textures/UI/FlipBook"
			};

			for (const auto& targetDir : targetDirectories)
				UITextureResourceLoader::LoadDirectory(
					"LEVEL_TERRAIN", targetDir);
		}

		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TERRAIN", "Prototype_GameObject_TextureUI", CTextureUI::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TERRAIN", "Prototype_GameObject_EffectUI", CEffectUI::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TERRAIN", "Prototype_GameObject_TextBox", CTextBox::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TERRAIN", "Prototype_GameObject_Button", CButton::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TERRAIN", "Prototype_GameObject_GeneralButton", CGeneralButton::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TERRAIN", "Prototype_GameObject_SpellMeter", CSpellMeter::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TERRAIN", "Prototype_GameObject_HPBar", CHPBar::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TERRAIN", "Prototype_GameObject_MiniMap", CMiniMap::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TERRAIN", "Prototype_GameObject_UIController", CUIController::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TERRAIN", "Prototype_GameObject_SpellMiniGame", CSpellMiniGame::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TERRAIN", "Prototype_GameObject_GameOverMask", CGameOverMask::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TERRAIN", "Prototype_GameObject_VideoObject", CVideoObject::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TERRAIN", "Prototype_GameObject_Cursor", CCursor::Create())))
		{
			return false;
		}
	}
	return true;
}
HRESULT CLevelTerrainLoader::MonsterLoad_InWorker()
{
	//TombBos
	{
		if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(LEVEL::TERRAIN, "Model_Resource_TombBoss",
			CResModel::Create("./Resources/SampleClient/Models/Skeleton/Tomb_Protector/SK_Tomb_Protector.bin")))
		{
			E::CResModel::DESC pDesc{};
			pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixRotationY(XMConvertToRadians(180.f));
			if (FAILED(res->Load(pDesc)))
			{
				MSG_BOX("LEVEL_CREATURE Failed Model_Resource_TombBoss");
				return E_FAIL;
			}
		}

		if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_BossTMB, CBossTMB::Create())))
		{
			MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_BossTMB");
			return E_FAIL;
		}

		if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>(LEVEL::TERRAIN, "Model_Resource_BossWeapon",
			CResStaticModel::Create("./Resources/SampleClient/Models/Static/SM_BossWeapon.bin")))
		{
			E::CResStaticModel::DESC pDesc{};
			pDesc.PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f);

			if (FAILED(res->Load(pDesc)))
			{
				MSG_BOX("BOSS_CHARLES_ROOKWOOD Failed Static_Model_Resource_BossWeapon");
				return E_FAIL;
			}
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_BossWeapon, CBossMace::Create())))
		{
			MSG_BOX("BOSS_CHARLES_ROOKWOOD Failed Prototype_GameObject_BossWeapon");
			return E_FAIL;
		}
	}
	//BOSSSTAR
	{
		if (FAILED(E::CGameInstance::Get().AddPrototype(
			LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_BossStarBurst, CBoss_StarBurst::Create())))
		{
			MSG_BOX("TERRAIN Failed Prototype_GameObject_BossStarBurst");
			return E_FAIL;
		}
	}
	//BOSSBULLET
	{
		if (FAILED(E::CGameInstance::Get().AddPrototype(
			LEVEL::TERRAIN,
			PROTO_GAMEOBJECT::Prototype_GameObject_TombBossBullet, CTombBossBullet::Create())))
		{
			MSG_BOX("TERRAIN Failed Prototype_GameObject_TombBossBullet");
			return E_FAIL;
		}
	}
	//TombGurDian
	{
		if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(LEVEL::TERRAIN, "Model_Resource_TMBGurdian",
			CResModel::Create("./Resources/SampleClient/Models/Skeleton/Tomb_Grunt/SK_Tomb_Grunt.bin")))
		{
			E::CResModel::DESC pDesc{};
			pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixRotationY(XMConvertToRadians(180.f));
			if (FAILED(res->Load(pDesc)))
			{
				MSG_BOX("LEVEL_CREATURE Failed Model_Resource_TMBGurdian");
				return E_FAIL;
			}
		}

		if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_TMBGurdian, CTmbGurdian::Create())))
		{
			MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_TMBGurdian");
			return E_FAIL;
		}

	}
	//TombGurDianDead
	{
		for (uint32_t i = 0; i < 13; ++i)
		{
			std::string path = "./Resources/SampleClient/Models/Static/SM_Med_" + std::to_string(i) + ".bin";
			StringID resTag = "Static_Med_Debris_" + std::to_string(i);
			if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>(LEVEL::TERRAIN, resTag,
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
			LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_TmbGurdianDead, CTmbGurdianDead::Create())))
		{
			MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_TmbGurdianDead");
			return E_FAIL;
		}
	}
	//TombWeapon
	{
		if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>(LEVEL::TERRAIN, "Model_Resource_Mace",
			CResStaticModel::Create("./Resources/SampleClient/Models/Static/SM_Tomb_Mace.bin"))) {

			E::CResStaticModel::DESC pDesc{};
			pDesc.PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f);

			if (FAILED(res->Load(pDesc)))
			{
				MSG_BOX("LEVEL_CREATURE Failed Static_Mace_Model_Resource");
				return E_FAIL;
			}
		}
		if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>(LEVEL::TERRAIN, "Model_Resource_Sword",
			CResStaticModel::Create("./Resources/SampleClient/Models/Static/SM_Tomb_Sword.bin"))) {

			E::CResStaticModel::DESC pDesc{};
			pDesc.PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f);

			if (FAILED(res->Load(pDesc)))
			{
				MSG_BOX("LEVEL_CREATURE Failed Static_Sword_Model_Resource");
				return E_FAIL;
			}
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_Mace, CGurdianWeapon::Create())))
		{
			MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_Mace");
			return E_FAIL;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_Sword, CGurdianWeapon::Create())))
		{
			MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_Sword");
			return E_FAIL;
		}

	}
	/*----------- 광윤 추가 -----------*/
	if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(LEVEL::TERRAIN, "Model_Resource_Dragon_BoneModel",
		CResModel::Create("./Resources/SampleClient/Models/Skeleton/Dragon/SK_Dragon_BoneMesh.bin"))) {

		E::CResModel::DESC pDesc{};
		pDesc.PreTransformMatrix = XMMatrixScaling(1.6f, 1.6f, 1.6f) * XMMatrixRotationY(XMConvertToRadians(180.f));

		if (FAILED(res->Load(pDesc)))
		{
			MSG_BOX("LAST_BOSS_RANROK Failed Model_Resource_Dragon_BoneModel");
			return E_FAIL;
		}
	}
	/*---------------------------------*/
	//Dragon	
	{ 
		if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(LEVEL::TERRAIN, "Model_Resource_Dragon",
			CResModel::Create("./Resources/SampleClient/Models/Skeleton/Dragon/SK_Dragon.bin"))) {
		
			E::CResModel::DESC pDesc{};
			pDesc.PreTransformMatrix = XMMatrixScaling(1.6f, 1.6f, 1.6f) * XMMatrixRotationY(XMConvertToRadians(180.f));
		
			if (FAILED(res->Load(pDesc)))
			{
				MSG_BOX("TERRAIN Failed Model_Resource_Dragon");
				return E_FAIL;
			}
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_Dragon, CEnderDragon::Create())))
		{
			MSG_BOX("TERRAIN Failed Prototype_GameObject_Dragon");
			return E_FAIL;
		}
		if (FAILED(CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, "Prototype_Component_Dragon_FSM",CEnderDragon_State::Create()))) return E_FAIL;
		if (FAILED(CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_Dragon_FireBall, CEdgFireBall::Create()))) return E_FAIL;
		if (FAILED(CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_Dragon_Breath, CEdgBreath::Create()))) return E_FAIL;
		if (FAILED(CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_Dragon_Pulse, CEdgPulse::Create()))) return E_FAIL;
		if (FAILED(CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_Dragon_RandomBall, CEdgRandomBall::Create()))) return E_FAIL;
		if (FAILED(CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_Dragon_Gasi, CEdgGasi::Create()))) return E_FAIL;

	}
	//Spider
	{
		if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(LEVEL::TERRAIN, "Model_Resource_Spider",
			CResModel::Create("./Resources/SampleClient/Models/Skeleton/Spider/SK_Spider.bin"))) {

			E::CResModel::DESC pDesc{};
			pDesc.PreTransformMatrix = XMMatrixScaling(6.f, 6.f, 6.f) * XMMatrixRotationY(XMConvertToRadians(180.f));

			if (FAILED(res->Load(pDesc)))
			{
				MSG_BOX("TERRAIN Failed Model_Resource_Spider");
				return E_FAIL;
			}
		}
		struct NPC_MODEL_ENTRY { const char* pTag; const char* pCharacter; };
		static constexpr NPC_MODEL_ENTRY NpcModels[] =
		{
			// TERRAIN NPC 검증용으로 검증 완료된 남성 NPC 모델만 로드한다.
			// NPC 전체 선로드는 각 모델 폴더의 모든 AN_ 클립까지 메모리에 올리므로 비활성화한다.
			/*{ "Model_Resource_NPC_VictorRookwood", "AesopSharp" },*/
			{ "Model_Resource_NPC_AlbieWeekes", "AlbieWeekes" },
			{ "Model_Resource_NPC_AugustusHill", "AugustusHill" },
			{ "Model_Resource_NPC_CrispinDunn", "CrispinDunn" },
			{ "Model_Resource_NPC_LeopoldBabcocke", "LeopoldBabcocke" },
			/*{ "Model_Resource_NPC_AnneSallow", "AnneSallow" },
			{ "Model_Resource_NPC_EffieBones", "EffieBones" },
			{ "Model_Resource_NPC_EleazarFig", "EleazarFig" },
			{ "Model_Resource_NPC_GladwinMoon", "GladwinMoon" },
			{ "Model_Resource_NPC_HelenThistlewood", "HelenThistlewood" },
			{ "Model_Resource_NPC_JasperTrout", "JasperTrout" },
			{ "Model_Resource_NPC_LeonaPeck", "LeonaPeck" },
			{ "Model_Resource_NPC_NoreenBlainey", "NoreenBlainey" },
			{ "Model_Resource_NPC_PadraicHaggarty", "PadraicHaggarty" },
			{ "Model_Resource_NPC_PhineasBlack", "PhineasBlack" },
			{ "Model_Resource_NPC_SironaRyan", "SironaRyan" },
			{ "Model_Resource_NPC_ThomasBrown", "ThomasBrown" },
			{ "Model_Resource_NPC_TimothyTeasdale", "TimothyTeasdale" },*/
		};
		for (const auto& Entry : NpcModels)
		{
			const _string ModelPath = "./Resources/SampleClient/Models/Skeleton/NPC_" + _string(Entry.pCharacter) +
				"/SK_NPC_" + Entry.pCharacter + ".bin";
			if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(
				LEVEL::TERRAIN, Entry.pTag, CResModel::Create(ModelPath)))
			{
				E::CResModel::DESC Desc{};
				Desc.PreTransformMatrix = XMMatrixScaling(3.f, 3.f, 3.f) *
					XMMatrixRotationY(XMConvertToRadians(180.f)) * XMMatrixTranslation(0.f, 2.f, 0.f);
				if (FAILED(res->Load(Desc)))
				{
					MSG_BOX("TERRAIN Failed NPC model resource");
					return E_FAIL;
				}
			}
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_Spider, CSpider::Create())))
		{
			MSG_BOX("TERRAIN Failed Prototype_GameObject_Spider");
			return E_FAIL;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype(
			LEVEL::TERRAIN,
			PROTO_GAMEOBJECT::Prototype_GameObject_WorldNpc,
			CWorldNpc::Create())))
		{
			MSG_BOX("TERRAIN Failed Prototype_GameObject_WorldNpc");
			return E_FAIL;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype(
			LEVEL::TERRAIN,
			PROTO_GAMEOBJECT::Prototype_GameObject_MiniGameNpc,
			CInteractiveNpc::Create())))
		{
			MSG_BOX("TERRAIN Failed Prototype_GameObject_MiniGameNpc");
			return E_FAIL;
		}
		if (FAILED(CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, "Prototype_Component_Mon_FSM", CMon_State::Create()))) return E_FAIL;
		
		if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_MonSpawner, CMon_Spawner::Create())))
		{
			MSG_BOX("TERRAIN Failed Prototype_GameObject_Spawner");
			return E_FAIL;
		}
	}

	//Coin
	{
		if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_Coin, CCoin::Create())))
		{
			MSG_BOX("TERRAIN Failed Prototype_GameObject_Coin");
			return E_FAIL;
		}
	}

	//PX_COLLISION_PROXY_PROTOTYPE_GROUP
	{
		if (FAILED(E::CGameInstance::Get().AddPrototype(PX_COLLISION_PROXY_PROTOTYPE_GROUP, PROTO_GAMEOBJECT::Prototype_GameObject_Coin, CCoin::Create())))
		{
			MSG_BOX("TERRAIN Failed Prototype_GameObject_Coin");
			return E_FAIL;
		}
	}
	return S_OK;
}
