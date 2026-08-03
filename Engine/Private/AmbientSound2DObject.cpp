#include "pch.h"
#include "AmbientSound2DObject.h"

#include "ComSound.h"
#include "GameInstance.h"

NS_USING(Engine)

HRESULT CAmbientSound2DObject::Initialize(void* pArg)
{
	auto* pDesc = static_cast<DESC*>(pArg);
	if (pDesc == nullptr || FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	CComSound::DESC tSoundDesc{};
	if (FAILED(AddComponentFromProto(
		ES_EngineProtoMajorType::PERMANENT,
		ES_EngineProtoComponent::Prototype_Component_ComSound,
		"Com_Sound",
		&tSoundDesc,
		&m_pComSound)) ||
		m_pComSound == nullptr)
	{
		return E_FAIL;
	}

	m_tSoundData = pDesc->tSoundData;
	SanitizeData(m_tSoundData);

	if (m_tSoundData.bEnabled &&
		m_tSoundData.bAutoPlay &&
		!m_tSoundData.sSoundPath.empty() &&
		Play() == INVALID_SOUND_ID)
	{
		DEBUG_LOG_STR(
			std::string{ "[Sound][Ambient2D] Auto play failed: " } +
			m_tSoundData.sSoundPath + "\n");
	}

	return S_OK;
}

void CAmbientSound2DObject::Update(_float fTimeDelta)
{
	if (m_pComSound == nullptr)
		return;

	m_pComSound->Update();
	if (m_iSoundID != INVALID_SOUND_ID &&
		!m_pComSound->IsValidSlot(GetAmbientSlotID()))
	{
		m_iSoundID = INVALID_SOUND_ID;
	}
}

void CAmbientSound2DObject::UpdateGUI()
{
	CGameObject::UpdateGUI();

	ImGui::Separator();
	ImGui::TextUnformatted("Ambient Sound 2D");
	ImGui::Text("Name: %s",
		m_tSoundData.sName.empty() ? "<unnamed>" : m_tSoundData.sName.c_str());
	ImGui::TextWrapped("Path: %s",
		m_tSoundData.sSoundPath.empty() ? "<empty>" : m_tSoundData.sSoundPath.c_str());
	ImGui::Text("State: %s", IsPlaying() ? "Playing" : "Stopped");

	_bool bEnabled = m_tSoundData.bEnabled;
	if (ImGui::Checkbox("Enabled", &bEnabled))
		SetEnabled(bEnabled);

	if (ImGui::Button("Play Ambient 2D"))
		Play();
	ImGui::SameLine();
	if (ImGui::Button("Stop Ambient 2D"))
		Stop();
	ImGui::SameLine();
	if (ImGui::Button("Fade Out Ambient 2D"))
		FadeOutAndStop();
}

SOUND_ID CAmbientSound2DObject::Play()
{
	if (m_pComSound == nullptr ||
		!m_tSoundData.bEnabled ||
		m_tSoundData.sSoundPath.empty())
	{
		return INVALID_SOUND_ID;
	}

	m_iSoundID = m_pComSound->PlaySlot2D(
		GetAmbientSlotID(),
		m_tSoundData.sSoundPath,
		SOUND_PLAY_DESC{
			.sBusID = m_tSoundData.sBusID,
			.fVolume = m_tSoundData.fVolume,
			.fPitch = m_tSoundData.fPitch,
			.fFadeInDuration = m_tSoundData.fFadeInDuration,
			.iPriority = m_tSoundData.iPriority,
			.bLoop = m_tSoundData.bLoop,
			.bStartPaused = false
		},
		SOUND_SLOT_PLAY_MODE::REPLACE,
		m_tSoundData.eLoadType);

	return m_iSoundID;
}

_bool CAmbientSound2DObject::Stop()
{
	if (m_pComSound == nullptr)
		return false;

	const _bool bStopped = m_pComSound->StopSlot(GetAmbientSlotID());
	m_iSoundID = INVALID_SOUND_ID;
	return bStopped;
}

_bool CAmbientSound2DObject::FadeOutAndStop()
{
	if (m_pComSound == nullptr)
		return false;

	return m_pComSound->FadeOutAndStopSlot(
		GetAmbientSlotID(), m_tSoundData.fFadeOutDuration);
}

_bool CAmbientSound2DObject::FadeOutAndDetach()
{
	if (m_pComSound == nullptr)
		return false;

	const _bool bScheduled = m_pComSound->FadeOutAndDetachSlot(
		GetAmbientSlotID(), m_tSoundData.fFadeOutDuration);
	if (bScheduled)
		m_iSoundID = INVALID_SOUND_ID;

	return bScheduled;
}

_bool CAmbientSound2DObject::ApplyData(
	const AMBIENT_SOUND_2D_DATA& tData)
{
	const _bool bWasPlaying = IsPlaying();
	Stop();

	m_tSoundData = tData;
	SanitizeData(m_tSoundData);

	if (!m_tSoundData.bEnabled ||
		m_tSoundData.sSoundPath.empty() ||
		(!m_tSoundData.bAutoPlay && !bWasPlaying))
	{
		return true;
	}

	return Play() != INVALID_SOUND_ID;
}

void CAmbientSound2DObject::SetEnabled(_bool bEnabled)
{
	if (m_tSoundData.bEnabled == bEnabled)
		return;

	m_tSoundData.bEnabled = bEnabled;
	if (!bEnabled)
	{
		if (m_tSoundData.fFadeOutDuration > 0.f)
			FadeOutAndStop();
		else
			Stop();
		return;
	}

	if (m_tSoundData.bAutoPlay)
		Play();
}

_bool CAmbientSound2DObject::IsPlaying() const
{
	return m_pComSound != nullptr &&
		m_iSoundID != INVALID_SOUND_ID &&
		m_pComSound->IsValidSlot(GetAmbientSlotID());
}

UPtr<CAmbientSound2DObject> CAmbientSound2DObject::Create()
{
	auto pInstance = ToUPtr(new CAmbientSound2DObject{});
	if (FAILED(pInstance->InitializePrototype()))
		return nullptr;

	return pInstance;
}

UPtr<CPrototype> CAmbientSound2DObject::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CAmbientSound2DObject{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
		return nullptr;

	return pInstance;
}

const StringID& CAmbientSound2DObject::GetAmbientSlotID()
{
	static const StringID sSlotID{ "AMBIENT_2D" };
	return sSlotID;
}

void CAmbientSound2DObject::SanitizeData(
	AMBIENT_SOUND_2D_DATA& tData)
{
	tData.fVolume = std::max(tData.fVolume, 0.f);
	tData.fPitch = std::max(tData.fPitch, 0.01f);
	tData.fFadeInDuration = std::max(tData.fFadeInDuration, 0.f);
	tData.fFadeOutDuration = std::max(tData.fFadeOutDuration, 0.f);
	tData.iPriority = std::clamp(tData.iPriority, 0, 256);
	if (tData.sBusID.hash == 0)
		tData.sBusID = SOUND_MASTER_BUS_ID;
}

void CAmbientSound2DObject::Free()
{
	if (m_pComSound != nullptr)
		m_pComSound->StopAll();

	m_iSoundID = INVALID_SOUND_ID;
	CGameObject::Free();
}
