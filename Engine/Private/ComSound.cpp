#include "pch.h"
#include "ComSound.h"

#include "GameInstance.h"

NS_USING(Engine)

CComSound::CComSound()
{
}

CComSound::CComSound(const CComSound& rhs)
	: CComponent{ rhs }
{
}

CComSound::~CComSound()
{
}

HRESULT CComSound::Initialize(void* pArg)
{
	if (pArg == nullptr || FAILED(CComponent::Initialize(pArg)))
		return E_FAIL;

	return S_OK;
}

SOUND_ID CComSound::Play2D(const _string& sPath, const SOUND_PLAY_DESC& tPlayDesc,
	SOUND_LOAD_TYPE eLoadType)
{
	return PlayInternal(sPath, tPlayDesc, eLoadType);
}

SOUND_ID CComSound::PlaySlot2D(const StringID& sSlotID, const _string& sPath,
	const SOUND_PLAY_DESC& tPlayDesc, SOUND_SLOT_PLAY_MODE ePlayMode,
	SOUND_LOAD_TYPE eLoadType)
{
	if (sSlotID.hash == 0)
		return INVALID_SOUND_ID;

	if (ePlayMode == SOUND_SLOT_PLAY_MODE::REPLACE)
		StopSlot(sSlotID);

	const SOUND_ID iSoundID = PlayInternal(sPath, tPlayDesc, eLoadType);
	if (iSoundID == INVALID_SOUND_ID)
		return INVALID_SOUND_ID;

	m_Slots[sSlotID].push_back(iSoundID);
	return iSoundID;
}

SOUND_ID CComSound::Play3D(const _string& sPath, const SOUND_3D_DESC& t3DDesc,
	const SOUND_PLAY_DESC& tPlayDesc, SOUND_LOAD_TYPE eLoadType)
{
	return PlayInternal(sPath, t3DDesc, tPlayDesc, eLoadType);
}

SOUND_ID CComSound::PlaySlot3D(const StringID& sSlotID, const _string& sPath,
	const SOUND_3D_DESC& t3DDesc, const SOUND_PLAY_DESC& tPlayDesc,
	SOUND_SLOT_PLAY_MODE ePlayMode, SOUND_LOAD_TYPE eLoadType)
{
	if (sSlotID.hash == 0)
		return INVALID_SOUND_ID;

	if (ePlayMode == SOUND_SLOT_PLAY_MODE::REPLACE)
		StopSlot(sSlotID);

	const SOUND_ID iSoundID = PlayInternal(sPath, t3DDesc, tPlayDesc, eLoadType);
	if (iSoundID == INVALID_SOUND_ID)
		return INVALID_SOUND_ID;

	m_Slots[sSlotID].push_back(iSoundID);
	return iSoundID;
}

_bool CComSound::StopSlot(const StringID& sSlotID)
{
	const auto iter = m_Slots.find(sSlotID);
	if (iter == m_Slots.end())
		return false;

	const auto vecSoundIDs = iter->second;
	_bool bStoppedAny{};
	for (const SOUND_ID iSoundID : vecSoundIDs)
		bStoppedAny |= StopInternal(iSoundID);

	return bStoppedAny;
}

_bool CComSound::FadeSlotTo(
	const StringID& sSlotID, _float fTargetVolume, _float fDuration)
{
	const auto iter = m_Slots.find(sSlotID);
	auto* pSoundManager = CGameInstance::Get().GetSoundManager();
	if (iter == m_Slots.end() || pSoundManager == nullptr)
		return false;

	_bool bSucceeded{ true };
	for (const SOUND_ID iSoundID : iter->second)
	{
		bSucceeded &= pSoundManager->FadeTo(
			iSoundID, fTargetVolume, fDuration);
	}

	return bSucceeded;
}

_bool CComSound::FadeOutAndStopSlot(
	const StringID& sSlotID, _float fDuration)
{
	const auto iter = m_Slots.find(sSlotID);
	auto* pSoundManager = CGameInstance::Get().GetSoundManager();
	if (iter == m_Slots.end() || pSoundManager == nullptr)
		return false;

	_bool bSucceeded{ true };
	for (const SOUND_ID iSoundID : iter->second)
	{
		bSucceeded &= pSoundManager->FadeOutAndStop(
			iSoundID, fDuration);
	}

	return bSucceeded;
}

_bool CComSound::FadeOutAndDetachSlot(
	const StringID& sSlotID, _float fDuration)
{
	const auto iter = m_Slots.find(sSlotID);
	auto* pSoundManager = CGameInstance::Get().GetSoundManager();
	if (iter == m_Slots.end() || pSoundManager == nullptr)
		return false;

	const auto soundIDs = iter->second;
	std::vector<SOUND_ID> failedSoundIDs{};
	failedSoundIDs.reserve(soundIDs.size());

	for (const SOUND_ID iSoundID : soundIDs)
	{
		if (pSoundManager->FadeOutAndStop(iSoundID, fDuration))
		{
			m_PlayingSounds.erase(iSoundID);
			continue;
		}

		failedSoundIDs.push_back(iSoundID);
	}

	const _bool bAllScheduled = failedSoundIDs.empty();
	if (bAllScheduled)
		m_Slots.erase(iter);
	else
		iter->second = std::move(failedSoundIDs);

	return !soundIDs.empty() && bAllScheduled;
}

