#include "pch.h"
#include "ParticleShaderCache.h"
#include "GameInstance.h"
#include "ResPixelShader.h"
#include "ResVertexShader.h"

NS_BEGIN(Engine)

namespace
{
	constexpr const char* PARTICLE_PS_VARIANT_GROUP = "INTERNAL_PARTICLE_PS_VARIANT";
	constexpr const char* PARTICLE_VS_VARIANT_GROUP = "INTERNAL_PARTICLE_VS_VARIANT";
}
SPtr<CResPixelShader> CParticleShaderCache::GetPixelShader(const StringID& groupID, const StringID& shaderID, const std::string& entryPoint)
{
	if (entryPoint.empty())
		return nullptr;
	const std::string key = std::string(groupID.GetDbgStr()) + "::" + shaderID.GetDbgStr() + "::" + entryPoint + "::ps_5_0";

	auto cachedShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(PARTICLE_PS_VARIANT_GROUP, key);

	if (cachedShader)
		return cachedShader;

	const auto baseShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(groupID, shaderID);

	if (!baseShader)
		return nullptr;

	auto variant = CResPixelShader::Create(baseShader->GetPath());

	if (!variant)
		return nullptr;

	auto addedShader = CGameInstance::Get().AddResourceT<CResPixelShader>(PARTICLE_PS_VARIANT_GROUP, key, variant);

	if (!addedShader)
	{
		return CGameInstance::Get().GetResourceFirst<CResPixelShader>(PARTICLE_PS_VARIANT_GROUP, key);
	}

	if (FAILED(addedShader->Load(CResShader::DESC{
		.sEntryPoint = entryPoint,
		.sTarget = "ps_5_0"
		})))
	{
		return nullptr;
	}

	return addedShader;
}

SPtr<CResVertexShader> CParticleShaderCache::GetVertexShader(const StringID& groupID, const StringID& shaderID, const std::string& entryPoint)
{
	if (entryPoint.empty())
		return nullptr;
	const std::string key = std::string(groupID.GetDbgStr()) + "::" + shaderID.GetDbgStr() + "::" + entryPoint + "::vs_5_0";

	auto cachedShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(PARTICLE_VS_VARIANT_GROUP, key);

	if (cachedShader)
		return cachedShader;

	const auto baseShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(groupID, shaderID);

	if (!baseShader)
		return nullptr;

	auto variant = CResVertexShader::Create(baseShader->GetPath());

	if (!variant)
		return nullptr;

	auto addedShader = CGameInstance::Get().AddResourceT<CResVertexShader>(PARTICLE_VS_VARIANT_GROUP, key, variant);

	if (!addedShader)
	{
		return CGameInstance::Get().GetResourceFirst<CResVertexShader>(PARTICLE_VS_VARIANT_GROUP, key);
	}

	if (FAILED(addedShader->Load(CResShader::DESC{
		.sEntryPoint = entryPoint,
		.sTarget = "vs_5_0"
		})))
	{
		return nullptr;
	}

	return addedShader;
}

NS_END
