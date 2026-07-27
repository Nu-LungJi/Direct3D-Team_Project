#pragma once
#include "pch.h"
#include "Engine_Defines.h"
NS_BEGIN(Engine)

class CParticleShaderCache : public CEngineBase
{
public:
	SPtr<CResPixelShader> GetPixelShader(
		const StringID& groupID,
		const StringID& shaderID,
		const std::string& entryPoint);

	SPtr<CResVertexShader> GetVertexShader(
		const StringID& groupID,
		const StringID& shaderID,
		const std::string& entryPoint);



};
NS_END