void CComSound::StopAll()
{
	if (auto* pSoundManager = CGameInstance::Get().GetSoundManager())
	{
		for (const SOUND_ID iSoundID : m_PlayingSounds)
			pSoundManager->Stop(iSoundID);
	}

	m_PlayingSounds.clear();
	m_Slots.clear();
}

_bool CComSound::SetSlotPaused(const StringID& sSlotID, _bool bPaused)
{
	const auto iter = m_Slots.find(sSlotID);
	auto* pSoundManager = CGameInstance::Get().GetSoundManager();
	if (iter == m_Slots.end() || pSoundManager == nullptr)
		return false;

	_bool bSucceeded{ true };
	for (const SOUND_ID iSoundID : iter->second)
		bSucceeded &= pSoundManager->SetPaused(iSoundID, bPaused);
	return bSucceeded;
}

_bool CComSound::SetSlotVolume(const StringID& sSlotID, _float fVolume)
{
	const auto iter = m_Slots.find(sSlotID);
	auto* pSoundManager = CGameInstance::Get().GetSoundManager();
	if (iter == m_Slots.end() || pSoundManager == nullptr)
		return false;

	_bool bSucceeded{ true };
	for (const SOUND_ID iSoundID : iter->second)
		bSucceeded &= pSoundManager->SetVolume(iSoundID, fVolume);
	return bSucceeded;
}

_bool CComSound::SetSlotPitch(const StringID& sSlotID, _float fPitch)
{
	const auto iter = m_Slots.find(sSlotID);
	auto* pSoundManager = CGameInstance::Get().GetSoundManager();
	if (iter == m_Slots.end() || pSoundManager == nullptr)
		return false;

	_bool bSucceeded{ true };
	for (const SOUND_ID iSoundID : iter->second)
		bSucceeded &= pSoundManager->SetPitch(iSoundID, fPitch);
	return bSucceeded;
}

_bool CComSound::SetSlot3DAttributes(const StringID& sSlotID,
	const _float3& vPosition, const _float3& vVelocity)
{
	const auto iter = m_Slots.find(sSlotID);
	auto* pSoundManager = CGameInstance::Get().GetSoundManager();
	if (iter == m_Slots.end() || pSoundManager == nullptr)
		return false;

	_bool bSucceeded{ true };
	for (const SOUND_ID iSoundID : iter->second)
		bSucceeded &= pSoundManager->Set3DAttributes(iSoundID, vPosition, vVelocity);
	return bSucceeded;
}

_bool CComSound::SetSlot3DMinMaxDistance(const StringID& sSlotID,
	_float fMinDistance, _float fMaxDistance)
{
	const auto iter = m_Slots.find(sSlotID);
	auto* pSoundManager = CGameInstance::Get().GetSoundManager();
	if (iter == m_Slots.end() || pSoundManager == nullptr)
		return false;

	_bool bSucceeded{ true };
	for (const SOUND_ID iSoundID : iter->second)
		bSucceeded &= pSoundManager->Set3DMinMaxDistance(
			iSoundID, fMinDistance, fMaxDistance);
	return bSucceeded;
}

_bool CComSound::IsValidSlot(const StringID& sSlotID) const
{
	const auto iter = m_Slots.find(sSlotID);
	const auto* pSoundManager = CGameInstance::Get().GetSoundManager();
	if (iter == m_Slots.end() || pSoundManager == nullptr)
		return false;

	return std::ranges::any_of(iter->second,
		[pSoundManager](SOUND_ID iSoundID)
		{
			return pSoundManager->IsValidSound(iSoundID);
		});
}

void CComSound::Update()
{
	auto* pSoundManager = CGameInstance::Get().GetSoundManager();
	if (pSoundManager == nullptr)
		return;

	for (auto iter = m_PlayingSounds.begin(); iter != m_PlayingSounds.end();)
	{
		const SOUND_ID iSoundID = *iter;
		if (!pSoundManager->IsValidSound(iSoundID))
		{
			RemoveSlotReference(iSoundID);
			iter = m_PlayingSounds.erase(iter);
		}
		else
		{
			++iter;
		}
	}
}

