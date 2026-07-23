#pragma once

#include "Engine_Defines.h"
#include "Engine_ParticleDefines.h"
#include "Handle.h"
#include "SoundManager.h"

NS_BEGIN(Engine)

class CParticleManager;
class CLightManager;
class CSoundManager;

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

	EFFECT_LIGHT_TYPE eType = EFFECT_LIGHT_TYPE::POINT;

	_float3 vLocalPosition{};
	_float3 vDirection{ 0.f, -1.f, 0.f };
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

using EffectCommandVariant =
std::variant<
	EFFECT_LIGHT_COMMAND,
	EFFECT_SOUND_COMMAND>;

struct EFFECT_COMMAND
{
	EFFECT_COMMAND_TYPE eType = EFFECT_COMMAND_TYPE::END;

	// 정렬 및 디스패치에서 사용하는 단일 지연 시간
	_float fSpawnDelay = 0.f;

	EffectCommandVariant data;
};

struct EFFECT_PRESET
{
	std::string sEffectName;

	_float fDuration = 0.f;

	// 하나의 파티클 JSON 안에 여러 파티클이 들어갈 수 있다.
	EFFECT_PARTICLE_COMMAND particleCommand{};

	std::vector<EFFECT_COMMAND> vecCommands;
};

struct EFFECT_INSTANCE
{
	EFFECT_INSTANCE_ID iEffectId =
		INVALID_EFFECT_INSTANCE_ID;

	uint32_t iParticleOwnerId =
		INVALID_PARTICLE_OWNER_ID;

	std::vector<CHandle> vecLightHandles;
	std::vector<SOUND_ID> vecSoundIds;

	const EFFECT_PRESET* pPreset = nullptr;

	// 스케일이 제거된 현재 부착 대상 월드 행렬
	_float4x4 matWorld{};

	_float4 vEndPosition{};

	_float fElapsed = 0.f;
	_float fDuration = 0.f;

	size_t iNextCommandIndex = 0;

	_bool bParticleDispatched = false;
};

class CEffectManager final
{
private:
	CEffectManager(
		CParticleManager* pParticleManager,
		CLightManager* pLightManager,
		CSoundManager* pSoundManager);

public:
	~CEffectManager();

public:
	void UpdateGUI();
	void Update(_float fTimeDelta);

	HRESULT AddPreset(EFFECT_PRESET preset);

	HRESULT SaveEffectPreset(
		const std::string& strPath,
		const EFFECT_PRESET& preset);

	HRESULT LoadEffectPreset(
		const std::string& strPath);

	EFFECT_INSTANCE_ID Spawn(
		const std::string& sEffectName,
		const _float4x4& matWorld,
		_fvector vEndPosition);

	void Stop(EFFECT_INSTANCE_ID iEffectId);

	void SetPosition(
		EFFECT_INSTANCE_ID iEffectId,
		const _float3& vPosition);

	void SetWorldMatrix(
		EFFECT_INSTANCE_ID iEffectId,
		const _float4x4& colliderWorldMatrix);

	const EFFECT_INSTANCE* FindInstance(
		EFFECT_INSTANCE_ID iEffectId) const;

private:
	void DispatchReadyCommands(
		EFFECT_INSTANCE& instance);

	void DispatchCommand(
		EFFECT_INSTANCE& instance,
		const EFFECT_COMMAND& command);

	void DispatchParticle(
		EFFECT_INSTANCE& instance,
		const EFFECT_PARTICLE_COMMAND& command);

	void DispatchLight(
		EFFECT_INSTANCE& instance,
		const EFFECT_LIGHT_COMMAND& command);

	void DispatchSound(
		EFFECT_INSTANCE& instance,
		const EFFECT_SOUND_COMMAND& command);

	void RemoveFinishedInstances();

	_float3 TransformPosition(
		const _float3& localPosition,
		const _float4x4& worldMatrix) const;

	_float3 TransformDirection(
		const _float3& localDirection,
		const _float4x4& worldMatrix) const;

	_bool MakeNoScaleWorldMatrix(
		const _float4x4& sourceMatrix,
		_float4x4& outMatrix) const;

private:
	CParticleManager* m_pParticleManager = nullptr;
	CLightManager* m_pLightManager = nullptr;
	CSoundManager* m_pSoundManager = nullptr;

	std::unordered_map<
		std::string,
		EFFECT_PRESET> m_Presets;

	std::unordered_map<
		EFFECT_INSTANCE_ID,
		EFFECT_INSTANCE> m_Instances;

	EFFECT_INSTANCE_ID m_iNextEffectId = 1;

public:
	static UPtr<CEffectManager> Create(
		CParticleManager* pParticleManager,
		CLightManager* pLightManager,
		CSoundManager* pSoundManager);
};

NS_END
