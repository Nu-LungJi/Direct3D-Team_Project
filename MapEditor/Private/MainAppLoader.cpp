#include "pch.h"

#include "MainAppLoader.h"
#include "GameInstance.h"
#include "LevelLoading.h"
#include "Resources.h"

NS_USING(Client)

HRESULT CMainAppLoader::Load()
{
	// 터레인 띄우려고 SampleClient에서 복붙해온 셰이더
	{
		if (auto res = CGameInstance::Get().AddResource(
			"MAP_EDITOR_SHADER",
			"VS_MAP_PICKING",
			CResVertexShader::Create("./ShaderFiles/MapPicking.hlsl")))
		{
			if (FAILED(res->Load()))
				return E_FAIL;
		}

		if (auto res = CGameInstance::Get().AddResource(
			"MAP_EDITOR_SHADER",
			"PS_MAP_PICKING",
			CResPixelShader::Create("./ShaderFiles/MapPicking.hlsl")))
		{
			if (FAILED(res->Load()))
				return E_FAIL;
		}

		if (auto res = CGameInstance::Get().AddResource(
			"MAP_EDITOR_SHADER",
			"VS_TERRAIN_PICKING",
			CResVertexShader::Create("./ShaderFiles/TerrainPicking.hlsl")))
		{
			if (FAILED(res->Load()))
				return E_FAIL;
		}

		if (auto res = CGameInstance::Get().AddResource(
			"MAP_EDITOR_SHADER",
			"PS_TERRAIN_PICKING",
			CResPixelShader::Create("./ShaderFiles/TerrainPicking.hlsl")))
		{
			if (FAILED(res->Load()))
				return E_FAIL;
		}
	}

	return S_OK;
}
