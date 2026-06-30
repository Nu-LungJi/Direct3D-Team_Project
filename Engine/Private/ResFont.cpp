#include "pch.h"

#include "ResFont.h"

NS_USING(Engine)

CResFont::CResFont(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
	: CResource{sPath}
	, m_pDevice {pDevice }
	, m_pContext { pContext }
{
}

CResFont::~CResFont()
{
}
