#include "pch.h"
#include "LevelCharlesRookwoodLoader.h"

#include "GameInstance.h"
#include "BackGround.h"

NS_USING(Client)

std::future<bool> CLevelCharlesRookwoodLoader::Load()
{
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_CharlesRookwood", []()
		{
			_bool bWorkerReturn = true;

			// STEP: 맵 로딩
			{
				const std::filesystem::path staticModelDir = /*E::PATH_MINSOO_FBX;*/ E::PATH_MAPEDITOR_STATIC_MODEL_DIR;
				if (!std::filesystem::exists(staticModelDir))
				{
					MSG_BOX("NO_STATIC_MODEL_DIR");
				}
				std::list<std::future<bool>> result;

				for (const auto& entry : std::filesystem::recursive_directory_iterator(staticModelDir))
				{
					if (!entry.is_regular_file() || _stricmp(entry.path().extension().string().c_str(), ".bin") != 0)
					{
						continue;
					}
					result.emplace_back(E::CGameInstance::Get().WorkerEnqueueWithFuture("Loading_MapFast", [=]()
						{
							const std::string resourceTag = MakeStaticModelResourceTag(staticModelDir, entry.path());
							auto res = E::CGameInstance::Get().AddResourceT<E::CResStaticModel>(
								E::TAG_RES_GRP_MAPEDITOR_STATIC_MODEL,
								resourceTag,
								E::CResStaticModel::Create(entry.path().string()));

							if (!res)
							{
								return false;
							}

							E::CResStaticModel::DESC desc{};
							desc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f);

							if (FAILED(res->Load(desc)))
							{
								return false;
							}
						}
					));
				}

				for (auto& future : result)
				{
					if (!future.valid())
					{
						bWorkerReturn = false;
						continue;
					}

					try
					{
						if (!future.get())
							bWorkerReturn = false;
					}
					catch (const std::exception& e)
					{
						bWorkerReturn = false;
						DEBUG_LOG_STR(
							std::string("[Future] Exception: ") + e.what() + "\n");
					}
				}

				if (!bWorkerReturn)
				{
					return false;
				}
			}// END STEP: 맵 로딩

			return true;
		});
}

std::future<bool> CLevelCharlesRookwoodLoader::UnLoad()
{
	LOG_MEMORY("start");

	// 메인스레드 MAP해제
	E::CGameInstance::Get().ClearAllChunk();
	E::CGameInstance::Get().GetNavMeshManager()->Clear();
	
	LOG_MEMORY("end");
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("UNLOADING_CharlesRookwood", []()
		{
			// 워커스레드 MAP 해제
			E::CGameInstance::Get().DelPrototype("MAPEDITOR");
			E::CGameInstance::Get().DelResource("MAPEDITOR");   E::CGameInstance::Get().DelResource(TAG_RES_GRP_MAPEDITOR_STATIC_MODEL);

			return true;
		});
}
