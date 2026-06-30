#include "pch.h"
#include "ResDynamicVIBuffer.h"
#include "GameInstance.h"

NS_USING(Engine)

CResDynamicVIBuffer::CResDynamicVIBuffer(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
	: CResVIBuffer{ sPath, pDevice, pContext }
{
}

CResDynamicVIBuffer::~CResDynamicVIBuffer()
{
}

HRESULT CResDynamicVIBuffer::Load(const std::any& arg)
{
	auto desc = std::any_cast<DESC>(&arg);
	if (!desc)
	{
		return E_FAIL;
	}
	if (m_eState == STATE::LOADED)
	{
		return S_OK;
	}
	m_eState = STATE::LOADING;
	{
		m_ePrimitiveType = desc->ePrimitiveType;
		m_iNumVertices = desc->iNumVertices;
		m_iVertexStride = desc->iVertexStride;
        if (FAILED(CreateVertexBuffer(desc->vertexDesc, desc->vertexSubResource.pSysMem ? &desc->vertexSubResource : 0)))
        {
            m_eState = STATE::LOADFAIL;
            return E_FAIL;
        }

		if (desc->iNumIndices > 0)
		{
			m_iNumIndices = desc->iNumIndices;
			m_iIndexStride = desc->iIndexStride;
			m_eIndexFormat = desc->eIndexFormat;
			if (FAILED(CreateIndexBuffer(desc->IndexDesc, desc->indexSubResource.pSysMem ? &desc->indexSubResource : 0)))
			{
				m_eState = STATE::LOADFAIL;
				return E_FAIL;
			}
		}
	}
	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResDynamicVIBuffer::Unload(const std::any& arg)
{
	return S_OK;
}

SPtr<CResDynamicVIBuffer> CResDynamicVIBuffer::Create()
{
	return ToSPtr(new CResDynamicVIBuffer{"", CGameInstance::Get().GetGraphicDevice() , CGameInstance::Get().GetGraphicDeviceContext() });
}
