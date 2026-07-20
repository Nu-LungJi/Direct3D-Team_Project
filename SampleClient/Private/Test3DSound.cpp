#include "pch.h"
#include "Test3DSound.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "Resources.h"
#include "GameInstance.h"

NS_USING(Client)

namespace
{
	constexpr E::_float SOUND_MIN_DISTANCE = 1.f;
	constexpr E::_float SOUND_MAX_DISTANCE = 10.f;
}

CTest3DSound::CTest3DSound()
	: CGameObject{}
{
}

CTest3DSound::~CTest3DSound()
{
}


HRESULT CTest3DSound::Initialize(void* pArg)
{
	if (FAILED(CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	}

	{
		const _string sSoundPath = "./Resources/SampleClient/Sound/Verses_1_4_of_the_National_Anthem.mp3";
		auto* pSoundManager = CGameInstance::Get().GetSoundManager();
		if (pSoundManager == nullptr || !pSoundManager->Preload(sSoundPath))
			return E_FAIL;

		const E::_float3 vSoundPosition = GetTransform().GetPosition();
		m_soundID = pSoundManager->Play3D(
			sSoundPath,
			E::SOUND_3D_DESC{
				.vPosition = vSoundPosition,
				.vVelocity = {},
				.fMinDistance = SOUND_MIN_DISTANCE,
				.fMaxDistance = SOUND_MAX_DISTANCE,
				.eRolloff = E::SOUND_3D_ROLLOFF::LINEAR
			},
			E::SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::VOICE,
				.fVolume = 1.f,
				.fPitch = 1.f,
				.iPriority = 64,
				.bLoop = true
			});
		if (m_soundID == INVALID_SOUND_ID)
			return E_FAIL;

	}

	return S_OK;
}

void CTest3DSound::LateUpdate(E::_float fTimeDelta)
{
	GetTransform().Update();

	auto& gameInstance = CGameInstance::Get();
	auto* pSoundManager = gameInstance.GetSoundManager();
	if (pSoundManager != nullptr)
	{
		const E::_float3 vSoundPosition = GetTransform().GetPosition();

		if (pSoundManager->IsValidSound(m_soundID))
			pSoundManager->Set3DAttributes(m_soundID, vSoundPosition);
	}

	auto dmode = gameInstance.GetDbgLineRender()->GetDepthMode();
	gameInstance.GetDbgLineRender()->SetDepthTest(true);
	gameInstance.GetDbgLineRender()->AddSphere(
		SOUND_MAX_DISTANCE,
		GetTransform().GetLoadedCombinedWorldMatrix());
	gameInstance.GetDbgLineRender()->SetDepthMode(dmode);
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

void CTest3DSound::Free()
{
	if (auto* pSoundManager = CGameInstance::Get().GetSoundManager();
		pSoundManager != nullptr && m_soundID != INVALID_SOUND_ID)
	{
		pSoundManager->Stop(m_soundID);
		m_soundID = INVALID_SOUND_ID;
	}

	CGameObject::Free();
}
