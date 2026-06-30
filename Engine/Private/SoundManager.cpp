#include "pch.h"
#include "SoundManager.h"
#include "fmod.h"
//#include "fmod.hpp"
#include "fmod_common.h"
//#include <fmod.hpp>
NS_USING(Engine)

CSoundManager::CSoundManager()
{
}

CSoundManager::~CSoundManager()
{
}

void CSoundManager::UpdateGUI()
{
	ImGui::Begin("SoundManager");
	if (ImGui::TreeNode("Channels"))
	{
		
		for (const auto& [k, v] : m_mapChannels)
		{
			if (ImGui::TreeNode(k.GetDbgStr()))
			{
				// 1. 음량 가져오기
				float fVolume = 0.0f;
				FMOD_Channel_GetVolume(v.pChannel, &fVolume);

				// 2. 일시정지 상태 확인
				FMOD_BOOL bPaused = false;
				FMOD_Channel_GetPaused(v.pChannel, &bPaused);

				// 3. 현재 재생 위치 가져오기 (단위: 밀리초)
				unsigned int nMs = 0;
				FMOD_Channel_GetPosition(v.pChannel, &nMs, FMOD_TIMEUNIT_MS);

				// 4. 주파수(피치) 가져오기
				float fFrequency = 0.0f;
				FMOD_Channel_GetFrequency(v.pChannel, &fFrequency);

				ImGui::Text(k.GetDbgStr());
				ImGui::Text("isPlayer: %i", IsPlaying(k));
				ImGui::Text("fVolume: %f", fVolume);
				ImGui::Text("bPaused: %i", bPaused);
				ImGui::Text("nMs: %i", nMs);
				ImGui::Text("fFrequency: %f", fFrequency);
				if (ImGui::Button("Play"))
				{
					Play(k);
				}
				ImGui::SameLine();
				if (ImGui::Button("Stop"))
				{
					Stop(k);
				}
				ImGui::SameLine();
				if (ImGui::Button("Pause"))
				{
					Pause(k, true);
				}
				ImGui::SameLine();
				if (ImGui::Button("Resume"))
				{
					Pause(k, false);
				}
				ImGui::SameLine();
				if (ImGui::Button("PlayLoop"))
				{
					//PlayLoop(k);
				}
				_float fCurrVol;
				if (GetVolume(k, fCurrVol))
				{
					if (ImGui::DragFloat("Vol", &fCurrVol, 0.1f, 0.f, 1.f))
					{
						SetVolume(k, fCurrVol);
					}
				}


				ImGui::TreePop();
			}
			//ImGui::Separator();
			
			
		}

		ImGui::TreePop();
	}


	ImGui::End();
}

HRESULT CSoundManager::Initialize()
{
	// 사운드를 담당하는 대표객체를 생성하는 함수
	FMOD_System_Create(&m_pSystem, FMOD_VERSION);

	FMOD_System_SetDSPBufferSize(m_pSystem, 2048, 4);

	// 1. 시스템 포인터, 2. 사용할 가상채널 수 , 초기화 방식) 
	FMOD_System_Init(m_pSystem, 32, FMOD_INIT_NORMAL, NULL);

	return S_OK;
}

void CSoundManager::Update()
{
	FMOD_System_Update(m_pSystem);
}

HRESULT CSoundManager::CreateSound(const _string& sPath, FMOD_SOUND** ppSound)
{
	FMOD_RESULT eRes = FMOD_System_CreateSound(
		m_pSystem,
		sPath.c_str(),
		FMOD_CREATESAMPLE, 0, ppSound);

	if (eRes != FMOD_OK)
	{
		return E_FAIL;
	}
	return S_OK;
}

const CSoundManager::SChannel* CSoundManager::GetChannel(const StringID& channelTag) const
{
	auto iter = m_mapChannels.find(channelTag);
	if (iter == m_mapChannels.end())
	{
		return nullptr;
	}
	return &iter->second;
}

HRESULT CSoundManager::AddChannel(const StringID& channelTag, const std::pair<StringID, StringID>& soundResources)
{
	SPtr< CResFmodSound> soundRes{};
	if (auto res = CGameInstance::Get().GetResourceFirst<CResFmodSound>(soundResources.first, soundResources.second))
	{
		soundRes = res;
	}
	if(!soundRes)
	{
		return E_FAIL;
	}

	auto iter = m_mapChannels.find(channelTag);
	if (iter != m_mapChannels.end())
	{
		iter->second.vecRes.push_back({ soundResources.first, soundResources.second , soundRes });
	}

	m_mapChannels.emplace(channelTag, SChannel{ .vecRes = {{soundResources.first, soundResources.second , soundRes}} });

	return S_OK;
}

HRESULT CSoundManager::Play(const StringID& channelTag)
{
	auto iter = m_mapChannels.find(channelTag);
	if (iter == m_mapChannels.end())
	{
		return E_FAIL;
	}
	
	for (auto& [_,__,pSound] : iter->second.vecRes)
	{
		FMOD_System_PlaySound(
			m_pSystem,
			pSound->GetSound(),
			NULL,
			false,
			&iter->second.pChannel
		);
	}
	
	return S_OK;
}

HRESULT CSoundManager::Play(const StringID& channelTag, _float fVolume)
{
	auto iter = m_mapChannels.find(channelTag);
	if (iter == m_mapChannels.end())
	{
		return E_FAIL;
	}

	for (auto& [_, __, pSound] : iter->second.vecRes)
	{
		FMOD_System_PlaySound(
			m_pSystem,
			pSound->GetSound(),
			NULL,
			false,
			&iter->second.pChannel
		);
		FMOD_Channel_SetVolume(iter->second.pChannel, fVolume);
	}

	return S_OK;
}

