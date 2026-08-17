#pragma once

#include "Engine_Defines.h"

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
using SOUND_BUS_ID = StringID;
inline const SOUND_BUS_ID SOUND_MASTER_BUS_ID{ "MASTER" };


enum class SOUND_LOAD_TYPE : uint8_t
{
	SAMPLE,
	STREAM
};

enum class SOUND_3D_ROLLOFF : uint8_t
{
	// 반비례 감쇠: 거리가 멀어질수록 볼륨이 반비례로 줄어든다.(현실세계 모방)
	INVERSE,
	// 선형 감쇠 : 거리가 멀어질수록 볼륨이 선형적으로 줄어든다.
	LINEAR
};

// 2D, 3D 공통적으로 받아야하는 사운드 DESC
struct SOUND_PLAY_DESC
{
	SOUND_BUS_ID sBusID{ SOUND_MASTER_BUS_ID };
	_float fVolume{ 1.f };
	_float fPitch{ 1.f };
	_float fFadeInDuration{};
	int32_t iPriority{ 128 };
	_bool bLoop{};
	_bool bStartPaused{};
};

// 3D Attr을 달아주기위한 DESC
// velocity는 도플러 효과를 위한건데 필요업으면 0 벡터
struct SOUND_3D_DESC
{
	_float3 vPosition{};
	_float3 vVelocity{};
	_float fMinDistance{ 1.f };
	_float fMaxDistance{ 50.f };
	SOUND_3D_ROLLOFF eRolloff{ SOUND_3D_ROLLOFF::INVERSE };
};

// 3D사운드 리스너 위치
struct SOUND_LISTENER_DESC
{
	_float3 vPosition{};
	_float3 vVelocity{};
	_float3 vForward{ 0.f, 0.f, 1.f };
	_float3 vUp{ 0.f, 1.f, 0.f };
};

class ENGINE_DLL CSoundManager final : public CEngineBase
{
	friend struct SSoundCallbackBridge;

private:
	struct SPlayingSound
	{
		SPtr<CResFmodSound> pSound{};
		FMOD_CHANNEL* pChannel{};
		SOUND_BUS_ID sBusID{ SOUND_MASTER_BUS_ID };
		_bool b3D{};
		SOUND_3D_DESC t3DDesc{};

		// FMOD FadePoint가 완료된 뒤 사라져도
		// 마지막으로 페이드된 실제 채널 볼륨을 유지하기 위한 값.
		_bool bHasLastFadeTarget{};
		_float fLastFadeTargetVolume{};
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
	_bool FadeTo(SOUND_ID iSoundID, _float fTargetVolume, _float fDuration);
	_bool FadeOutAndStop(SOUND_ID iSoundID, _float fDuration);
	_bool SetPitch(SOUND_ID iSoundID, _float fPitch);
	_bool Set3DAttributes(SOUND_ID iSoundID, const _float3& vPosition, const _float3& vVelocity = {});
	_bool Set3DMinMaxDistance(SOUND_ID iSoundID, _float fMinDistance, _float fMaxDistance);
	_bool IsPlaying(SOUND_ID iSoundID) const;
	_bool IsPaused(SOUND_ID iSoundID) const;
	_bool IsValidSound(SOUND_ID iSoundID) const;
	void Set3DDebugRenderEnabled(_bool bEnabled) { m_b3DDebugRenderEnabled = bEnabled; }
	_bool Is3DDebugRenderEnabled() const { return m_b3DDebugRenderEnabled; }

public:
	_bool CreateBus(const SOUND_BUS_ID& sBusID);
	_bool RemoveBus(const SOUND_BUS_ID& sBusID);
	_bool SetListenerAttributes(uint32_t iListenerIndex, const SOUND_LISTENER_DESC& tDesc);
	_bool SetBusVolume(const SOUND_BUS_ID& sBusID, _float fVolume);
	_bool FadeBusTo(const SOUND_BUS_ID& sBusID, _float fTargetVolume, _float fDuration);
	_bool SetBusMuted(const SOUND_BUS_ID& sBusID, _bool bMuted);
	_bool SetBusPaused(const SOUND_BUS_ID& sBusID, _bool bPaused);
	_bool StopBus(const SOUND_BUS_ID& sBusID);

private:
	SPtr<CResFmodSound> GetOrLoadResourceByPath(const _string& sPath, SOUND_LOAD_TYPE eLoadType);
	SOUND_ID PlayInternal(const SPtr<CResFmodSound>& pSound, const SOUND_PLAY_DESC& tDesc,
		const SOUND_3D_DESC* p3DDesc);
	SOUND_ID GenerateSoundID();
	FMOD_CHANNELGROUP* GetBus(const SOUND_BUS_ID& sBusID) const;
	_bool StopSoundsByPath(const _string& sNormalizedPath);
	_bool StopSoundsByBus(const SOUND_BUS_ID& sBusID);
	void StopAllSounds();
	void EnqueueCompletedSound(SOUND_ID iSoundID);
	void FlushCompletedSounds();
	void Draw3DSoundDebug();
	_bool ScheduleChannelFade(SPlayingSound& tPlayingSound, _float fTargetVolume,
		_float fDuration, _bool bStopAtEnd);
	_bool ScheduleBusFade(FMOD_CHANNELGROUP* pBus, _float fTargetVolume,
		_float fDuration);

private:
	std::mutex m_SoundResourceRegistrationMutex{};

	std::unordered_map<SOUND_ID, SPlayingSound> m_mapPlayingSounds{};
	std::unordered_map<SOUND_BUS_ID, FMOD_CHANNELGROUP*> m_SoundBuses{};
	SOUND_ID m_iNextSoundID{ 1 };

	std::mutex m_CompletedSoundMutex{};
	std::queue<SOUND_ID> m_CompletedSounds{};

	FMOD_SYSTEM* m_pSystem{};
	int m_iSoftwareSampleRate{};
	_bool m_b3DDebugRenderEnabled{};

public:
	static UPtr<CSoundManager> Create();

private:
	void Free() override;
};

NS_END
