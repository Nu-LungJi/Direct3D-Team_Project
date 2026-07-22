#pragma once

#include "Engine_Defines.h"
#include "Engine_ParticleDefines.h"
#include "Handle.h"
#include "SoundManager.h"

NS_BEGIN(Engine)

using EFFECT_INSTANCE_ID = uint32_t;

inline constexpr EFFECT_INSTANCE_ID INVALID_EFFECT_INSTANCE_ID = 0;

enum class EFFECT_COMMAND_TYPE
{
	LIGHT,
	SOUND,
	END
};

enum class EFFECT_LIGHT_TYPE
{
	POINT,
	SPOT,
	END
};

struct EFFECT_LIGHT_COMMAND
{
	std::string sCommandName;
	std::string sLightPresetName;


	EFFECT_LIGHT_TYPE eType = EFFECT_LIGHT_TYPE::POINT;

	_float3 vLocalPosition{};
	_float3 vDirection{ 0.f, -1.f, 0.f };
	_float3 vColor{ 1.f, 1.f, 1.f };
	_float fIntensity = 1.f;
	_float fRange = 10.f;
	_float fSpawnDelay = 0.f;
	_float fDuration = 1.f;
};
struct EFFECT_PARTICLE_COMMAND
{
	std::string sCommandName;
	std::string sParticleJson;
	_float fSpawnDelay = 0.f;
};
struct EFFECT_SOUND_COMMAND
{
	std::string sCommandName;
	_string sSoundPath;
	_float3 vLocalPosition{};
	_float fVolume = 1.f;
	_float fSpawnDelay = 0.f;
};


using EffectCommandVariant = std::variant<EFFECT_LIGHT_COMMAND,EFFECT_SOUND_COMMAND>;

struct EFFECT_COMMAND
{
	EFFECT_COMMAND_TYPE eType = EFFECT_COMMAND_TYPE::END;;
	EffectCommandVariant data{};

	_float fSpawnDelay = 0.f;
};

struct EFFECT_PRESET
{
	std::string sEffectName;
	_float fDuration = 0.f;
	EFFECT_PARTICLE_COMMAND particleCommand;
	std::vector<EFFECT_COMMAND> vecCommands;
};

struct EFFECT_INSTANCE
{
	EFFECT_INSTANCE_ID iEffectId =INVALID_EFFECT_INSTANCE_ID;
	uint32_t iParticleOwnerId = INVALID_PARTICLE_OWNER_ID;

	std::vector<CHandle> vecLightHandles;
	std::vector<SOUND_ID> vecSoundIds;

	const EFFECT_PRESET* pPreset = nullptr;

	_float4x4 matWorld{};
	_float4 vEndPosition{};
	_float fElapsed = 0.f;
	_float fDuration = 0.f;
	size_t iNextCommandIndex = 0;
};
struct EFFECT_PENDING_COMMAND
{
	EFFECT_INSTANCE_ID iEffectId = INVALID_EFFECT_INSTANCE_ID;
	EFFECT_COMMAND command;
	_float fRemainingTime = 0.f;
};

NS_END
