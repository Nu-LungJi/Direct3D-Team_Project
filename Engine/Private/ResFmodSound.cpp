#include "pch.h"
#include "ResFmodSound.h"
#include "fmod.h"
#include "GameInstance.h"

NS_USING(Engine)

CResFmodSound::CResFmodSound(const _string& sPath, SOUND_LOAD_TYPE eLoadType)
    : CResource{ sPath }
    , m_eLoadType{ eLoadType }
{

}

CResFmodSound::~CResFmodSound()
{
}

HRESULT CResFmodSound::Load(const std::any& arg)
{
    if (m_eState == STATE::LOADED)
    {
        return S_OK;
    }

    m_eState = STATE::LOADING;

    if (FAILED(CGameInstance::Get().CreateSound(m_sPath, &m_pFmodSound, m_eLoadType)))
    {
        MSG_BOX_STR(_wstring{ L"CResFmodSound Faield Path:" + StringToWString(m_sPath) }.c_str());
        m_eState = STATE::LOADFAIL;
        //std::wstring msg = L"CResFmodSound Load Failed, Path: " + m_sPath;
        //MSG_BOX_STR(msg.c_str());
        return E_FAIL;
    }

    m_eState = STATE::LOADED;
    return S_OK;
}

HRESULT CResFmodSound::Unload(const std::any& arg)
{
    if (m_pFmodSound != nullptr)
    {
        FMOD_Sound_Release(m_pFmodSound);
        m_pFmodSound = nullptr;
    }

    m_eState = STATE::UNLOAD;
    return S_OK;
}

SPtr<CResFmodSound> CResFmodSound::Create(const _string& sPath, SOUND_LOAD_TYPE eLoadType)
{
    return ToSPtr(new CResFmodSound{ sPath, eLoadType });
}

void CResFmodSound::Free()
{
    if (m_pFmodSound != nullptr)
    {
        FMOD_Sound_Release(m_pFmodSound);
        m_pFmodSound = nullptr;
    }

    CResource::Free();
}
