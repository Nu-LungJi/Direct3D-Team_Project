#include "pch.h"
#include "GameInstance.h"
#include "ComAnimMontage.h"
#include "ComModelInstance.h"
#include "ResModelAnim.h"
#include "ResModel.h"


CComAnimMontage::CComAnimMontage()
{


}

CComAnimMontage::~CComAnimMontage()
{
}


HRESULT CComAnimMontage::Initialize(void* pArg)
{
	if (FAILED(CComponent::Initialize(pArg)))
	{
		return E_FAIL;
	}

	if (pArg != nullptr) {

	}


	return S_OK;
}


HRESULT CComAnimMontage::Update(_float fTimeDelta)
{



	return S_OK;
}

HRESULT CComAnimMontage::Update_Montage(_float fTimeDelta)
{
	return E_NOTIMPL;
}

HRESULT CComAnimMontage::Process_Clips()
{
	return E_NOTIMPL;
}

HRESULT CComAnimMontage::Process_Events()
{
	return E_NOTIMPL;
}

_bool CComAnimMontage::IsClipActive(const MONTAGE_CLIP& clip, float fMontageTime)
{
	return _bool();
}

float CComAnimMontage::CalcClipLocalTime(const MONTAGE_CLIP& clip, float fMontageTime)
{
	return 0.0f;
}

float CComAnimMontage::CalcClipWeight(const MONTAGE_CLIP& clip, float fMontageTime)
{
	return 0.0f;
}

void CComAnimMontage::DispatchEvent(const MONTAGE_EVENT& eventDesc)
{
}

void CComAnimMontage::PlaySoundEvent(const MONTAGE_EVENT& eventDesc)
{
}

void CComAnimMontage::SpawnParticleEvent(const MONTAGE_EVENT& eventDesc)
{
}

void CComAnimMontage::ExecuteCallbackEvent(const MONTAGE_EVENT& eventDesc)
{
}


	
HRESULT CComAnimMontage::Load_Montage(const std::string& strPath) {

	return S_OK;
}

HRESULT CComAnimMontage::Save_Montage(const std::string& strPath) {
	return S_OK;
}

void CComAnimMontage::AddClip(const MONTAGE_CLIP& clip) {

}

void CComAnimMontage::AddEvent(const MONTAGE_EVENT& eventDesc) {

}


UPtr<CComAnimMontage> CComAnimMontage::Create()
{
	auto pInstance = ToUPtr(new CComAnimMontage{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CComAnimMontage");
		return nullptr;
	}
	return pInstance;
}

UPtr<CPrototype> CComAnimMontage::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CComAnimMontage{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned: CComAnimMontage");
		return nullptr;
	}
	return pInstance;
}