HRESULT CSoundManager::Play(const StringID& channelTag, _float fVolume, _float fPitch)
{
	if (fVolume == 0.f)
	{
		return S_OK;
	}
	auto iter = m_mapChannels.find(channelTag);
	if (iter == m_mapChannels.end())
	{
		return E_FAIL;
	}

	for (auto& [_, __, pSound] : iter->second.vecRes)
	{
		FMOD_System_PlaySound(
			m_pSystem,
			pSound->GetSound(),
			NULL,
			false,
			&iter->second.pChannel
		);
		FMOD_Channel_SetVolume(iter->second.pChannel, fVolume);

		// 1. 해당 채널의 기본 주파수(Frequency)를 가져옵니다.
		float fBaseFrequency = 0.0f;
		FMOD_Channel_GetFrequency(iter->second.pChannel, &fBaseFrequency);

		// 2. 비율을 곱하여 새로운 주파수 설정
		// fPitchRatio가 1.2면 20% 상승, 0.8이면 20% 하락
		FMOD_Channel_SetFrequency(iter->second.pChannel, fBaseFrequency * fPitch);
	}

	return S_OK;
}

HRESULT CSoundManager::PlayLoop(const StringID& channelTag, _float fVolume)
{
	auto iter = m_mapChannels.find(channelTag);
	if (iter == m_mapChannels.end())
	{
		return E_FAIL;
	}

	for (auto& [_, __, pSound] : iter->second.vecRes)
	{
		FMOD_System_PlaySound(
			m_pSystem,
			pSound->GetSound(),
			NULL,
			false,
			&iter->second.pChannel
		);
		FMOD_Channel_SetMode(iter->second.pChannel, FMOD_LOOP_NORMAL);
		FMOD_Channel_SetVolume(iter->second.pChannel, fVolume);
	}

	return S_OK;
}

void CSoundManager::Stop(const StringID& channelTag)
{
	auto iter = m_mapChannels.find(channelTag);
	if (iter != m_mapChannels.end() && iter->second.pChannel != nullptr)
	{
		FMOD_Channel_Stop(iter->second.pChannel);
		iter->second.pChannel = nullptr;
	}
}

void CSoundManager::Pause(const StringID& channelTag, _bool bPause)
{
	auto iter = m_mapChannels.find(channelTag);
	if (iter != m_mapChannels.end() && iter->second.pChannel != nullptr)
	{
		FMOD_Channel_SetPaused(iter->second.pChannel, bPause);
	}
}

_bool CSoundManager::GetVolume(const StringID& channelTag, _float& fVolume)
{
	auto iter = m_mapChannels.find(channelTag);
	if (iter != m_mapChannels.end() && iter->second.pChannel != nullptr)
	{
		float tmp{};
		FMOD_Channel_GetVolume(iter->second.pChannel, &tmp);
		fVolume = tmp;
		return true;
	}

	return false;
}

_bool CSoundManager::SetVolume(const StringID& channelTag, _float fVolume)
{
	auto iter = m_mapChannels.find(channelTag);
	if (iter != m_mapChannels.end() && iter->second.pChannel != nullptr)
	{
		FMOD_Channel_SetVolume(iter->second.pChannel, fVolume);
		return true;
	}
	return false;
}

_bool CSoundManager::IsPlaying(const StringID& channelTag) const
{
	auto iter = m_mapChannels.find(channelTag);
	if (iter == m_mapChannels.end() || iter->second.pChannel == nullptr)
	{
		return false;
	}

	FMOD_BOOL bIsPlaying = false;
	FMOD_Channel_IsPlaying(iter->second.pChannel, &bIsPlaying);

	return (bIsPlaying == true);
}

void CSoundManager::SetPitch(const StringID& channelTag, float fPitchRatio)
{
	auto iter = m_mapChannels.find(channelTag);
	if (iter != m_mapChannels.end() && iter->second.pChannel != nullptr)
	{
		// 1. 해당 채널의 기본 주파수(Frequency)를 가져옵니다.
		float fBaseFrequency = 0.0f;
		FMOD_Channel_GetFrequency(iter->second.pChannel, &fBaseFrequency);

		// 2. 비율을 곱하여 새로운 주파수 설정
		// fPitchRatio가 1.2면 20% 상승, 0.8이면 20% 하락
		FMOD_Channel_SetFrequency(iter->second.pChannel, fBaseFrequency * fPitchRatio);
	}
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
	// 1. 모든 채널 및 사운드 리소스 정리
	// m_mapChannels에 저장된 모든 사운드 리소스를 순회하며 Release 호출
	for (auto& pair : m_mapChannels)
	{
		for (auto& [_, __, pSoundRes] : pair.second.vecRes)
		{
			if (pSoundRes->GetSound() != nullptr)
			{
				FMOD_Sound_Release(pSoundRes->GetSound());
			}
		}
	}
	m_mapChannels.clear();

	// 2. FMOD 시스템 종료
	if (m_pSystem)
	{
		FMOD_System_Close(m_pSystem);    // 사운드 시스템 중지
		FMOD_System_Release(m_pSystem);  // 시스템 자원 해제
		m_pSystem = nullptr;
	}

	CEngineBase::Free();
}
