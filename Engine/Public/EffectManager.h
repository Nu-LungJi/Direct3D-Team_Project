#pragma once

#include "Engine_Defines.h"
#include "EffectDefines.h"

NS_BEGIN(Engine)

class CParticleManager;
class CLightManager;
class CSoundManager;

class ENGINE_DLL CEffectManager final : public CEngineBase
{
private:
	explicit CEffectManager(
		CParticleManager* pParticleManager,
		CLightManager* pLightManager,
		CSoundManager* pSoundManager);

	~CEffectManager() override;

public:
	void UpdateGUI();

	HRESULT AddPreset(EFFECT_PRESET preset);
	EFFECT_INSTANCE_ID Spawn(const std::string& sPresetName,const _float4x4& matWorld,_fvector vEndPosition = XMVectorZero());
	HRESULT SaveEffectPreset(const std::string& strPath, const EFFECT_PRESET& preset);


	void Stop(EFFECT_INSTANCE_ID iEffectId);
	void Update(_float fTimeDelta);
	void Clear();
	const EFFECT_INSTANCE* FindInstance(EFFECT_INSTANCE_ID iEffectId) const;
	void RemoveFinishedInstances();
private:
	_float3 TransformPosition(
		const _float3& vLocalPosition,
		const _float4x4& matWorld) const;

	void DispatchReadyCommands(EFFECT_INSTANCE& instance);

	void DispatchCommand(EFFECT_INSTANCE& instance, const EFFECT_COMMAND& command);

	void DispatchLight(EFFECT_INSTANCE& instance, const EFFECT_LIGHT_COMMAND& command);

private:
	CParticleManager* m_pParticleManager = nullptr;
	CLightManager* m_pLightManager = nullptr;
	CSoundManager* m_pSoundManager = nullptr;

	EFFECT_INSTANCE_ID m_iNextEffectId = 1;
	//프리셋 파일이름과 프리셋 정보
	std::unordered_map<std::string, EFFECT_PRESET> m_Presets;

	//현재 재생중인 이펙트들을 보관해 놓은 것
	std::unordered_map<EFFECT_INSTANCE_ID,EFFECT_INSTANCE> m_Instances;

public:
	static UPtr<CEffectManager> Create(CParticleManager* pParticleManager,CLightManager* pLightManager,CSoundManager* pSoundManager);
};

NS_END
