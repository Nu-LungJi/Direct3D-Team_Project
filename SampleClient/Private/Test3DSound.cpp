#include "pch.h"
#include "Test3DSound.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "Resources.h"
#include "GameInstance.h"
#include "ComSound.h"

NS_USING(Client)

namespace
{
	constexpr E::_float SOUND_MIN_DISTANCE = 1.f;
	constexpr E::_float SOUND_MAX_DISTANCE = 10.f;
	const E::StringID SOUND_SLOT{ "LOOP" };
	const E::StringID TEST_SLOT{ "TEST" };
}

CTest3DSound::CTest3DSound()
	: CGameObject{}
{
}

CTest3DSound::~CTest3DSound()
{
}


void CTest3DSound::UpdateGUI()
{
	CGameObject::UpdateGUI();

	if (ImGui::Button("Play3d"))
	{
		auto id = m_pComSound->PlaySlot3D(
			TEST_SLOT,
			"./Resources/SampleClient/Sound/avada.wav",
			SOUND_3D_DESC{
				.vPosition = GetTransform().GetPosition(),
				.fMinDistance = SOUND_MIN_DISTANCE,
				.fMaxDistance = 30.f,
				.eRolloff = SOUND_3D_ROLLOFF::LINEAR
			},
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::VOICE,
				.fVolume = 1.f,
				.fPitch = 1.f,
				.iPriority = 64,
				.bLoop = false
			});

		if (id == INVALID_SOUND_ID)
		{
			MSG_BOX("INVALID_SOUND_ID");
		}
	}

	if (ImGui::Button("Play3d Overlap"))
	{
		auto id = m_pComSound->PlaySlot3D(
			TEST_SLOT,
			"./Resources/SampleClient/Sound/avada.wav",
			SOUND_3D_DESC{
				.vPosition = GetTransform().GetPosition(),
				.fMinDistance = SOUND_MIN_DISTANCE,
				.fMaxDistance = 30.f,
				.eRolloff = SOUND_3D_ROLLOFF::LINEAR
			},
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::VOICE,
				.fVolume = 1.f,
				.fPitch = 1.f,
				.iPriority = 64,
				.bLoop = false
			}, SOUND_SLOT_PLAY_MODE::OVERLAP);

		if (id == INVALID_SOUND_ID)
		{
			MSG_BOX("INVALID_SOUND_ID");
		}
	}

	if (ImGui::Button("Play2d"))
	{
		auto id = m_pComSound->PlaySlot2D(
			TEST_SLOT,
			"./Resources/SampleClient/Sound/avada.wav",
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::VOICE,
				.fVolume = 1.f,
				.fPitch = 1.f,
				.iPriority = 64,
				.bLoop = false
			});

		if (id == INVALID_SOUND_ID)
		{
			MSG_BOX("INVALID_SOUND_ID");
		}
	}

	if (ImGui::Button("Play2d Overlap"))
	{
		auto id = m_pComSound->PlaySlot2D(
			TEST_SLOT,
			"./Resources/SampleClient/Sound/avada.wav",
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::VOICE,
				.fVolume = 1.f,
				.fPitch = 1.f,
				.iPriority = 64,
				.bLoop = false
			}, SOUND_SLOT_PLAY_MODE::OVERLAP);

		if (id == INVALID_SOUND_ID)
		{
			MSG_BOX("INVALID_SOUND_ID");
		}
	}
}

HRESULT CTest3DSound::Initialize(void* pArg)
{
	auto* pDesc = static_cast<CTest3DSound::DESC*>(pArg);
	if (pDesc->loopSoundPath.empty())
	{
		return E_FAIL;
	}
	if (FAILED(CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	}
	

	CComSound::DESC tSoundDesc{};
	if (FAILED(AddComponentFromProto(
		ES_EngineProtoMajorType::PERMANENT,
		ES_EngineProtoComponent::Prototype_Component_ComSound,
		"Com_Sound",
		&tSoundDesc,
		&m_pComSound)))
	{
		return E_FAIL;
	}

	if (m_pComSound->PlaySlot3D(
		SOUND_SLOT,
		pDesc->loopSoundPath,
		SOUND_3D_DESC{
			.vPosition = GetTransform().GetPosition(),
			.fMinDistance = SOUND_MIN_DISTANCE,
			.fMaxDistance = SOUND_MAX_DISTANCE,
			.eRolloff = SOUND_3D_ROLLOFF::LINEAR
		},
		SOUND_PLAY_DESC{
			.sBusID = SOUND_BUS::VOICE,
			.fVolume = 1.f,
			.fPitch = 1.f,
			.iPriority = 64,
			.bLoop = true
		}) == INVALID_SOUND_ID)
	{
		return E_FAIL;
	}

	return S_OK;
}

void CTest3DSound::LateUpdate(E::_float fTimeDelta)
{
	GetTransform().Update();

	m_pComSound->SetSlot3DAttributes(SOUND_SLOT, GetTransform().GetPosition());
	m_pComSound->Update();
}

E::UPtr<CTest3DSound> CTest3DSound::Create()
{
	auto pInstance = E::ToUPtr(new CTest3DSound{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CTest3DSound");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CTest3DSound::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CTest3DSound{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTest3DSound");
		return nullptr;
	}

	return pInstance;
}
