#pragma once
#include "Engine_Defines.h"

#include "fmod_common.h"
//#include <fmod.hpp>
//struct FMOD_SYSTEM;
//struct FMOD_SOUND;
//struct FMOD_CHANNEL;
//NS_BEGIN(FMOD)
//class Channel;
//NS_END
NS_BEGIN(Engine)
class CResFmodSound;

class CSoundManager final : public CEngineBase
{
public:
	struct SChannel
	{
		std::vector<std::tuple<StringID, StringID, SPtr<CResFmodSound>>> vecRes{};
		FMOD_CHANNEL* pChannel{};
	};
private:
	CSoundManager();
	~CSoundManager();

public:
	void UpdateGUI();

public:
	HRESULT Initialize();
	void Update();
	HRESULT CreateSound(const _string& sPath, FMOD_SOUND** ppSound);

public:
	const SChannel* GetChannel(const StringID& channelTag) const;
	HRESULT AddChannel(const StringID& channelTag, const std::pair<StringID, StringID>& soundResources);
	HRESULT Play(const StringID& channelTag);
	HRESULT Play(const StringID& channelTag, _float fVolume);
	HRESULT Play(const StringID& channelTag, _float fVolume, _float fPitch);
	HRESULT PlayLoop(const StringID& channelTag, _float fVolume);
	void Stop(const StringID& channelTag);
	void Pause(const StringID& channelTag, _bool bPause);
	_bool GetVolume(const StringID& channelTag, _float& fVolume);
	_bool SetVolume(const StringID& channelTag, _float fVolume);
	_bool IsPlaying(const StringID& channelTag) const;
	void SetPitch(const StringID& channelTag, float fPitchRatio);

private:
	std::unordered_map<StringID, SChannel> m_mapChannels{};

private:
	FMOD_SYSTEM* m_pSystem{};

public:
	static UPtr<CSoundManager> Create();

private:
	void Free() override;
};

NS_END
