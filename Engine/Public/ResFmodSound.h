#pragma once

#include "Engine_Defines.h"

#include "Resource.h"
#include "SoundManager.h"
struct FMOD_SOUND;

NS_BEGIN(Engine)

class ENGINE_DLL CResFmodSound final: public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResFmodSound, CResource)

private:
	explicit CResFmodSound(const _string& sPath, SOUND_LOAD_TYPE eLoadType);
	~CResFmodSound() override;

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

public:
	FMOD_SOUND* GetSound() const { return m_pFmodSound; }

private:
	FMOD_SOUND* m_pFmodSound{};
	SOUND_LOAD_TYPE m_eLoadType{ SOUND_LOAD_TYPE::SAMPLE };

public:
	static SPtr<CResFmodSound> Create(const _string& sPath, SOUND_LOAD_TYPE eLoadType = SOUND_LOAD_TYPE::SAMPLE);

private:
	void Free() override;
};

NS_END
