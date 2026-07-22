#include "pch.h"
#include "LevelPercivalLoader.h"

#include "GameInstance.h"
#include "BackGround.h"

NS_USING(Client)

namespace
{
	std::string MakeStaticModelResourceTag(const std::filesystem::path& rootPath, const std::filesystem::path& binPath)
	{
		std::filesystem::path relativePath = binPath.lexically_relative(rootPath);
		if (relativePath.empty())
		{
			relativePath = binPath.filename();
		}

		relativePath.replace_extension();

		std::string resourceTag = relativePath.string();
		for (char& ch : resourceTag)
		{
			const unsigned char value = static_cast<unsigned char>(ch);
			if (!std::isalnum(value))
			{
				ch = '_';
			}
		}

		return resourceTag;
	}
}

std::future<bool> CLevelPercivalLoader::Load()
{
	// 메인 스레드 시작

	const std::filesystem::path staticModelDir = /*E::PATH_MINSOO_FBX;*/ E::PATH_MAPEDITOR_STATIC_MODEL_DIR;
	if (!std::filesystem::exists(staticModelDir))
	{
		MSG_BOX("NO_STATIC_MODEL_DIR");
	}

	std::future<bool> result;
	for (const auto& entry : std::filesystem::recursive_directory_iterator(staticModelDir))
	{
		if (!entry.is_regular_file() || _stricmp(entry.path().extension().string().c_str(), ".bin") != 0)
		{
			continue;
		}

		result = E::CGameInstance::Get().WorkerEnqueueWithFuture("Loading_MapFast", [=]()
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
		);
	}
	

	// 메인 스레드 종료
	result =  E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_PERCIBAL", []()
		{
			// 워커 스레드
			
			// 워커 스레드 종료
			return  true;
		});

	return result;
}

std::future<bool> CLevelPercivalLoader::UnLoad()
{
	LOG_MEMORY("start");
	
	LOG_MEMORY("end");
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("UNLOADING_PERCIVAL", []()
		{
			return true;
		});
}