void CComSound::UpdateGUI()
{
	CComponent::UpdateGUI();
	ImGui::Text("Playing 3D Sounds: %zu", m_PlayingSounds.size());
	ImGui::Text("Named Slots: %zu", m_Slots.size());

	std::vector<std::pair<StringID, std::vector<SOUND_ID>>> vecSlots{};
	vecSlots.reserve(m_Slots.size());
	for (const auto& [sSlotID, iSoundID] : m_Slots)
		vecSlots.emplace_back(sSlotID, iSoundID);

	std::ranges::sort(vecSlots,
		[](const auto& lhs, const auto& rhs)
		{
			return lhs.first.hash < rhs.first.hash;
		});

	std::optional<StringID> sStopSlotID{};
	SOUND_ID iStopSoundID{ INVALID_SOUND_ID };
	if (ImGui::BeginTable("Sound Slots", 4,
		ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
	{
		ImGui::TableSetupColumn("Slot");
		ImGui::TableSetupColumn("Sound ID");
		ImGui::TableSetupColumn("State");
		ImGui::TableSetupColumn("Action");
		ImGui::TableHeadersRow();

		for (auto& [sSlotID, vecSoundIDs] : vecSlots)
		{
			std::ranges::sort(vecSoundIDs);
			for (size_t i = 0; i < vecSoundIDs.size(); ++i)
			{
				const SOUND_ID iSoundID = vecSoundIDs[i];
				ImGui::PushID(reinterpret_cast<void*>(static_cast<uintptr_t>(iSoundID)));
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				if (i == 0)
					ImGui::Text("%s (%zu)", sSlotID.GetDbgStr(), vecSoundIDs.size());

				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%llu", static_cast<unsigned long long>(iSoundID));

				ImGui::TableSetColumnIndex(2);
				ImGui::TextUnformatted(m_PlayingSounds.contains(iSoundID) ? "Playing" : "Invalid");

				ImGui::TableSetColumnIndex(3);
				if (ImGui::SmallButton("Stop Sound"))
					iStopSoundID = iSoundID;
				if (i == 0)
				{
					ImGui::SameLine();
					if (ImGui::SmallButton("Stop Slot"))
						sStopSlotID = sSlotID;
				}

				ImGui::PopID();
			}
		}

		ImGui::EndTable();
	}

	if (sStopSlotID)
		StopSlot(*sStopSlotID);
	else if (iStopSoundID != INVALID_SOUND_ID)
		StopInternal(iStopSoundID);

	if (ImGui::Button("Stop All Sounds"))
		StopAll();
}

SOUND_ID CComSound::PlayInternal(const _string& sPath,
	const SOUND_PLAY_DESC& tPlayDesc, SOUND_LOAD_TYPE eLoadType)
{
	auto* pSoundManager = CGameInstance::Get().GetSoundManager();
	if (pSoundManager == nullptr)
		return INVALID_SOUND_ID;

	const SOUND_ID iSoundID = pSoundManager->Play2D(sPath, tPlayDesc, eLoadType);
	if (iSoundID != INVALID_SOUND_ID)
		m_PlayingSounds.emplace(iSoundID);

	return iSoundID;
}

SOUND_ID CComSound::PlayInternal(const _string& sPath, const SOUND_3D_DESC& t3DDesc,
	const SOUND_PLAY_DESC& tPlayDesc, SOUND_LOAD_TYPE eLoadType)
{
	auto* pSoundManager = CGameInstance::Get().GetSoundManager();
	if (pSoundManager == nullptr)
		return INVALID_SOUND_ID;

	const SOUND_ID iSoundID = pSoundManager->Play3D(sPath, t3DDesc, tPlayDesc, eLoadType);
	if (iSoundID != INVALID_SOUND_ID)
		m_PlayingSounds.emplace(iSoundID);

	return iSoundID;
}

_bool CComSound::StopInternal(SOUND_ID iSoundID)
{
	if (iSoundID == INVALID_SOUND_ID)
		return false;

	_bool bStopped{};
	if (auto* pSoundManager = CGameInstance::Get().GetSoundManager())
		bStopped = pSoundManager->Stop(iSoundID);

	m_PlayingSounds.erase(iSoundID);
	RemoveSlotReference(iSoundID);
	return bStopped;
}

void CComSound::RemoveSlotReference(SOUND_ID iSoundID)
{
	for (auto iter = m_Slots.begin(); iter != m_Slots.end(); ++iter)
	{
		auto& vecSoundIDs = iter->second;
		const auto soundIter = std::ranges::find(vecSoundIDs, iSoundID);
		if (soundIter != vecSoundIDs.end())
		{
			vecSoundIDs.erase(soundIter);
			if (vecSoundIDs.empty())
				m_Slots.erase(iter);
			return;
		}
	}
}

UPtr<CComSound> CComSound::Create()
{
	auto pInstance = ToUPtr(new CComSound{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CComSound");
		return nullptr;
	}

	return pInstance;
}

UPtr<CPrototype> CComSound::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CComSound{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CComSound");
		return nullptr;
	}

	return pInstance;
}

void CComSound::Free()
{
	StopAll();
	CComponent::Free();
}
