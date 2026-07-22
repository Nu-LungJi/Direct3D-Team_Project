#include "pch.h"
#include "EffectManager.h"

#include "ParticleManager.h"
#include "LightManager.h"
#include "SoundManager.h"

NS_USING(Engine)

CEffectManager::CEffectManager(CParticleManager* pParticleManager,CLightManager* pLightManager,CSoundManager* pSoundManager)
	: m_pParticleManager(pParticleManager)
	, m_pLightManager(pLightManager)
	, m_pSoundManager(pSoundManager)
{
}

CEffectManager::~CEffectManager()
{
}
void CEffectManager::UpdateGUI()
{
	ImGui::Begin("Effect Manager");
	ImGui::End();
}
void CEffectManager::Update(_float fTimeDelta)
{
	for (auto& [effectId, instance] : m_Instances)
	{
		instance.fElapsed += fTimeDelta;
		DispatchReadyCommands(instance);
	}
	RemoveFinishedInstances();
}
HRESULT CEffectManager::AddPreset(EFFECT_PRESET preset)
{
	if (preset.sEffectName.empty())
		return E_FAIL;

	m_Presets[preset.sEffectName] = std::move(preset);

	return S_OK;
}

EFFECT_INSTANCE_ID CEffectManager::Spawn(const std::string& sEffectName,const _float4x4& matWorld,_fvector vEndPosition)
{
	auto presetIter = m_Presets.find(sEffectName);

	if (presetIter == m_Presets.end())
		return INVALID_EFFECT_INSTANCE_ID;

	EFFECT_INSTANCE instance{};

	instance.iEffectId = m_iNextEffectId++;
	instance.pPreset = &presetIter->second;
	instance.matWorld = matWorld;
	instance.fDuration = presetIter->second.fDuration;

	XMStoreFloat4(&instance.vEndPosition,vEndPosition);

	const EFFECT_INSTANCE_ID effectId = instance.iEffectId;

	m_Instances.emplace(effectId,std::move(instance));

	return effectId;
}
HRESULT CEffectManager::SaveEffectPreset(const std::string& strPath,const EFFECT_PRESET& preset)
{
	nlohmann::json json;

	json["effectName"] = preset.sEffectName;
	json["duration"] = preset.fDuration;

	json["particle"] = {
		{
			"commandName",
			preset.particleCommand.sCommandName
		},
		{
			"particleJson",
			preset.particleCommand.sParticleJson
		},
		{
			"spawnDelay",
			preset.particleCommand.fSpawnDelay
		}
	};

	json["commands"] =
		nlohmann::json::array();

	for (const EFFECT_COMMAND& command :
		preset.vecCommands)
	{
		if (command.eType ==
			EFFECT_COMMAND_TYPE::LIGHT)
		{
			const auto& light =
				std::get<EFFECT_LIGHT_COMMAND>(
					command.data);

			json["commands"].push_back({
				{ "type", "LIGHT" },
				{ "commandName", light.sCommandName },

				{
					"lightType",
					light.eType == EFFECT_LIGHT_TYPE::POINT
					? "POINT"
					: "SPOT"
				},

				{
					"localPosition",
					{
						light.vLocalPosition.x,
						light.vLocalPosition.y,
						light.vLocalPosition.z
					}
				},

				{
					"direction",
					{
						light.vDirection.x,
						light.vDirection.y,
						light.vDirection.z
					}
				},

				{
					"color",
					{
						light.vColor.x,
						light.vColor.y,
						light.vColor.z
					}
				},

				{ "intensity", light.fIntensity },
				{ "range", light.fRange },
				{ "spawnDelay", light.fSpawnDelay },
				{ "duration", light.fDuration }
				});
		}
		else if (command.eType == EFFECT_COMMAND_TYPE::SOUND)
		{
			const auto& sound =std::get<EFFECT_SOUND_COMMAND>(command.data);

			const std::string soundPath(sound.sSoundPath.begin(),sound.sSoundPath.end());

			json["commands"].push_back({
				{ "type", "SOUND" },
				{ "commandName", sound.sCommandName },
				{ "soundPath", soundPath },

				{
					"localPosition",
					{
						sound.vLocalPosition.x,
						sound.vLocalPosition.y,
						sound.vLocalPosition.z
					}
				},

				{ "volume", sound.fVolume },
				{ "spawnDelay", sound.fSpawnDelay },
				});
		}
	}

	std::ofstream file(strPath);

	if (!file.is_open())
		return E_FAIL;

	file << json.dump(4);

	return S_OK;
}
void CEffectManager::Stop(EFFECT_INSTANCE_ID iEffectId)
{
}
const EFFECT_INSTANCE* CEffectManager::FindInstance(EFFECT_INSTANCE_ID iEffectId) const
{
	return nullptr;
}
_float3 CEffectManager::TransformPosition(const _float3& vLocalPosition, const _float4x4& matWorld) const
{
	return _float3();
}


