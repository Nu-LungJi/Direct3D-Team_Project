#include "pch.h"
#include "AmbientSound3DObject.h"

#include "ComSound.h"
#include "GameInstance.h"

NS_USING(Engine)

HRESULT CAmbientSound3DObject::Initialize(void* pArg)
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
	GetTransform().SetPosition(m_tSoundData.vPosition);
	m_vLastSoundPosition = m_tSoundData.vPosition;

	if (m_tSoundData.bEnabled &&
		m_tSoundData.bAutoPlay &&
		!m_tSoundData.sSoundPath.empty() &&
		Play() == INVALID_SOUND_ID)
	{
		DEBUG_LOG_STR(
			std::string{ "[Sound][Ambient] Auto play failed: " } +
			m_tSoundData.sSoundPath + "\n");
	}

	return S_OK;
}

void CAmbientSound3DObject::Update(_float fTimeDelta)
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

void CAmbientSound3DObject::LateUpdate(_float fTimeDelta)
{
	GetTransform().Update();
	SyncSoundPosition();
}

void CAmbientSound3DObject::UpdateGUI()
{
	CGameObject::UpdateGUI();

	ImGui::Separator();
	ImGui::TextUnformatted("Ambient Sound 3D");
	ImGui::Text("Name: %s",
		m_tSoundData.sName.empty() ? "<unnamed>" : m_tSoundData.sName.c_str());
	ImGui::TextWrapped("Path: %s",
		m_tSoundData.sSoundPath.empty() ? "<empty>" : m_tSoundData.sSoundPath.c_str());
	ImGui::Text("Range: %.2f - %.2f",
		m_tSoundData.fMinDistance, m_tSoundData.fMaxDistance);
	ImGui::Text("State: %s", IsPlaying() ? "Playing" : "Stopped");

	_bool bEnabled = m_tSoundData.bEnabled;
	if (ImGui::Checkbox("Enabled", &bEnabled))
		SetEnabled(bEnabled);

	if (ImGui::Button("Play Ambient 3D"))
		Play();
	ImGui::SameLine();
	if (ImGui::Button("Stop Ambient 3D"))
		Stop();
}

SOUND_ID CAmbientSound3DObject::Play()
{
	if (m_pComSound == nullptr ||
		!m_tSoundData.bEnabled ||
		m_tSoundData.sSoundPath.empty())
	{
		return INVALID_SOUND_ID;
	}

	m_tSoundData.vPosition = GetTransform().GetPosition();
	m_vLastSoundPosition = m_tSoundData.vPosition;
	m_iSoundID = m_pComSound->PlaySlot3D(
		GetAmbientSlotID(),
		m_tSoundData.sSoundPath,
		SOUND_3D_DESC{
			.vPosition = m_tSoundData.vPosition,
			.vVelocity = {},
			.fMinDistance = m_tSoundData.fMinDistance,
			.fMaxDistance = m_tSoundData.fMaxDistance,
			.eRolloff = m_tSoundData.eRolloff
		},
		SOUND_PLAY_DESC{
			.sBusID = m_tSoundData.sBusID,
			.fVolume = m_tSoundData.fVolume,
			.fPitch = m_tSoundData.fPitch,
			.iPriority = m_tSoundData.iPriority,
			.bLoop = m_tSoundData.bLoop,
			.bStartPaused = false
		},
		SOUND_SLOT_PLAY_MODE::REPLACE,
		m_tSoundData.eLoadType);

	return m_iSoundID;
}

_bool CAmbientSound3DObject::Stop()
{
	if (m_pComSound == nullptr)
		return false;

	const _bool bStopped = m_pComSound->StopSlot(GetAmbientSlotID());
	m_iSoundID = INVALID_SOUND_ID;
	return bStopped;
}

_bool CAmbientSound3DObject::ApplyData(const AMBIENT_SOUND_3D_DATA& tData)
{
	const _bool bWasPlaying = IsPlaying();
	Stop();

	m_tSoundData = tData;
	SanitizeData(m_tSoundData);
	GetTransform().SetPosition(m_tSoundData.vPosition);
	m_vLastSoundPosition = m_tSoundData.vPosition;

	if (!m_tSoundData.bEnabled ||
		m_tSoundData.sSoundPath.empty() ||
		(!m_tSoundData.bAutoPlay && !bWasPlaying))
	{
		return true;
	}

	return Play() != INVALID_SOUND_ID;
}

void CAmbientSound3DObject::SetPosition(const _float3& vPosition)
{
	GetTransform().SetPosition(vPosition);
	m_tSoundData.vPosition = vPosition;
	SyncSoundPosition();
}

void CAmbientSound3DObject::SetEnabled(_bool bEnabled)
{
	if (m_tSoundData.bEnabled == bEnabled)
		return;

	m_tSoundData.bEnabled = bEnabled;
	if (!bEnabled)
	{
		Stop();
		return;
	}

	if (m_tSoundData.bAutoPlay)
		Play();
}

_bool CAmbientSound3DObject::IsPlaying() const
{
	return m_pComSound != nullptr &&
		m_iSoundID != INVALID_SOUND_ID &&
		m_pComSound->IsValidSlot(GetAmbientSlotID());
}

UPtr<CAmbientSound3DObject> CAmbientSound3DObject::Create()
{
	auto pInstance = ToUPtr(new CAmbientSound3DObject{});
	if (FAILED(pInstance->InitializePrototype()))
		return nullptr;

	return pInstance;
}

UPtr<CPrototype> CAmbientSound3DObject::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CAmbientSound3DObject{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
		return nullptr;

	return pInstance;
}

const StringID& CAmbientSound3DObject::GetAmbientSlotID()
{
	static const StringID sSlotID{ "AMBIENT" };
	return sSlotID;
}

void CAmbientSound3DObject::SanitizeData(AMBIENT_SOUND_3D_DATA& tData)
{
	tData.fMinDistance = std::max(tData.fMinDistance, 0.f);
	tData.fMaxDistance = std::max(
		tData.fMaxDistance,
		tData.fMinDistance + 0.01f);
	tData.fVolume = std::max(tData.fVolume, 0.f);
	tData.fPitch = std::max(tData.fPitch, 0.01f);
	tData.iPriority = std::clamp(tData.iPriority, 0, 256);
	if (tData.sBusID.hash == 0)
		tData.sBusID = SOUND_MASTER_BUS_ID;
}

void CAmbientSound3DObject::SyncSoundPosition()
{
	const _float3& vPosition = GetTransform().GetPosition();
	m_tSoundData.vPosition = vPosition;

	if (vPosition.x == m_vLastSoundPosition.x &&
		vPosition.y == m_vLastSoundPosition.y &&
		vPosition.z == m_vLastSoundPosition.z)
	{
		return;
	}

	m_vLastSoundPosition = vPosition;
	if (IsPlaying())
	{
		m_pComSound->SetSlot3DAttributes(
			GetAmbientSlotID(),
			vPosition);
	}
}

void CAmbientSound3DObject::Free()
{
	if (m_pComSound != nullptr)
		m_pComSound->StopAll();

	m_iSoundID = INVALID_SOUND_ID;
	CGameObject::Free();
}
