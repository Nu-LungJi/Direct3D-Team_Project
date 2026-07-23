#pragma once

#include "Engine_Defines.h"
#include "Engine_ParticleDefines.h"
#include "Handle.h"
#include "SoundManager.h"

NS_BEGIN(Engine)

using EFFECT_INSTANCE_ID = uint32_t;

inline constexpr EFFECT_INSTANCE_ID INVALID_EFFECT_INSTANCE_ID = 0;




struct EFFECT_PARTICLE_COMMAND
{
	std::string sCommandName;
	std::string sParticleJson;
	_float fSpawnDelay = 0.f;
};


struct EFFECT_LIGHT_COMMAND
{
	std::string sCommandName;
	std::string sLightPresetName;


	_float3 vLocalPosition{};
	_float3 vVelocity{ 0.f, -1.f, 0.f };
	_float3 vColor{ 1.f, 1.f, 1.f };
	_float fIntensity = 1.f;
	_float fRange = 10.f;
	_float fSpawnDelay = 0.f;
	_float fDuration = 1.f;
};


struct EFFECT_SOUND_COMMAND
{
	std::string sCommandName;
	_string sSoundPath;

	_float3 vLocalPosition{};

	_float fVolume = 1.f;
	_float fPitch = 1.f;

	_float fMinDistance = 1.f;
	_float fMaxDistance = 50.f;

	_float fSpawnDelay = 0.f;

	_bool bLoop = false;
	_bool b3D = true;
};

using EffectCommandVariant = std::variant<EFFECT_PARTICLE_COMMAND, EFFECT_LIGHT_COMMAND, EFFECT_SOUND_COMMAND>;


enum class EFFECT_COMMAND_TYPE
{
	PARTICLE,
	LIGHT,
	SOUND,
	END
};

struct EFFECT_COMMAND
{
	EFFECT_COMMAND_TYPE eType = EFFECT_COMMAND_TYPE::END;
	EffectCommandVariant data{};

	_float fSpawnDelay = 0.f;
};





struct EFFECT_PRESET
{
	std::string sEffectName;
	_float fDuration = 0.f;
	std::vector<EFFECT_COMMAND> vecCommands;
};
struct EFFECT_INSTANCE
{
	EFFECT_INSTANCE_ID iEffectId =INVALID_EFFECT_INSTANCE_ID;
	std::vector<uint32_t> vecParticleOwnerId;
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
