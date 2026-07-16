#include "pch.h"
#include "Level.h"

NS_USING(Engine)

CLevel::CLevel(uint32_t iLevelID)
	: m_iLevelID{ iLevelID }
{
}

CLevel::~CLevel()
{

}

HRESULT CLevel::Initialize()
{
    return S_OK;
}

void CLevel::Update(_float fTimeDelta)
{
}

HRESULT CLevel::Render()
{
    return S_OK;
}

void CLevel::FrameStart(_float fTimeDelta)
{
}

void CLevel::FrameEnd(_float fTimeDelta)
{
}

void CLevel::UpdateGUI()
{
}

