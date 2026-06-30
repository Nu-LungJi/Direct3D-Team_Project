#pragma once

#include "Engine_Defines.h"

#include "Resource.h"
struct FMOD_SOUND;

NS_BEGIN(Engine)

class ENGINE_DLL CResFmodSound final: public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResFmodSound, CResource)

private:
	explicit CResFmodSound(const _string& sPath);
	~CResFmodSound() override;

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

public:
	FMOD_SOUND* GetSound() const { return m_pFmodSound; }

private:
	FMOD_SOUND* m_pFmodSound{};

public:
	static SPtr<CResFmodSound> Create(const _string& sPath);
};

NS_END