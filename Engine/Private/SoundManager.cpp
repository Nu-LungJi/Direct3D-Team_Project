#include "pch.h"
#include "SoundManager.h"

#include "GameInstance.h"
#include "ResFmodSound.h"

#include "fmod.h"

NS_USING(Engine)

NS_BEGIN(Engine)

struct SSoundCallbackBridge
{
	static FMOD_RESULT F_CALL ChannelCallback(
		FMOD_CHANNELCONTROL* pChannelControl,
		FMOD_CHANNELCONTROL_TYPE eControlType,
		FMOD_CHANNELCONTROL_CALLBACK_TYPE eCallbackType,
		void* pCommandData1,
		void* pCommandData2);
};

NS_END

namespace
{
	constexpr int32_t SOUND_MAX_CHANNELS = 256;
	constexpr _float SOUND_DOPPLER_SCALE = 1.f;
	constexpr _float SOUND_DISTANCE_FACTOR = 1.f;
	constexpr _float SOUND_ROLLOFF_SCALE = 1.f;

	const StringID& GetSoundResourceGroupTag()
	{
		static const StringID sGroupTag{ "SOUND_RESOURCE" };
		return sGroupTag;
	}

	FMOD_VECTOR ToFmodVector(const _float3& vValue)
	{
		return { vValue.x, vValue.y, vValue.z };
	}

	FMOD_MODE ToFmodRolloffMode(SOUND_3D_ROLLOFF eRolloff)
	{
		switch (eRolloff)
		{
		case SOUND_3D_ROLLOFF::LINEAR:
			return FMOD_3D_LINEARROLLOFF;

		case SOUND_3D_ROLLOFF::INVERSE:
		default:
			return FMOD_3D_INVERSEROLLOFF;
		}
	}

	const char* GetBusName(SOUND_BUS eBus)
	{
		switch (eBus)
		{
		case SOUND_BUS::MASTER:   return "Master";
		case SOUND_BUS::BGM:      return "BGM";
		case SOUND_BUS::SFX:      return "SFX";
		case SOUND_BUS::VOICE:    return "Voice";
		case SOUND_BUS::UI:       return "UI";
		case SOUND_BUS::AMBIENCE: return "Ambience";
		default:                  return "Invalid";
		}
	}
}

CSoundManager::CSoundManager()
{
}

CSoundManager::~CSoundManager()
{
}