void CEffectManager::DispatchReadyCommands(EFFECT_INSTANCE& instance)
{
	if (!instance.pPreset)
		return;

	const auto& commands = instance.pPreset->vecCommands;

	while (instance.iNextCommandIndex < commands.size())
	{
		const EFFECT_COMMAND& command =
			commands[instance.iNextCommandIndex];

		if (command.fSpawnDelay >instance.fElapsed)
		{
			break;
		}

		DispatchCommand(instance,command);

		++instance.iNextCommandIndex;
	}
}
void CEffectManager::DispatchCommand(EFFECT_INSTANCE& instance,const EFFECT_COMMAND& command)
{
	switch (command.eType)
	{

	case EFFECT_COMMAND_TYPE::LIGHT:
	{
		const auto& lightCommand = std::get<EFFECT_LIGHT_COMMAND>(command.data);

		DispatchLight(instance,lightCommand);

		break;
	}

	//case EFFECT_COMMAND_TYPE::SOUND:
	//{
	//	const auto& soundCommand = std::get<EFFECT_SOUND_COMMAND>(command.data);
	//
	//	DispatchSound(instance,soundCommand);
	//
	//	break;
	//}

	default:
		break;
	}
}
void CEffectManager::DispatchLight(EFFECT_INSTANCE& instance,const EFFECT_LIGHT_COMMAND& command)
{
	_float3 worldPosition{};

	XMStoreFloat3(&worldPosition,XMVector3TransformCoord(XMLoadFloat3(	&command.vLocalPosition),XMLoadFloat4x4(&instance.matWorld)));

	//auto lightHandle =
	//	m_pLightManager->SpawnEffectLight(
	//		command.eType,
	//		worldPosition,
	//		command.vDirection,
	//		command.vColor,
	//		command.fIntensity,
	//		command.fRange,
	//		command.fDuration);
	//
	//if (lightHandle)
	//{
	//	instance.vecLightHandles.push_back(
	//		*lightHandle);
	//}
}
void CEffectManager::RemoveFinishedInstances()
{
	for (auto iter = m_Instances.begin();iter != m_Instances.end();)
	{
		EFFECT_INSTANCE& instance = iter->second;

		//모든 커맨드가 디스패치 됐는지
		const bool allCommandsDispatched =
			instance.pPreset == nullptr ||
			instance.iNextCommandIndex >=
			instance.pPreset->vecCommands.size();
		// 모든 커맨드의 duration이 0이 되어서 자연사 했는지
		const bool durationFinished =
			instance.fDuration > 0.f &&
			instance.fElapsed >=
			instance.fDuration;

		if (allCommandsDispatched &&
			durationFinished)
		{
			// 실제 파티클·라이트·사운드는 종료하지 않는다.
			// 강제 종료를 위해 보관하던 기록만 제거한다.
			iter = m_Instances.erase(iter);
		}
		else
		{
			++iter;
		}
	}
}
UPtr<CEffectManager> CEffectManager::Create(CParticleManager* pParticleManager, CLightManager* pLightManager, CSoundManager* pSoundManager)
{
	return UPtr<CEffectManager>(new CEffectManager{ pParticleManager, pLightManager, pSoundManager });
}
