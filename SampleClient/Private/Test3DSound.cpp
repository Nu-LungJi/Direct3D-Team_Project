#include "pch.h"
#include "Test3DSound.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "Resources.h"
#include "GameInstance.h"

NS_USING(Client)

CTest3DSound::CTest3DSound()
	: CGameObject{}
{
}

CTest3DSound::~CTest3DSound()
{
}


void CTest3DSound::LateUpdate(E::_float fTimeDelta)
{
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
