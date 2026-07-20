#pragma once

#include "Engine_Defines.h"

#include <array>
#include <mutex>

struct FMOD_SYSTEM;
struct FMOD_SOUND;
struct FMOD_CHANNEL;
struct FMOD_CHANNELGROUP;

NS_BEGIN(Engine)

class CResFmodSound;
struct SSoundCallbackBridge;

using SOUND_ID = uint64_t;
inline constexpr SOUND_ID INVALID_SOUND_ID = 0;

enum class SOUND_BUS : uint8_t
{
	MASTER,
	BGM,
	SFX,
	VOICE,
	UI,
	AMBIENCE,
	END
};

enum class SOUND_LOAD_TYPE : uint8_t
{
	SAMPLE,
	STREAM
};

enum class SOUND_3D_ROLLOFF : uint8_t
{
	INVERSE,
	LINEAR
};

struct SOUND_PLAY_DESC
{
	SOUND_BUS eBus{ SOUND_BUS::SFX };
	_float fVolume{ 1.f };
	_float fPitch{ 1.f };
	int32_t iPriority{ 128 };
	_bool bLoop{};
	_bool bStartPaused{};
};

struct SOUND_3D_DESC
{
	_float3 vPosition{};
	_float3 vVelocity{};
	_float fMinDistance{ 1.f };
	_float fMaxDistance{ 50.f };
	SOUND_3D_ROLLOFF eRolloff{ SOUND_3D_ROLLOFF::INVERSE };
};

struct SOUND_LISTENER_DESC
{
	_float3 vPosition{};
	_float3 vVelocity{};
	_float3 vForward{ 0.f, 0.f, 1.f };
	_float3 vUp{ 0.f, 1.f, 0.f };
};

class CSoundManager final : public CEngineBase
{
	friend struct SSoundCallbackBridge;

private:
	struct SPlayingSound
	{
		SPtr<CResFmodSound> pSound{};
		FMOD_CHANNEL* pChannel{};
		SOUND_BUS eBus{ SOUND_BUS::SFX };
		_bool b3D{};
	};

private:
	CSoundManager();
	~CSoundManager();

public:
	void UpdateGUI();

public:
	HRESULT Initialize();
	void Update();
	HRESULT CreateSound(const _string& sPath, FMOD_SOUND** ppSound, SOUND_LOAD_TYPE eLoadType = SOUND_LOAD_TYPE::SAMPLE);

public:
	_bool Preload(const _string& sPath, SOUND_LOAD_TYPE eLoadType = SOUND_LOAD_TYPE::SAMPLE);
	_bool RemoveResourceByPath(const _string& sPath);
	void ClearResources();

	SOUND_ID Play2D(const _string& sPath, const SOUND_PLAY_DESC& tDesc = {},
		SOUND_LOAD_TYPE eLoadType = SOUND_LOAD_TYPE::SAMPLE);
	SOUND_ID Play3D(const _string& sPath, const SOUND_3D_DESC& t3DDesc,
		const SOUND_PLAY_DESC& tPlayDesc = {}, SOUND_LOAD_TYPE eLoadType = SOUND_LOAD_TYPE::SAMPLE);

	_bool Stop(SOUND_ID iSoundID);
	_bool SetPaused(SOUND_ID iSoundID, _bool bPaused);
	_bool SetVolume(SOUND_ID iSoundID, _float fVolume);
	_bool SetPitch(SOUND_ID iSoundID, _float fPitch);
	_bool Set3DAttributes(SOUND_ID iSoundID, const _float3& vPosition, const _float3& vVelocity = {});
	_bool Set3DMinMaxDistance(SOUND_ID iSoundID, _float fMinDistance, _float fMaxDistance);
	_bool IsPlaying(SOUND_ID iSoundID) const;
	_bool IsPaused(SOUND_ID iSoundID) const;
	_bool IsValidSound(SOUND_ID iSoundID) const;

public:
	_bool SetListenerAttributes(uint32_t iListenerIndex, const SOUND_LISTENER_DESC& tDesc);
	_bool SetBusVolume(SOUND_BUS eBus, _float fVolume);
	_bool SetBusMuted(SOUND_BUS eBus, _bool bMuted);
	_bool SetBusPaused(SOUND_BUS eBus, _bool bPaused);
	_bool StopBus(SOUND_BUS eBus);

private:
	SPtr<CResFmodSound> GetOrLoadResourceByPath(const _string& sPath, SOUND_LOAD_TYPE eLoadType);
	SOUND_ID PlayInternal(const SPtr<CResFmodSound>& pSound, const SOUND_PLAY_DESC& tDesc,
		const SOUND_3D_DESC* p3DDesc);
	SOUND_ID GenerateSoundID();
	FMOD_CHANNELGROUP* GetBus(SOUND_BUS eBus) const;
	_bool StopSoundsByPath(const _string& sNormalizedPath);
	void StopAllSounds();
	void EnqueueCompletedSound(SOUND_ID iSoundID);
	void FlushCompletedSounds();

private:
	std::mutex m_SoundResourceRegistrationMutex{};

	std::unordered_map<SOUND_ID, SPlayingSound> m_mapPlayingSounds{};
	std::array<FMOD_CHANNELGROUP*, static_cast<size_t>(SOUND_BUS::END)> m_pBuses{};
	SOUND_ID m_iNextSoundID{ 1 };

	std::mutex m_CompletedSoundMutex{};
	std::queue<SOUND_ID> m_CompletedSounds{};

	FMOD_SYSTEM* m_pSystem{};

public:
	static UPtr<CSoundManager> Create();

private:
	void Free() override;
};

NS_END
