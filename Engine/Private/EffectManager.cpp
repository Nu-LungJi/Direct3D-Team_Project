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

HRESULT CEffectManager::Initialize() {

	auto k = Load_FilePath_ByExtension("./Resources/json/Effect", ".json");
	if (FAILED(Load_EffectJsonPackage(k))){
		MSG_BOX("EFFECT LOAD FAILED");
		return E_FAIL;
	}


	return S_OK;
}

void CEffectManager::UpdateGUI()
{
	static EFFECT_PRESET preset{};
	static std::string savePath = "./Resources/json/Effect/NewEffect.json";
	static int removeCommandIndex = -1;
	static std::string effectName = "";
	static std::string playEffectName = "";

	static float Duration = 0;
	ImGui::Begin("Effect Manager");

	InputText("Effect Name", effectName);
	InputText("Save Path", savePath);
	ImGui::InputFloat("Effect Duration", &Duration);

	ImGui::Separator();

	if (ImGui::Button("Add Particle"))
	{
		EFFECT_COMMAND command{};
		command.eType = EFFECT_COMMAND_TYPE::PARTICLE;
		command.data = EFFECT_PARTICLE_COMMAND{};
		command.fSpawnDelay = 0.f;

		preset.vecCommands.push_back(std::move(command));
	}

	ImGui::SameLine();

	if (ImGui::Button("Add Light"))
	{
		EFFECT_COMMAND command{};
		command.eType = EFFECT_COMMAND_TYPE::LIGHT;
		command.data = EFFECT_LIGHT_COMMAND{};
		command.fSpawnDelay = 0.f;

		preset.vecCommands.push_back(std::move(command));
	}

	ImGui::SameLine();

	if (ImGui::Button("Add Sound"))
	{
		EFFECT_COMMAND command{};
		command.eType = EFFECT_COMMAND_TYPE::SOUND;
		command.data = EFFECT_SOUND_COMMAND{};
		command.fSpawnDelay = 0.f;

		preset.vecCommands.push_back(std::move(command));
	}

	ImGui::Separator();

	for (int i = 0; i < static_cast<int>(preset.vecCommands.size()); ++i)
	{
		EFFECT_COMMAND& command = preset.vecCommands[i];

		ImGui::PushID(i);

		switch (command.eType)
		{
		case EFFECT_COMMAND_TYPE::PARTICLE:
		{
			EFFECT_PARTICLE_COMMAND* particle =
				std::get_if<EFFECT_PARTICLE_COMMAND>(&command.data);

			if (!particle)
				break;

			if (ImGui::CollapsingHeader("Particle Command", ImGuiTreeNodeFlags_DefaultOpen))
			{
				InputText("Command Name", particle->sCommandName);
				InputText("Particle JSON File Name. Include .json", particle->sParticleJson);
				ImGui::InputFloat("Spawn Delay", &command.fSpawnDelay);

				if (ImGui::Button("Remove Particle"))
					removeCommandIndex = i;
			}

			break;
		}

		case EFFECT_COMMAND_TYPE::LIGHT:
		{
			EFFECT_LIGHT_COMMAND* light =
				std::get_if<EFFECT_LIGHT_COMMAND>(&command.data);

			if (!light)
				break;

			if (ImGui::CollapsingHeader("Light Command", ImGuiTreeNodeFlags_DefaultOpen))
			{
				InputText("Command Name", light->sCommandName);
				InputText("Light Preset Name", light->sLightPresetName);

				ImGui::InputFloat3("Local Position", &light->vLocalPosition.x);
				ImGui::InputFloat3("Velocity", &light->vVelocity.x);
				ImGui::ColorEdit3("Color", &light->vColor.x);

				ImGui::InputFloat("Intensity", &light->fIntensity);
				ImGui::InputFloat("Range", &light->fRange);
				ImGui::InputFloat("Duration", &light->fDuration);
				ImGui::InputFloat("Spawn Delay", &command.fSpawnDelay);

				if (ImGui::Button("Remove Light"))
					removeCommandIndex = i;
			}

			break;
		}

		case EFFECT_COMMAND_TYPE::SOUND:
		{
			EFFECT_SOUND_COMMAND* sound =
				std::get_if<EFFECT_SOUND_COMMAND>(&command.data);

			if (!sound)
				break;

			if (ImGui::CollapsingHeader("Sound Command", ImGuiTreeNodeFlags_DefaultOpen))
			{
				InputText("Command Name", sound->sCommandName);

				std::string soundPath(
					sound->sSoundPath.begin(),
					sound->sSoundPath.end());

				if (InputText("Sound Path", soundPath))
				{
					sound->sSoundPath = _string(
						soundPath.begin(),
						soundPath.end());
				}

				ImGui::InputFloat3("Local Position", &sound->vLocalPosition.x);
				ImGui::InputFloat("Volume", &sound->fVolume);
				ImGui::InputFloat("Min Distance", &sound->fMinDistance);
				ImGui::InputFloat("Max Distance", &sound->fMaxDistance);
				ImGui::InputFloat("Spawn Delay", &command.fSpawnDelay);

				ImGui::Checkbox("Loop", &sound->bLoop);

				if (ImGui::Button("Remove Sound"))
					removeCommandIndex = i;
			}

			break;
		}

		default:
			ImGui::Text("Invalid Effect Command");

			if (ImGui::Button("Remove Invalid Command"))
				removeCommandIndex = i;

			break;
		}

		ImGui::Separator();
		ImGui::PopID();
	}

	if (removeCommandIndex >= 0 &&
		removeCommandIndex < static_cast<int>(preset.vecCommands.size()))
	{
		preset.vecCommands.erase(
			preset.vecCommands.begin() + removeCommandIndex);

		removeCommandIndex = -1;
	}

	if (ImGui::Button("Save Effect Preset"))
	{
		preset.sEffectName = effectName;
		preset.fDuration = Duration;
		if (FAILED(SaveEffectPreset(savePath, preset)))
			MSG_BOX("Failed to save effect preset");
	}

	ImGui::SameLine();

	if (ImGui::Button("Clear Editor"))
		preset = EFFECT_PRESET{};

	ImGui::Text("Command Count: %zu", preset.vecCommands.size());
	ImGui::Text("Active Effects: %zu", m_Instances.size());


	ImGui::Separator();

	InputText("Load Effect Name", playEffectName);

	if (ImGui::Button("Play Effect"))
	{
		_float4x4 matWorld{};

		XMStoreFloat4x4(
			&matWorld,
			XMMatrixTranslation(5.f, 3.f, 5.f));

		PlayEffect(playEffectName, matWorld);
	}
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



HRESULT CEffectManager::SaveEffectPreset(const std::string& strPath,const EFFECT_PRESET& preset)
{
	if (strPath.empty() ||
		preset.sEffectName.empty())
	{
		return E_FAIL;
	}

	nlohmann::json json;

	json["effectName"] = preset.sEffectName;

	json["duration"] = preset.fDuration;

	json["commands"] = nlohmann::json::array();

	for (const EFFECT_COMMAND& command :preset.vecCommands)
	{
		switch (command.eType)
		{
		case EFFECT_COMMAND_TYPE::PARTICLE:
		{
			if (!std::holds_alternative<EFFECT_PARTICLE_COMMAND>(command.data))
			{
				return E_FAIL;
			}

			const auto& particle = std::get<EFFECT_PARTICLE_COMMAND>(
					command.data);

			json["commands"].push_back({
				{ "type", "PARTICLE" },
				{ "commandName",
					particle.sCommandName },
				{ "particleJson",
					particle.sParticleJson },
				{ "spawnDelay",
					command.fSpawnDelay }
				});

			break;
		}

		case EFFECT_COMMAND_TYPE::LIGHT:
		{
			if (!std::holds_alternative<
				EFFECT_LIGHT_COMMAND>(
					command.data))
			{
				return E_FAIL;
			}

			const auto& light =
				std::get<EFFECT_LIGHT_COMMAND>(
					command.data);

			json["commands"].push_back({
				{ "type", "LIGHT" },

				{ "commandName",
					light.sCommandName },

				{ "lightPresetName",
					light.sLightPresetName },

				{
					"localPosition",
					{
						light.vLocalPosition.x,
						light.vLocalPosition.y,
						light.vLocalPosition.z
					}
				},

				{
					"velocity",
					{
						light.vVelocity.x,
						light.vVelocity.y,
						light.vVelocity.z
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

				{ "intensity",
					light.fIntensity },

				{ "range",
					light.fRange },

				{ "spawnDelay",
					command.fSpawnDelay },

				{ "duration",
					light.fDuration }
				});

			break;
		}

		case EFFECT_COMMAND_TYPE::SOUND:
		{
			if (!std::holds_alternative<
				EFFECT_SOUND_COMMAND>(
					command.data))
			{
				return E_FAIL;
			}

			const auto& sound =
				std::get<EFFECT_SOUND_COMMAND>(
					command.data);

			const std::string soundPath(
				sound.sSoundPath.begin(),
				sound.sSoundPath.end());

			json["commands"].push_back({
				{ "type", "SOUND" },

				{ "commandName",
					sound.sCommandName },

				{ "soundPath",
					soundPath },

				{
					"localPosition",
					{
						sound.vLocalPosition.x,
						sound.vLocalPosition.y,
						sound.vLocalPosition.z
					}
				},

				{ "volume",
					sound.fVolume },

				{ "minDistance",
					sound.fMinDistance },

				{ "maxDistance",
					sound.fMaxDistance },

				{ "spawnDelay",
					command.fSpawnDelay },

				{ "loop",
					sound.bLoop },
				});

			break;
		}

		default:
			return E_FAIL;
		}
	}

	std::ofstream file(strPath);

	if (!file.is_open())
		return E_FAIL;

	file << json.dump(4);

	if (!file.good())
		return E_FAIL;

	return S_OK;
}

HRESULT CEffectManager::LoadEffectPreset(const std::string& strPath)
{
	if (strPath.empty())
		return E_FAIL;

	std::ifstream file(strPath);

	if (!file.is_open())
		return E_FAIL;

	try
	{
		nlohmann::json json;
		file >> json;

		if (!json.is_object() ||
			!json.contains("effectName") ||
			!json.contains("duration") ||
			!json.contains("commands"))
		{
			return E_FAIL;
		}

		if (!json["effectName"].is_string() ||
			!json["duration"].is_number() ||
			!json["commands"].is_array())
		{
			return E_FAIL;
		}

		EFFECT_PRESET loadedPreset{};

		loadedPreset.sEffectName = json["effectName"].get<std::string>();
		loadedPreset.fDuration = json["duration"].get<_float>();

		if (loadedPreset.sEffectName.empty())
			return E_FAIL;

		for (const nlohmann::json& commandJson : json["commands"])
		{
			if (!commandJson.is_object() ||
				!commandJson.contains("type") ||
				!commandJson["type"].is_string())
			{
				return E_FAIL;
			}

			const std::string commandType = commandJson["type"].get<std::string>();

			EFFECT_COMMAND command{};

			if (commandJson.contains("spawnDelay"))
				command.fSpawnDelay = commandJson["spawnDelay"].get<_float>();

			if (commandType == "PARTICLE")
			{
				EFFECT_PARTICLE_COMMAND particle{};

				command.eType = EFFECT_COMMAND_TYPE::PARTICLE;

				if (commandJson.contains("commandName"))
					particle.sCommandName = commandJson["commandName"].get<std::string>();

				if (commandJson.contains("particleJson"))
					particle.sParticleJson = commandJson["particleJson"].get<std::string>();

				if (particle.sParticleJson.empty())
					return E_FAIL;

				command.data = std::move(particle);
			}
			else if (commandType == "LIGHT")
			{
				EFFECT_LIGHT_COMMAND light{};

				command.eType = EFFECT_COMMAND_TYPE::LIGHT;

				if (commandJson.contains("commandName"))
					light.sCommandName = commandJson["commandName"].get<std::string>();

				if (commandJson.contains("lightPresetName"))
					light.sLightPresetName = commandJson["lightPresetName"].get<std::string>();

				if (commandJson.contains("localPosition"))
				{
					const auto& position = commandJson["localPosition"];

					if (!position.is_array() || position.size() != 3)
						return E_FAIL;

					light.vLocalPosition = {
						position[0].get<_float>(),
						position[1].get<_float>(),
						position[2].get<_float>()
					};
				}

				if (commandJson.contains("velocity"))
				{
					const auto& velocity = commandJson["velocity"];

					if (!velocity.is_array() || velocity.size() != 3)
						return E_FAIL;

					light.vVelocity = {
						velocity[0].get<_float>(),
						velocity[1].get<_float>(),
						velocity[2].get<_float>()
					};
				}

				if (commandJson.contains("color"))
				{
					const auto& color = commandJson["color"];

					if (!color.is_array() || color.size() != 3)
						return E_FAIL;

					light.vColor = {
						color[0].get<_float>(),
						color[1].get<_float>(),
						color[2].get<_float>()
					};
				}

				if (commandJson.contains("intensity"))
					light.fIntensity = commandJson["intensity"].get<_float>();

				if (commandJson.contains("range"))
					light.fRange = commandJson["range"].get<_float>();

				if (commandJson.contains("duration"))
					light.fDuration = commandJson["duration"].get<_float>();

				command.data = std::move(light);
			}
			else if (commandType == "SOUND")
			{
				EFFECT_SOUND_COMMAND sound{};

				command.eType = EFFECT_COMMAND_TYPE::SOUND;

				if (commandJson.contains("commandName"))
					sound.sCommandName = commandJson["commandName"].get<std::string>();

				if (commandJson.contains("soundPath"))
				{
					const std::string soundPath = commandJson["soundPath"].get<std::string>();
					sound.sSoundPath = _string(soundPath.begin(), soundPath.end());
				}

				if (commandJson.contains("localPosition"))
				{
					const auto& position = commandJson["localPosition"];

					if (!position.is_array() || position.size() != 3)
						return E_FAIL;

					sound.vLocalPosition = {
						position[0].get<_float>(),
						position[1].get<_float>(),
						position[2].get<_float>()
					};
				}

				if (commandJson.contains("volume"))
					sound.fVolume = commandJson["volume"].get<_float>();

				if (commandJson.contains("minDistance"))
					sound.fMinDistance = commandJson["minDistance"].get<_float>();

				if (commandJson.contains("maxDistance"))
					sound.fMaxDistance = commandJson["maxDistance"].get<_float>();

				if (commandJson.contains("loop"))
					sound.bLoop = commandJson["loop"].get<_bool>();

				if (sound.sSoundPath.empty())
					return E_FAIL;

				command.data = std::move(sound);
			}
			else
			{
				return E_FAIL;
			}

			loadedPreset.vecCommands.push_back(std::move(command));
		}

		std::stable_sort(
			loadedPreset.vecCommands.begin(),
			loadedPreset.vecCommands.end(),
			[](const EFFECT_COMMAND& lhs, const EFFECT_COMMAND& rhs)
			{
				return lhs.fSpawnDelay < rhs.fSpawnDelay;
			});

		return AddPreset(std::move(loadedPreset));
	}
	catch (const nlohmann::json::exception&)
	{
		return E_FAIL;
	}
	catch (const std::exception&)
	{
		return E_FAIL;
	}

	return S_OK;
}

EFFECT_INSTANCE_ID CEffectManager::PlayEffect(const std::string& sEffectName,const _float4x4& matWorld, _fvector vEndPosition, EFFECT_FINISHED_CALLBACK onFinished)
{
	_float totalLife = 0;
	auto presetIter = m_Presets.find(sEffectName);

	if (presetIter == m_Presets.end())
		return INVALID_EFFECT_INSTANCE_ID;

	_float4x4 noScaleWorld{};

	if (!MakeNoScaleWorldMatrix(matWorld,noScaleWorld))
	{
		return INVALID_EFFECT_INSTANCE_ID;
	}

	EFFECT_INSTANCE instance{};

	instance.iEffectId = m_iNextEffectId++;
	instance.pPreset = &presetIter->second;

	instance.matWorld = noScaleWorld;

	instance.fElapsed = 0.f;
	instance.fDuration = 0.f;
		

	instance.iNextCommandIndex = 0;
	instance.onFinished = std::move(onFinished);
	XMStoreFloat4(
		&instance.vEndPosition,
		vEndPosition);

	const EFFECT_INSTANCE_ID effectId =
		instance.iEffectId;

	auto [iter, inserted] =
		m_Instances.emplace(
			effectId,
			std::move(instance));

	if (!inserted)
		return INVALID_EFFECT_INSTANCE_ID;

	// spawnDelay가 0인 명령은 즉시 실행한다.
	DispatchReadyCommands(iter->second);

	return effectId;
}

void CEffectManager::DispatchReadyCommands(EFFECT_INSTANCE& instance)
{
	if (!instance.pPreset)
		return;

	const auto& commands = instance.pPreset->vecCommands;

	while (instance.iNextCommandIndex < commands.size())
	{
		const EFFECT_COMMAND& command = commands[instance.iNextCommandIndex];

		if (command.fSpawnDelay >instance.fElapsed)
		{
			break;
		}

		const _float commandLife = DispatchCommand(instance, command);

		if (commandLife < 0.f)
			instance.fDuration = -1.f;
		else if (instance.fDuration >= 0.f)
			instance.fDuration = std::max(instance.fDuration, instance.fElapsed + commandLife);

		++instance.iNextCommandIndex;
	}
}

_float CEffectManager::DispatchCommand(EFFECT_INSTANCE& instance,const EFFECT_COMMAND& command)
{

	switch (command.eType)
	{
	case EFFECT_COMMAND_TYPE::PARTICLE:
	{
		const auto& particleCommand = std::get<EFFECT_PARTICLE_COMMAND>(command.data);

		return DispatchParticle(instance, particleCommand);

	}
	case EFFECT_COMMAND_TYPE::LIGHT:
	{
		const auto& lightCommand = std::get<EFFECT_LIGHT_COMMAND>(command.data);

		return DispatchLight(instance,lightCommand);
	}

	case EFFECT_COMMAND_TYPE::SOUND:
	{
		const auto& soundCommand = std::get<EFFECT_SOUND_COMMAND>(command.data);

		DispatchSound(instance,soundCommand);

		break;
	}

	default:
		break;
	}
	
}

_float CEffectManager::DispatchParticle(EFFECT_INSTANCE& instance, const EFFECT_PARTICLE_COMMAND& command)
{
	if (!m_pParticleManager || command.sParticleJson.empty())
		return 0.f;

	auto particleQueue = m_pParticleManager->Parse_Command(command.sParticleJson);

	if (particleQueue.empty())
		return 0.f;

	_float totalLife = 0.f;

	for (const SPAWN_COMMAND& particle : particleQueue)
	{
		_float life = 0.f;
		const auto currentKind = particle.sGroupTag_KindTag;

		if (currentKind == SPAWN_COMMAND_KIND::STANDARD && std::holds_alternative<STANDARD_PARAMS>(particle.params))
		{
			const auto& param = std::get<STANDARD_PARAMS>(particle.params);

			if (param.bLoop)
				return -1.f;

			const uint32_t intervalCount = param.count > 0 ? param.count - 1 : 0;
			life = param.fSpawnDelay + param.fSpawnInterval * static_cast<_float>(intervalCount) + param.life;
		}
		else if (currentKind == SPAWN_COMMAND_KIND::BEAM && std::holds_alternative<BEAM_PARAMS>(particle.params))
		{
			const auto& param = std::get<BEAM_PARAMS>(particle.params);
			life = param.fSpawnDelay + param.beamDuration;
		}
		else if (currentKind == SPAWN_COMMAND_KIND::PATTERN && std::holds_alternative<PatternParamVariant>(particle.params))
		{
			const auto& pattern = std::get<PatternParamVariant>(particle.params);
			const auto spawnList = m_pParticleManager->BuildSpawnData(pattern);

			for (const PARTICLE_SPAWN_DATA& spawnData : spawnList)
			{
				if (spawnData.loop)
					return -1.f;

				life = std::max(life, spawnData.spawnDelay + spawnData.life);
			}
		}

		totalLife = std::max(totalLife, life);
	}

	const uint32_t ownerId = m_pParticleManager->Spawn(particleQueue, instance.matWorld, XMLoadFloat4(&instance.vEndPosition));

	if (ownerId != INVALID_PARTICLE_OWNER_ID)
		instance.vecParticleOwnerId.push_back(ownerId);

	return totalLife;
}

_float CEffectManager::DispatchLight(EFFECT_INSTANCE& instance,const EFFECT_LIGHT_COMMAND& command)
{
	if (!m_pLightManager)
		return 0.f;

	const _float3 worldPosition =TransformPosition(command.vLocalPosition,instance.matWorld);

	auto lightHandle =
		m_pLightManager->Allocate_EffectLight(
			XMVectorSet(worldPosition.x, worldPosition.y, worldPosition.z,1.f),
			command.fIntensity,
			command.vColor,
			command.fRange,
			command.fDuration,
			command.vVelocity);

	//핸들을 현재 재생대기중인 instance에 추가.
	if (lightHandle)
	{
		instance.vecLightHandles.push_back(
			*lightHandle);
	}
	return command.fDuration;
}

void CEffectManager::DispatchSound(EFFECT_INSTANCE& instance,const EFFECT_SOUND_COMMAND& command)
{
	if (!m_pSoundManager)
		return;

	const _float3 worldPosition =TransformPosition(command.vLocalPosition,instance.matWorld);

	SOUND_PLAY_DESC playDesc{};
	playDesc.fVolume = command.fVolume;
	playDesc.bLoop = command.bLoop;
	playDesc.sBusID = "SFX";
	SOUND_3D_DESC desc3D{};
	desc3D.fMinDistance = command.fMinDistance;
	desc3D.fMaxDistance = command.fMaxDistance;
	desc3D.vPosition = worldPosition;
	
	const SOUND_ID soundId = m_pSoundManager->Play3D(command.sSoundPath, desc3D,playDesc);

	if (soundId != INVALID_SOUND_ID)
	{
		instance.vecSoundIds.push_back(
			soundId);
	}
}

void CEffectManager::Stop(EFFECT_INSTANCE_ID iEffectId)
{
	auto iter =m_Instances.find(iEffectId);

	if (iter == m_Instances.end())
		return;

	EFFECT_INSTANCE& instance = iter->second;

	if (m_pParticleManager)
	{
		for (uint32_t ownerId : instance.vecParticleOwnerId)
			m_pParticleManager->ClearByOwner(ownerId);
	}

	if (m_pLightManager)
	{
		for (const CHandle& lightHandle :
			instance.vecLightHandles)
		{
			m_pLightManager->Reset_EffectLight(lightHandle);
		}
	}

	if (m_pSoundManager)
	{
		for (SOUND_ID soundId :instance.vecSoundIds)
		{
			m_pSoundManager->Stop(soundId);
		}
	}
	EFFECT_FINISHED_CALLBACK callback = std::move(instance.onFinished);

	m_Instances.erase(iter);

	if (callback)
		callback(iEffectId, EFFECT_FINISH_REASON::STOPPED);

}

void CEffectManager::SetWorldMatrix(EFFECT_INSTANCE_ID iEffectId,const _float4x4& colliderWorldMatrix)
{
	auto iter =m_Instances.find(iEffectId);

	if (iter == m_Instances.end())
		return;

	EFFECT_INSTANCE& instance =iter->second;

	_float4x4 newNoScaleWorld{};

	if (!MakeNoScaleWorldMatrix(colliderWorldMatrix,newNoScaleWorld))
	{
		return;
	}

	const XMMATRIX oldWorld = XMLoadFloat4x4(&instance.matWorld);

	const XMMATRIX newWorld =XMLoadFloat4x4(&newNoScaleWorld);

	XMVECTOR determinant{};

	const XMMATRIX inverseOldWorld =XMMatrixInverse(&determinant,oldWorld);

	if (fabsf(XMVectorGetX(determinant)) < 0.000001f)
	{
		return;
	}

	const XMMATRIX deltaMatrix =
		inverseOldWorld * newWorld;

	_float4x4 deltaMatrixData{};

	XMStoreFloat4x4(&deltaMatrixData,deltaMatrix);

	/*
	 * 지연 소환되는 명령은 최신 콜라이더 위치와
	 * 회전을 사용해야 하므로 먼저 갱신한다.
	 */
	instance.matWorld =
		newNoScaleWorld;


	for (uint32_t ownerId :instance.vecParticleOwnerId)
	{
		m_pParticleManager->TransformOwner(ownerId,deltaMatrixData);
	}

	const _float3  deltaPos = _float3(deltaMatrixData._41, deltaMatrixData._42, deltaMatrixData._43);
	if (m_pLightManager)
	{
		for (const CHandle& lightHandle :instance.vecLightHandles)
		{
			//m_pLightManager->TransformLight(lightHandle, deltaPos);
		}
	}

	//if (m_pSoundManager)
	//{
	//	for (SOUND_ID soundId :
	//	instance.vecSoundIds)
	//	{
	//		m_pSoundManager
	//			->Transform3DPosition(
	//				soundId,
	//				deltaMatrixData);
	//	}
	//}
}

void CEffectManager::ChangeColorByOwner(EFFECT_INSTANCE_ID iEffectId, const _float4& vColor)
{
	auto iter = m_Instances.find(iEffectId);

	if (iter == m_Instances.end() || !m_pParticleManager)
		return;

	for (uint32_t ownerId : iter->second.vecParticleOwnerId)
		m_pParticleManager->SetColorByOwner(ownerId, vColor);
}

void CEffectManager::SetPosition(EFFECT_INSTANCE_ID iEffectId,const _float3& newPosition)
{
	auto iter =
		m_Instances.find(iEffectId);

	if (iter == m_Instances.end())
		return;

	/*
	 * 순수 위치 변경도 SetWorldMatrix로 통일한다.
	 * 현재 회전은 유지되고 위치만 교체된다.
	 */
	_float4x4 newWorld =
		iter->second.matWorld;

	newWorld._41 = newPosition.x;
	newWorld._42 = newPosition.y;
	newWorld._43 = newPosition.z;

	SetWorldMatrix(
		iEffectId,
		newWorld);
}

void CEffectManager::RemoveFinishedInstances()
{
	std::vector<EFFECT_INSTANCE_ID> finishedIds;

	for (const auto& [effectId, instance] : m_Instances)
	{
		if (!instance.pPreset)
		{
			finishedIds.push_back(effectId);
			continue;
		}

		const bool allCommandsDispatched = instance.iNextCommandIndex >= instance.pPreset->vecCommands.size();
		const bool hasFiniteDuration = instance.fDuration >= 0.f;
		const bool durationFinished = instance.fElapsed >= instance.fDuration;

		if (allCommandsDispatched && hasFiniteDuration && durationFinished)
			finishedIds.push_back(effectId);
	}

	for (EFFECT_INSTANCE_ID effectId : finishedIds)
		Finish(effectId, EFFECT_FINISH_REASON::NATURAL);
}

void CEffectManager::Finish(EFFECT_INSTANCE_ID effectId, EFFECT_FINISH_REASON reason)
{
	auto iter = m_Instances.find(effectId);

	if (iter == m_Instances.end())
		return;

	EFFECT_FINISHED_CALLBACK callback = std::move(iter->second.onFinished);

	m_Instances.erase(iter);

	if (callback)
		callback(effectId, reason);
}

const EFFECT_INSTANCE *CEffectManager::FindInstance(EFFECT_INSTANCE_ID iEffectId) const
{
	auto iter = m_Instances.find(iEffectId);

	if (iter == m_Instances.end())
		return nullptr;

	return &iter->second;
}

_float3 CEffectManager::TransformPosition(
	const _float3& localPosition,
	const _float4x4& worldMatrix) const
{
	_float3 worldPosition{};

	XMStoreFloat3(&worldPosition,XMVector3TransformCoord(XMLoadFloat3(&localPosition),XMLoadFloat4x4(&worldMatrix)));

	return worldPosition;
}

_float3 CEffectManager::TransformDirection(
	const _float3& localDirection,
	const _float4x4& worldMatrix) const
{
	const XMVECTOR transformedDirection =XMVector3TransformNormal(XMLoadFloat3(&localDirection),XMLoadFloat4x4(&worldMatrix));

	const XMVECTOR lengthSquared = XMVector3LengthSq(transformedDirection);

	_float3 worldDirection{};

	if (XMVectorGetX(lengthSquared) <0.000001f)
	{
		worldDirection = localDirection;
	}
	else
	{
		XMStoreFloat3(&worldDirection,XMVector3Normalize(transformedDirection));
	}

	return worldDirection;
}

_bool CEffectManager::MakeNoScaleWorldMatrix(
	const _float4x4& sourceMatrix,
	_float4x4& outMatrix) const
{
	const XMMATRIX source =
		XMLoadFloat4x4(
			&sourceMatrix);

	XMVECTOR scale{};
	XMVECTOR rotation{};
	XMVECTOR translation{};

	if (!XMMatrixDecompose(
		&scale,
		&rotation,
		&translation,
		source))
	{
		return false;
	}

	const XMMATRIX noScaleMatrix =
		XMMatrixRotationQuaternion(rotation) *
		XMMatrixTranslationFromVector(
			translation);

	XMStoreFloat4x4(
		&outMatrix,
		noScaleMatrix);

	return true;
}

HRESULT CEffectManager::AddPreset(EFFECT_PRESET&& preset)
{
	if (preset.sEffectName.empty()) {
		MSG_BOX("Failed to add Preset: Effect Name is Empty");
		return E_FAIL;
	}

	auto iter = m_Presets.find(preset.sEffectName);
	if (iter != m_Presets.end())
	{
		MSG_BOX("Failed to add Preset: Duplicate Effect Name");
		return E_FAIL;
	}
	m_Presets.emplace(preset.sEffectName, std::move(preset));

	return S_OK;
}

UPtr<CEffectManager>
CEffectManager::Create(CParticleManager* pParticleManager,CLightManager* pLightManager,CSoundManager* pSoundManager)
{
	auto pInstance = UPtr<CEffectManager>(new CEffectManager{ pParticleManager,pLightManager,pSoundManager });

	if (!pInstance) {
		MSG_BOX("Failed to create Effect Manager");
		return nullptr;
	}
	pInstance->Initialize();

	return pInstance;
}
std::vector<std::string> CEffectManager::Load_FilePath_ByExtension(const std::filesystem::path& _FolderPath, std::string_view _Extension) {
	std::vector<std::string> FilePathStorage{};
	FilePathStorage.reserve(32);

	{
		namespace fs = std::filesystem;

		std::error_code ErrorCode{};

		if (fs::exists(_FolderPath) == false || fs::is_directory(_FolderPath) == false) {
			std::wstring MSGContent = L"Invalid FolderPath : " + _FolderPath.wstring();
			MessageBoxW(NULL, MSGContent.c_str(), L"System Message", MB_OK);

			return FilePathStorage;      // Empty vector return
		}
		auto Optimization = fs::directory_options::skip_permission_denied;

		fs::recursive_directory_iterator iterator(_FolderPath, Optimization, ErrorCode);
		fs::recursive_directory_iterator End;

		for (; iterator != End && !ErrorCode; iterator.increment(ErrorCode)) {
			if (iterator->is_regular_file(ErrorCode)) {
				const auto& FilePath = iterator->path();

				if (FilePath.extension() == _Extension) {
					FilePathStorage.push_back(FilePath.string());
				}
			}
		}
		return FilePathStorage;
	}
}
HRESULT CEffectManager::Load_EffectJsonPackage(
	const std::vector<std::string>& filePaths)
{
	if (filePaths.empty())
		return E_FAIL;

	for (const auto& filePath : filePaths) {
		const HRESULT hr = LoadEffectPreset(filePath.c_str());

		if (FAILED(hr))
			return hr;
	}

	return S_OK;
}
