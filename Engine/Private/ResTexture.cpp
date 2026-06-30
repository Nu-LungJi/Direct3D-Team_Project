#include "pch.h"
#include "ResTexture.h"
NS_USING(Engine)

CResTexture::CResTexture(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
    : CResource{ sPath }
    , m_pDevice{ pDevice }
    , m_pContext { pContext }
{
}
CResTexture::~CResTexture()
{
}