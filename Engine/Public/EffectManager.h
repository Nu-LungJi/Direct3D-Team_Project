#pragma once

#include "Engine_Defines.h"
#include "Handle.h"
#include "SoundManager.h"

NS_BEGIN(Engine)

class CParticleManager;
class CLightManager;
class CSoundManager;


inline bool InputText(const char* label, std::string& str, size_t maxLen = 256)
{
	std::vector<char> buf(maxLen);
	strncpy_s(buf.data(), buf.size(), str.c_str(), _TRUNCATE);

	bool changed = ImGui::InputText(label, buf.data(), buf.size());
	if (changed)
		str = buf.data(); // 수정됐으면 다시 std::string에 반영

	return changed;
}

class ENGINE_DLL CEffectManager final : public CEngineBase
{

public:
	CEffectManager(const CEffectManager&) = delete;
	CEffectManager& operator=(const CEffectManager& rhs) = delete;


private:
	CEffectManager(CParticleManager* pParticleManager,
		CLightManager* pLightManager,
		CSoundManager* pSoundManager);
	~CEffectManager()override;

public:
	HRESULT Initialize();
	void UpdateGUI();
	void Update(_float fTimeDelta);


	HRESULT SaveEffectPreset(const std::string& strPath,const EFFECT_PRESET& preset);

	HRESULT LoadEffectPreset(const std::string& strPath);

	EFFECT_INSTANCE_ID Spawn(const std::string& sEffectName,
		const _float4x4& matWorld,
		_fvector vEndPosition = XMVectorZero());

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
	void DispatchReadyCommands(EFFECT_INSTANCE& instance);

	void DispatchCommand(EFFECT_INSTANCE& instance,const EFFECT_COMMAND& command);

	void DispatchParticle(EFFECT_INSTANCE& instance, const EFFECT_PARTICLE_COMMAND& command);

	void DispatchLight(EFFECT_INSTANCE& instance,const EFFECT_LIGHT_COMMAND& command);

	void DispatchSound(EFFECT_INSTANCE& instance,const EFFECT_SOUND_COMMAND& command);

	void RemoveFinishedInstances();

	_float3 TransformPosition(const _float3& localPosition,const _float4x4& worldMatrix) const;

	_float3 TransformDirection(	const _float3& localDirection,const _float4x4& worldMatrix) const;

	_bool MakeNoScaleWorldMatrix(const _float4x4& sourceMatrix,_float4x4& outMatrix) const;

	HRESULT AddPreset(EFFECT_PRESET&& preset);
private:
	CParticleManager* m_pParticleManager = nullptr;
	CLightManager* m_pLightManager = nullptr;
	CSoundManager* m_pSoundManager = nullptr;

	std::unordered_map<std::string,EFFECT_PRESET> m_Presets;

	std::unordered_map<EFFECT_INSTANCE_ID,EFFECT_INSTANCE> m_Instances;

	EFFECT_INSTANCE_ID m_iNextEffectId = 1;

public:
	static UPtr<CEffectManager> Create(CParticleManager* pParticleManager,CLightManager* pLightManager,CSoundManager* pSoundManager);
};

NS_END
