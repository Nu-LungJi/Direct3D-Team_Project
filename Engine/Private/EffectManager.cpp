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

	ImGui::Text(
		"Active Effects: %zu",
		m_Instances.size());

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

HRESULT CEffectManager::AddPreset(
	EFFECT_PRESET preset)
{
	if (preset.sEffectName.empty())
		return E_FAIL;

	std::sort(preset.vecCommands.begin(),preset.vecCommands.end(),
		[](const EFFECT_COMMAND& lhs,
			const EFFECT_COMMAND& rhs)
		{
			return lhs.fSpawnDelay <
				rhs.fSpawnDelay;
		});

	m_Presets[preset.sEffectName] =
		std::move(preset);

	return S_OK;
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

				{ "pitch",
					sound.fPitch },

				{ "minDistance",
					sound.fMinDistance },

				{ "maxDistance",
					sound.fMaxDistance },

				{ "spawnDelay",
					command.fSpawnDelay },

				{ "loop",
					sound.bLoop },

				{ "is3D",
					sound.b3D }
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
	return S_OK;
}

EFFECT_INSTANCE_ID CEffectManager::Spawn(const std::string& sEffectName,const _float4x4& matWorld, _fvector vEndPosition)
{
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
	instance.fDuration =
		presetIter->second.fDuration;

	instance.iNextCommandIndex = 0;

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

		DispatchCommand(instance,command);

		++instance.iNextCommandIndex;
	}
}

void CEffectManager::DispatchCommand(EFFECT_INSTANCE& instance,const EFFECT_COMMAND& command)
{
	switch (command.eType)
	{
	case EFFECT_COMMAND_TYPE::PARTICLE:
	{
		const auto& particleCommand = std::get<EFFECT_PARTICLE_COMMAND>(command.data);

		DispatchParticle(instance, particleCommand);

		break;
	}
	case EFFECT_COMMAND_TYPE::LIGHT:
	{
		const auto& lightCommand = std::get<EFFECT_LIGHT_COMMAND>(command.data);

		DispatchLight(instance,lightCommand);

		break;
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

void CEffectManager::DispatchParticle(EFFECT_INSTANCE& instance,const EFFECT_PARTICLE_COMMAND& command)
{
	if (!m_pParticleManager)
		return;

	if (command.sParticleJson.empty())
		return;

	auto particleQueue =
		m_pParticleManager->Parse_Command(command.sParticleJson);

	if (particleQueue.empty())
		return;

	/*
	 * JSON 안에 파티클 명령이 여러 개 있어도
	 * 이 큐 전체에 ownerId 하나가 발급된다.
	 */
	const uint32_t ownerId = m_pParticleManager->
		Spawn(particleQueue,instance.matWorld,XMLoadFloat4(&instance.vEndPosition));
	if (ownerId != INVALID_PARTICLE_OWNER_ID)
	{
		instance.vecParticleOwnerId.push_back(ownerId);
	}
}

void CEffectManager::DispatchLight(EFFECT_INSTANCE& instance,const EFFECT_LIGHT_COMMAND& command)
{
	if (!m_pLightManager)
		return;

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
}

void CEffectManager::DispatchSound(EFFECT_INSTANCE& instance,const EFFECT_SOUND_COMMAND& command)
{
	if (!m_pSoundManager)
		return;

	const _float3 worldPosition =TransformPosition(command.vLocalPosition,instance.matWorld);

	/*
	 * 이 부분은 실제 SoundManager의 재생 함수
	 * 시그니처에 맞춰 이름과 인자를 변경해야 한다.
	 */
	/*const SOUND_ID soundId =
		m_pSoundManager->PlayEffectSound(
			command.sSoundPath,
			worldPosition,
			command.fVolume,
			command.fPitch,
			command.fMinDistance,
			command.fMaxDistance,
			command.bLoop,
			command.b3D);

	if (soundId != INVALID_SOUND_ID)
	{
		instance.vecSoundIds.push_back(
			soundId);
	}*/
}

void CEffectManager::Stop(EFFECT_INSTANCE_ID iEffectId)
{
	auto iter =m_Instances.find(iEffectId);

	if (iter == m_Instances.end())
		return;

	EFFECT_INSTANCE& instance =
		iter->second;

	for (uint32_t ownerId :instance.vecParticleOwnerId)
	{
		m_pParticleManager->ClearByOwner(ownerId);
	}

	if (m_pLightManager)
	{
		for (const CHandle& lightHandle :
			instance.vecLightHandles)
		{
		//	m_pLightManager->StopEffectLight(lightHandle);
		}
	}

	if (m_pSoundManager)
	{
		for (SOUND_ID soundId :instance.vecSoundIds)
		{
			m_pSoundManager->Stop(soundId);
		}
	}

	m_Instances.erase(iter);
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

void CEffectManager::SetPosition(
	EFFECT_INSTANCE_ID iEffectId,
	const _float3& newPosition)
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
	for (auto iter = m_Instances.begin(); iter != m_Instances.end();)
	{
		EFFECT_INSTANCE& instance = iter->second;

		if (!instance.pPreset)
		{
			iter = m_Instances.erase(iter);
			continue;
		}

		const _bool allCommandsDispatched =
			instance.iNextCommandIndex >= instance.pPreset ->vecCommands.size();

		const _bool durationFinished =
			instance.fDuration > 0.f &&
			instance.fElapsed >=
			instance.fDuration;

		if (allCommandsDispatched &&
			durationFinished)
		{
			/*
			 * 자연 종료된 자식들을 Stop하지 않는다.
			 * EffectManager의 추적 정보만 제거한다.
			 */
			iter = m_Instances.erase(iter);
		}
		else
		{
			++iter;
		}
	}
}

const EFFECT_INSTANCE*
CEffectManager::FindInstance(EFFECT_INSTANCE_ID iEffectId) const
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

UPtr<CEffectManager>
CEffectManager::Create(CParticleManager* pParticleManager,CLightManager* pLightManager,CSoundManager* pSoundManager)
{
	return UPtr<CEffectManager>(new CEffectManager{pParticleManager,pLightManager,pSoundManager});
}