void CSoundManager::UpdateGUI()
{
	SOUND_ID iStopSoundID{ INVALID_SOUND_ID };
	SOUND_BUS eStopBus{ SOUND_BUS::END };
	_bool bClearAll{};

	if (!ImGui::Begin("SoundManager"))
	{
		ImGui::End();
		return;
	}

	ImGui::Text("Active Sounds: %zu", m_mapPlayingSounds.size());
	ImGui::SameLine();
	if (ImGui::Button("Stop && Clear All"))
		bClearAll = true;

	if (ImGui::TreeNode("Buses"))
	{
		for (size_t i = 0; i < static_cast<size_t>(SOUND_BUS::END); ++i)
		{
			const SOUND_BUS eBus = static_cast<SOUND_BUS>(i);
			FMOD_CHANNELGROUP* pBus = GetBus(eBus);
			if (pBus == nullptr)
				continue;

			ImGui::PushID(static_cast<int>(i));
			if (ImGui::TreeNode(GetBusName(eBus)))
			{
				_float fVolume{ 1.f };
				FMOD_BOOL bMuted{};
				FMOD_BOOL bPaused{};
				FMOD_ChannelGroup_GetVolume(pBus, &fVolume);
				FMOD_ChannelGroup_GetMute(pBus, &bMuted);
				FMOD_ChannelGroup_GetPaused(pBus, &bPaused);

				if (ImGui::DragFloat("Volume", &fVolume, 0.01f, 0.f, 2.f, "%.2f"))
					SetBusVolume(eBus, fVolume);

				_bool bMutedValue = bMuted != false;
				if (ImGui::Checkbox("Muted", &bMutedValue))
					SetBusMuted(eBus, bMutedValue);

				_bool bPausedValue = bPaused != false;
				if (ImGui::Checkbox("Paused", &bPausedValue))
					SetBusPaused(eBus, bPausedValue);

				if (ImGui::Button("Stop Bus"))
					eStopBus = eBus;

				ImGui::TreePop();
			}
			ImGui::PopID();
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Playing Sounds"))
	{
		for (const auto& [iSoundID, tSound] : m_mapPlayingSounds)
		{
			ImGui::PushID(reinterpret_cast<void*>(static_cast<uintptr_t>(iSoundID)));
			const _string sLabel = "Sound " + std::to_string(iSoundID);
			if (ImGui::TreeNode(sLabel.c_str()))
			{
				FMOD_BOOL bPlaying{};
				FMOD_BOOL bPaused{};
				_float fVolume{ 1.f };
				_float fPitch{ 1.f };
				FMOD_Channel_IsPlaying(tSound.pChannel, &bPlaying);
				FMOD_Channel_GetPaused(tSound.pChannel, &bPaused);
				FMOD_Channel_GetVolume(tSound.pChannel, &fVolume);
				FMOD_Channel_GetPitch(tSound.pChannel, &fPitch);

				ImGui::Text("Path: %s", tSound.pSound != nullptr ? tSound.pSound->GetPath().c_str() : "<null>");
				ImGui::Text("Bus: %s | Type: %s | Playing: %s",
					GetBusName(tSound.eBus), tSound.b3D ? "3D" : "2D", bPlaying ? "Yes" : "No");

				_bool bPausedValue = bPaused != false;
				if (ImGui::Checkbox("Paused", &bPausedValue))
					SetPaused(iSoundID, bPausedValue);

				if (ImGui::DragFloat("Volume", &fVolume, 0.01f, 0.f, 2.f, "%.2f"))
					SetVolume(iSoundID, fVolume);

				if (ImGui::DragFloat("Pitch", &fPitch, 0.01f, 0.01f, 4.f, "%.2f"))
					SetPitch(iSoundID, fPitch);

				if (tSound.b3D)
				{
					FMOD_VECTOR vPosition{};
					FMOD_VECTOR vVelocity{};
					_float fMinDistance{};
					_float fMaxDistance{};
					FMOD_Channel_Get3DAttributes(tSound.pChannel, &vPosition, &vVelocity);
					FMOD_Channel_Get3DMinMaxDistance(tSound.pChannel, &fMinDistance, &fMaxDistance);

					_float3 vEnginePosition{ vPosition.x, vPosition.y, vPosition.z };
					_float3 vEngineVelocity{ vVelocity.x, vVelocity.y, vVelocity.z };
					const _bool bPositionChanged = ImGui::DragFloat3("Position", &vEnginePosition.x, 0.05f);
					const _bool bVelocityChanged = ImGui::DragFloat3("Velocity", &vEngineVelocity.x, 0.05f);
					if (bPositionChanged || bVelocityChanged)
						Set3DAttributes(iSoundID, vEnginePosition, vEngineVelocity);

					const _bool bMinChanged = ImGui::DragFloat("Min Distance", &fMinDistance, 0.1f, 0.f, fMaxDistance);
					const _bool bMaxChanged = ImGui::DragFloat("Max Distance", &fMaxDistance, 0.1f, fMinDistance, 10000.f);
					if ((bMinChanged || bMaxChanged) && fMaxDistance > fMinDistance)
						Set3DMinMaxDistance(iSoundID, fMinDistance, fMaxDistance);
				}

				if (ImGui::Button("Stop"))
					iStopSoundID = iSoundID;

				ImGui::TreePop();
			}
			ImGui::PopID();
		}

		ImGui::TreePop();
	}

	ImGui::End();

	if (bClearAll)
	{
		ClearResources();
		return;
	}

	if (iStopSoundID != INVALID_SOUND_ID)
		Stop(iStopSoundID);
	if (eStopBus != SOUND_BUS::END)
		StopBus(eStopBus);
}

HRESULT CSoundManager::Initialize()
{
	if (FMOD_System_Create(&m_pSystem, FMOD_VERSION) != FMOD_OK)
		return E_FAIL;

	if (FMOD_System_SetUserData(m_pSystem, this) != FMOD_OK)
		return E_FAIL;

	if (FMOD_System_SetDSPBufferSize(m_pSystem, 2048, 4) != FMOD_OK)
		return E_FAIL;

	if (FMOD_System_Init(m_pSystem, SOUND_MAX_CHANNELS, FMOD_INIT_NORMAL, nullptr) != FMOD_OK)
		return E_FAIL;

	if (FMOD_System_Set3DSettings(m_pSystem, SOUND_DOPPLER_SCALE, SOUND_DISTANCE_FACTOR, SOUND_ROLLOFF_SCALE) != FMOD_OK)
		return E_FAIL;

	if (FMOD_System_GetMasterChannelGroup(m_pSystem, &m_pBuses[static_cast<size_t>(SOUND_BUS::MASTER)]) != FMOD_OK)
		return E_FAIL;

	FMOD_CHANNELGROUP* pMasterBus = m_pBuses[static_cast<size_t>(SOUND_BUS::MASTER)];
	for (size_t i = static_cast<size_t>(SOUND_BUS::BGM); i < static_cast<size_t>(SOUND_BUS::END); ++i)
	{
		const SOUND_BUS eBus = static_cast<SOUND_BUS>(i);
		if (FMOD_System_CreateChannelGroup(m_pSystem, GetBusName(eBus), &m_pBuses[i]) != FMOD_OK)
			return E_FAIL;

		if (FMOD_ChannelGroup_AddGroup(pMasterBus, m_pBuses[i], false, nullptr) != FMOD_OK)
			return E_FAIL;
	}

	return S_OK;
}

void CSoundManager::Update()
{
	if (m_pSystem == nullptr)
		return;

	FMOD_System_Update(m_pSystem);
	FlushCompletedSounds();
}

HRESULT CSoundManager::CreateSound(const _string& sPath, FMOD_SOUND** ppSound, SOUND_LOAD_TYPE eLoadType)
{
	if (m_pSystem == nullptr || ppSound == nullptr)
		return E_FAIL;

	const FMOD_MODE eMode = eLoadType == SOUND_LOAD_TYPE::STREAM ? FMOD_CREATESTREAM : FMOD_CREATESAMPLE;
	return FMOD_System_CreateSound(m_pSystem, sPath.c_str(), eMode, nullptr, ppSound) == FMOD_OK ? S_OK : E_FAIL;
}

_bool CSoundManager::Preload(const _string& sPath, SOUND_LOAD_TYPE eLoadType)
{
	return GetOrLoadResourceByPath(sPath, eLoadType) != nullptr;
}

_bool CSoundManager::RemoveResourceByPath(const _string& sPath)
{
	if (sPath.empty())
		return false;

	const _string sNormalizedPath = std::filesystem::path{ sPath }.lexically_normal().generic_string();
	const StringID sPathTag{ sNormalizedPath };
	std::lock_guard lock{ m_SoundResourceRegistrationMutex };
	const _bool bStoppedSound = StopSoundsByPath(sNormalizedPath);
	const _bool bHasResource = !CGameInstance::Get().GetResource(GetSoundResourceGroupTag(), sPathTag).empty();

	if (bHasResource)
		CGameInstance::Get().DelResource(GetSoundResourceGroupTag(), sPathTag);

	return bStoppedSound || bHasResource;
}

void CSoundManager::ClearResources()
{
	std::lock_guard lock{ m_SoundResourceRegistrationMutex };
	StopAllSounds();
	CGameInstance::Get().DelResource(GetSoundResourceGroupTag());
}

SOUND_ID CSoundManager::Play2D(const _string& sPath, const SOUND_PLAY_DESC& tDesc,
	SOUND_LOAD_TYPE eLoadType)
{
	return PlayInternal(GetOrLoadResourceByPath(sPath, eLoadType), tDesc, nullptr);
}

SOUND_ID CSoundManager::Play3D(const _string& sPath, const SOUND_3D_DESC& t3DDesc,
	const SOUND_PLAY_DESC& tPlayDesc, SOUND_LOAD_TYPE eLoadType)
{
	if (t3DDesc.fMinDistance < 0.f || t3DDesc.fMaxDistance <= t3DDesc.fMinDistance)
		return INVALID_SOUND_ID;

	return PlayInternal(GetOrLoadResourceByPath(sPath, eLoadType), tPlayDesc, &t3DDesc);
}

SPtr<CResFmodSound> CSoundManager::GetOrLoadResourceByPath(const _string& sPath, SOUND_LOAD_TYPE eLoadType)
{
	if (sPath.empty())
		return nullptr;

	const _string sNormalizedPath = std::filesystem::path{ sPath }.lexically_normal().generic_string();
	auto pSound = CGameInstance::Get().GetOrCreateResourceByPath<CResFmodSound>(sNormalizedPath,
		[&sNormalizedPath, eLoadType]() -> SPtr<CResFmodSound>
		{
			auto pCreatedSound = CResFmodSound::Create(sNormalizedPath, eLoadType);
			if (pCreatedSound == nullptr || FAILED(pCreatedSound->Load()))
				return nullptr;

			return pCreatedSound;
		});

	if (pSound == nullptr)
		return nullptr;

	if (pSound->GetState() != CResource::STATE::LOADED)
	{
		if (pSound->GetState() == CResource::STATE::LOADING || FAILED(pSound->Load()))
			return nullptr;
	}

	const StringID sPathTag{ sNormalizedPath };
	std::lock_guard lock{ m_SoundResourceRegistrationMutex };
	const auto registeredResources = CGameInstance::Get().GetResource(GetSoundResourceGroupTag(), sPathTag);
	const _bool bAlreadyRegistered = std::ranges::any_of(registeredResources,
		[&pSound](const SPtr<CResource>& pResource)
		{
			return pResource == pSound;
		});

	if (!bAlreadyRegistered &&
		CGameInstance::Get().AddResource(GetSoundResourceGroupTag(), sPathTag, pSound) == nullptr)
	{
		return nullptr;
	}

	return pSound;
}

SOUND_ID CSoundManager::PlayInternal(const SPtr<CResFmodSound>& pSound, const SOUND_PLAY_DESC& tDesc,
	const SOUND_3D_DESC* p3DDesc)
{
	if (m_pSystem == nullptr || pSound == nullptr || pSound->GetSound() == nullptr)
		return INVALID_SOUND_ID;

	FMOD_CHANNELGROUP* pBus = GetBus(tDesc.eBus);
	if (pBus == nullptr)
		return INVALID_SOUND_ID;

	FMOD_CHANNEL* pChannel{};
	if (FMOD_System_PlaySound(m_pSystem, pSound->GetSound(), pBus, true, &pChannel) != FMOD_OK || pChannel == nullptr)
		return INVALID_SOUND_ID;

	auto FailPlay = [pChannel]()
	{
		FMOD_Channel_Stop(pChannel);
		return INVALID_SOUND_ID;
	};

	FMOD_MODE eMode = tDesc.bLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
	if (p3DDesc != nullptr)
		eMode |= FMOD_3D | FMOD_3D_WORLDRELATIVE | ToFmodRolloffMode(p3DDesc->eRolloff);
	else
		eMode |= FMOD_2D;

	if (FMOD_Channel_SetMode(pChannel, eMode) != FMOD_OK)
		return FailPlay();

	if (FMOD_Channel_SetVolume(pChannel, std::max(0.f, tDesc.fVolume)) != FMOD_OK)
		return FailPlay();

	if (FMOD_Channel_SetPitch(pChannel, std::max(0.01f, tDesc.fPitch)) != FMOD_OK)
		return FailPlay();

	if (FMOD_Channel_SetPriority(pChannel, std::clamp(tDesc.iPriority, 0, 256)) != FMOD_OK)
		return FailPlay();

	if (p3DDesc != nullptr)
	{
		const FMOD_VECTOR vPosition = ToFmodVector(p3DDesc->vPosition);
		const FMOD_VECTOR vVelocity = ToFmodVector(p3DDesc->vVelocity);

		if (FMOD_Channel_Set3DAttributes(pChannel, &vPosition, &vVelocity) != FMOD_OK)
			return FailPlay();

		if (FMOD_Channel_Set3DMinMaxDistance(pChannel, p3DDesc->fMinDistance, p3DDesc->fMaxDistance) != FMOD_OK)
			return FailPlay();
	}

	const SOUND_ID iSoundID = GenerateSoundID();
	if (iSoundID == INVALID_SOUND_ID)
		return FailPlay();

	if (FMOD_Channel_SetUserData(pChannel, reinterpret_cast<void*>(static_cast<uintptr_t>(iSoundID))) != FMOD_OK)
		return FailPlay();

	if (FMOD_Channel_SetCallback(pChannel, &SSoundCallbackBridge::ChannelCallback) != FMOD_OK)
		return FailPlay();

	m_mapPlayingSounds.emplace(iSoundID, SPlayingSound{
		.pSound = pSound,
		.pChannel = pChannel,
		.eBus = tDesc.eBus,
		.b3D = p3DDesc != nullptr
	});

	if (FMOD_Channel_SetPaused(pChannel, tDesc.bStartPaused) != FMOD_OK)
	{
		m_mapPlayingSounds.erase(iSoundID);
		return FailPlay();
	}

	return iSoundID;
}

_bool CSoundManager::Stop(SOUND_ID iSoundID)
{
	const auto iter = m_mapPlayingSounds.find(iSoundID);
	if (iter == m_mapPlayingSounds.end())
		return false;

	const FMOD_RESULT eResult = FMOD_Channel_Stop(iter->second.pChannel);
	m_mapPlayingSounds.erase(iter);
	return eResult == FMOD_OK;
}

_bool CSoundManager::SetPaused(SOUND_ID iSoundID, _bool bPaused)
{
	const auto iter = m_mapPlayingSounds.find(iSoundID);
	return iter != m_mapPlayingSounds.end() && FMOD_Channel_SetPaused(iter->second.pChannel, bPaused) == FMOD_OK;
}

_bool CSoundManager::SetVolume(SOUND_ID iSoundID, _float fVolume)
{
	const auto iter = m_mapPlayingSounds.find(iSoundID);
	return iter != m_mapPlayingSounds.end() && FMOD_Channel_SetVolume(iter->second.pChannel, std::max(0.f, fVolume)) == FMOD_OK;
}

_bool CSoundManager::SetPitch(SOUND_ID iSoundID, _float fPitch)
{
	const auto iter = m_mapPlayingSounds.find(iSoundID);
	return iter != m_mapPlayingSounds.end() && FMOD_Channel_SetPitch(iter->second.pChannel, std::max(0.01f, fPitch)) == FMOD_OK;
}

_bool CSoundManager::Set3DAttributes(SOUND_ID iSoundID, const _float3& vPosition, const _float3& vVelocity)
{
	const auto iter = m_mapPlayingSounds.find(iSoundID);
	if (iter == m_mapPlayingSounds.end() || !iter->second.b3D)
		return false;

	const FMOD_VECTOR vFmodPosition = ToFmodVector(vPosition);
	const FMOD_VECTOR vFmodVelocity = ToFmodVector(vVelocity);
	return FMOD_Channel_Set3DAttributes(iter->second.pChannel, &vFmodPosition, &vFmodVelocity) == FMOD_OK;
}

_bool CSoundManager::Set3DMinMaxDistance(SOUND_ID iSoundID, _float fMinDistance, _float fMaxDistance)
{
	if (fMinDistance < 0.f || fMaxDistance <= fMinDistance)
		return false;

	const auto iter = m_mapPlayingSounds.find(iSoundID);
	return iter != m_mapPlayingSounds.end() && iter->second.b3D &&
		FMOD_Channel_Set3DMinMaxDistance(iter->second.pChannel, fMinDistance, fMaxDistance) == FMOD_OK;
}

_bool CSoundManager::IsPlaying(SOUND_ID iSoundID) const
{
	const auto iter = m_mapPlayingSounds.find(iSoundID);
	if (iter == m_mapPlayingSounds.end())
		return false;

	FMOD_BOOL bPlaying{};
	return FMOD_Channel_IsPlaying(iter->second.pChannel, &bPlaying) == FMOD_OK && bPlaying;
}

_bool CSoundManager::IsPaused(SOUND_ID iSoundID) const
{
	const auto iter = m_mapPlayingSounds.find(iSoundID);
	if (iter == m_mapPlayingSounds.end())
		return false;

	FMOD_BOOL bPaused{};
	return FMOD_Channel_GetPaused(iter->second.pChannel, &bPaused) == FMOD_OK && bPaused;
}

_bool CSoundManager::IsValidSound(SOUND_ID iSoundID) const
{
	return iSoundID != INVALID_SOUND_ID && m_mapPlayingSounds.contains(iSoundID);
}

_bool CSoundManager::SetListenerAttributes(uint32_t iListenerIndex, const SOUND_LISTENER_DESC& tDesc)
{
	if (m_pSystem == nullptr)
		return false;

	const FMOD_VECTOR vPosition = ToFmodVector(tDesc.vPosition);
	const FMOD_VECTOR vVelocity = ToFmodVector(tDesc.vVelocity);
	const FMOD_VECTOR vForward = ToFmodVector(tDesc.vForward);
	const FMOD_VECTOR vUp = ToFmodVector(tDesc.vUp);

	return FMOD_System_Set3DListenerAttributes(m_pSystem, static_cast<int>(iListenerIndex),
		&vPosition, &vVelocity, &vForward, &vUp) == FMOD_OK;
}

_bool CSoundManager::SetBusVolume(SOUND_BUS eBus, _float fVolume)
{
	FMOD_CHANNELGROUP* pBus = GetBus(eBus);
	return pBus != nullptr && FMOD_ChannelGroup_SetVolume(pBus, std::max(0.f, fVolume)) == FMOD_OK;
}

_bool CSoundManager::SetBusMuted(SOUND_BUS eBus, _bool bMuted)
{
	FMOD_CHANNELGROUP* pBus = GetBus(eBus);
	return pBus != nullptr && FMOD_ChannelGroup_SetMute(pBus, bMuted) == FMOD_OK;
}

_bool CSoundManager::SetBusPaused(SOUND_BUS eBus, _bool bPaused)
{
	FMOD_CHANNELGROUP* pBus = GetBus(eBus);
	return pBus != nullptr && FMOD_ChannelGroup_SetPaused(pBus, bPaused) == FMOD_OK;
}

_bool CSoundManager::StopBus(SOUND_BUS eBus)
{
	FMOD_CHANNELGROUP* pBus = GetBus(eBus);
	return pBus != nullptr && FMOD_ChannelGroup_Stop(pBus) == FMOD_OK;
}

SOUND_ID CSoundManager::GenerateSoundID()
{
	for (;;)
	{
		const SOUND_ID iSoundID = m_iNextSoundID++;
		if (iSoundID != INVALID_SOUND_ID && !m_mapPlayingSounds.contains(iSoundID))
			return iSoundID;
	}
}

FMOD_CHANNELGROUP* CSoundManager::GetBus(SOUND_BUS eBus) const
{
	const size_t iIndex = static_cast<size_t>(eBus);
	return iIndex < m_pBuses.size() ? m_pBuses[iIndex] : nullptr;
}

_bool CSoundManager::StopSoundsByPath(const _string& sNormalizedPath)
{
	_bool bStopped{};
	for (auto iter = m_mapPlayingSounds.begin(); iter != m_mapPlayingSounds.end();)
	{
		const auto& pSound = iter->second.pSound;
		const _string sSoundPath = pSound != nullptr
			? std::filesystem::path{ pSound->GetPath() }.lexically_normal().generic_string()
			: _string{};

		if (sSoundPath != sNormalizedPath)
		{
			++iter;
			continue;
		}

		if (iter->second.pChannel != nullptr)
			FMOD_Channel_Stop(iter->second.pChannel);

		iter = m_mapPlayingSounds.erase(iter);
		bStopped = true;
	}

	return bStopped;
}

void CSoundManager::StopAllSounds()
{
	for (const auto& [_, tSound] : m_mapPlayingSounds)
	{
		if (tSound.pChannel != nullptr)
			FMOD_Channel_Stop(tSound.pChannel);
	}

	m_mapPlayingSounds.clear();
}

void CSoundManager::EnqueueCompletedSound(SOUND_ID iSoundID)
{
	if (iSoundID == INVALID_SOUND_ID)
		return;

	std::lock_guard lock{ m_CompletedSoundMutex };
	m_CompletedSounds.push(iSoundID);
}

void CSoundManager::FlushCompletedSounds()
{
	std::queue<SOUND_ID> completedSounds{};
	{
		std::lock_guard lock{ m_CompletedSoundMutex };
		completedSounds.swap(m_CompletedSounds);
	}

	while (!completedSounds.empty())
	{
		m_mapPlayingSounds.erase(completedSounds.front());
		completedSounds.pop();
	}
}

FMOD_RESULT F_CALL SSoundCallbackBridge::ChannelCallback(
	FMOD_CHANNELCONTROL* pChannelControl,
	FMOD_CHANNELCONTROL_TYPE eControlType,
	FMOD_CHANNELCONTROL_CALLBACK_TYPE eCallbackType,
	void* pCommandData1,
	void* pCommandData2)
{
	UNREFERENCED_PARAMETER(pCommandData1);
	UNREFERENCED_PARAMETER(pCommandData2);

	if (eControlType != FMOD_CHANNELCONTROL_CHANNEL || eCallbackType != FMOD_CHANNELCONTROL_CALLBACK_END)
		return FMOD_OK;

	auto* pChannel = reinterpret_cast<FMOD_CHANNEL*>(pChannelControl);
	void* pSoundUserData{};
	FMOD_SYSTEM* pSystem{};
	void* pSystemUserData{};

	if (FMOD_Channel_GetUserData(pChannel, &pSoundUserData) != FMOD_OK ||
		FMOD_Channel_GetSystemObject(pChannel, &pSystem) != FMOD_OK || pSystem == nullptr ||
		FMOD_System_GetUserData(pSystem, &pSystemUserData) != FMOD_OK || pSystemUserData == nullptr)
	{
		return FMOD_OK;
	}

	auto* pSoundManager = static_cast<CSoundManager*>(pSystemUserData);
	const SOUND_ID iSoundID = static_cast<SOUND_ID>(reinterpret_cast<uintptr_t>(pSoundUserData));
	pSoundManager->EnqueueCompletedSound(iSoundID);
	return FMOD_OK;
}

UPtr<CSoundManager> CSoundManager::Create()
{
	auto pInstance = UPtr<CSoundManager>(new CSoundManager{});
	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CSoundManager");
		return nullptr;
	}

	return pInstance;
}

void CSoundManager::Free()
{
	if (m_pSystem != nullptr)
	{
		if (FMOD_CHANNELGROUP* pMasterBus = GetBus(SOUND_BUS::MASTER))
			FMOD_ChannelGroup_Stop(pMasterBus);

		m_mapPlayingSounds.clear();

		for (size_t i = static_cast<size_t>(SOUND_BUS::BGM); i < static_cast<size_t>(SOUND_BUS::END); ++i)
		{
			if (m_pBuses[i] != nullptr)
				FMOD_ChannelGroup_Release(m_pBuses[i]);
		}

		FMOD_System_SetUserData(m_pSystem, nullptr);
		FMOD_System_Close(m_pSystem);
		FMOD_System_Release(m_pSystem);
		m_pSystem = nullptr;
	}

	m_mapPlayingSounds.clear();
	m_pBuses.fill(nullptr);

	{
		std::lock_guard lock{ m_CompletedSoundMutex };
		std::queue<SOUND_ID>{}.swap(m_CompletedSounds);
	}

	CEngineBase::Free();
}
