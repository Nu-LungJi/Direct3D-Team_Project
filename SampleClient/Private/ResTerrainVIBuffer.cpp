#include "pch.h"
#include "ResTerrainVIBuffer.h"
#include "GameInstance.h"
//../Bin/Resources/Textures/Terrain/Height.bmp

NS_USING(Client)

CResTerrainVIBuffer::CResTerrainVIBuffer(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
	: CResVIBuffer{ sPath, pDevice, pContext }
{
}

CResTerrainVIBuffer::~CResTerrainVIBuffer()
{
}

HRESULT CResTerrainVIBuffer::Load(const std::any& arg)
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
		{
			DWORD       dwByte = {};
			HANDLE      hFile = CreateFile(StringToWString(m_sPath).c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
			if (0 == hFile)
				return E_FAIL;

			BITMAPFILEHEADER            fh{};
			BITMAPINFOHEADER            ih{};

			ReadFile(hFile, &fh, sizeof fh, &dwByte, nullptr);
			ReadFile(hFile, &ih, sizeof ih, &dwByte, nullptr);

			std::unique_ptr<uint32_t[]>      pPixels = std::unique_ptr<uint32_t[]>(new uint32_t[ih.biWidth * ih.biHeight]);
			ReadFile(hFile, pPixels.get(), sizeof(uint32_t) * ih.biWidth * ih.biHeight, &dwByte, nullptr);

			m_iNumVerticesX = ih.biWidth;
			m_iNumVerticesZ = ih.biHeight;

			m_iVertexStride = sizeof(VTX_NORMAL_TEX);
			m_iNumVertices = m_iNumVerticesX * m_iNumVerticesZ;
			//VTX_NORMAL_TEX

			std::unique_ptr<VTX_NORMAL_TEX[]> pVertices = std::unique_ptr<VTX_NORMAL_TEX[]>(new VTX_NORMAL_TEX[m_iNumVertices]);
			ZeroMemory(pVertices.get(), sizeof(VTX_NORMAL_TEX) * m_iNumVertices);

			for (uint32_t i = 0; i < m_iNumVerticesZ; i++)
			{
				for (uint32_t j = 0; j < m_iNumVerticesX; j++)
				{
					uint32_t        iIndex = i * m_iNumVerticesX + j;


					pVertices[iIndex].pos = _float3(j, (pPixels[iIndex] & 0x000000ff) / 10.f, i);
					pVertices[iIndex].normal = _float3(0.f, 0.f, 0.f);
					pVertices[iIndex].texCoord = _float2(j / (m_iNumVerticesX - 1.f), i / (m_iNumVerticesZ - 1.f));
				}
			}








			m_iNumIndices = (m_iNumVerticesX - 1) * (m_iNumVerticesZ - 1) * 2 * 3;
			m_iIndexStride = sizeof(uint32_t);
			m_eIndexFormat = DXGI_FORMAT_R32_UINT;
			m_ePrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

			std::unique_ptr<uint32_t[]> pIndices = std::unique_ptr<uint32_t[]>(new uint32_t[m_iNumIndices]);
			ZeroMemory(pIndices.get(), sizeof(uint32_t) * m_iNumIndices);

			uint32_t        iNumIndices = {};

			for (uint32_t i = 0; i < m_iNumVerticesZ - 1; i++)
			{
				for (uint32_t j = 0; j < m_iNumVerticesX - 1; j++)
				{
					uint32_t        iIndex = i * m_iNumVerticesX + j;

					uint32_t        iIndices[4] = {
						iIndex + m_iNumVerticesX,
						iIndex + m_iNumVerticesX + 1,
						iIndex + 1,
						iIndex
					};

					_vector     vSour, vDest, vNormal;

					pIndices[iNumIndices++] = iIndices[0];
					pIndices[iNumIndices++] = iIndices[1];
					pIndices[iNumIndices++] = iIndices[2];

					vSour = XMLoadFloat3(&pVertices[iIndices[1]].pos) - XMLoadFloat3(&pVertices[iIndices[0]].pos);
					vDest = XMLoadFloat3(&pVertices[iIndices[2]].pos) - XMLoadFloat3(&pVertices[iIndices[1]].pos);
					vNormal = XMVector3Normalize(XMVector3Cross(vSour, vDest));

					XMStoreFloat3(&pVertices[iIndices[0]].normal, XMLoadFloat3(&pVertices[iIndices[0]].normal) + vNormal);
					XMStoreFloat3(&pVertices[iIndices[1]].normal, XMLoadFloat3(&pVertices[iIndices[1]].normal) + vNormal);
					XMStoreFloat3(&pVertices[iIndices[2]].normal, XMLoadFloat3(&pVertices[iIndices[2]].normal) + vNormal);


					pIndices[iNumIndices++] = iIndices[0];
					pIndices[iNumIndices++] = iIndices[2];
					pIndices[iNumIndices++] = iIndices[3];


					vSour = XMLoadFloat3(&pVertices[iIndices[2]].pos) - XMLoadFloat3(&pVertices[iIndices[0]].pos);
					vDest = XMLoadFloat3(&pVertices[iIndices[3]].pos) - XMLoadFloat3(&pVertices[iIndices[2]].pos);
					vNormal = XMVector3Normalize(XMVector3Cross(vSour, vDest));

					XMStoreFloat3(&pVertices[iIndices[0]].normal, XMLoadFloat3(&pVertices[iIndices[0]].normal) + vNormal);
					XMStoreFloat3(&pVertices[iIndices[2]].normal, XMLoadFloat3(&pVertices[iIndices[2]].normal) + vNormal);
					XMStoreFloat3(&pVertices[iIndices[3]].normal, XMLoadFloat3(&pVertices[iIndices[3]].normal) + vNormal);
				}
			}
			for (size_t i = 0; i < m_iNumVertices; i++)
				XMStoreFloat3(&pVertices[i].normal, XMVector3Normalize(XMLoadFloat3(&pVertices[i].normal)));



			D3D11_BUFFER_DESC vertexBufferDesc{};
			vertexBufferDesc.ByteWidth = m_iVertexStride * m_iNumVertices;
			vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
			vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			vertexBufferDesc.StructureByteStride = m_iVertexStride;
			vertexBufferDesc.CPUAccessFlags = 0;
			vertexBufferDesc.MiscFlags = 0;

			D3D11_SUBRESOURCE_DATA vertexInitialData{};
			vertexInitialData.pSysMem = pVertices.get();
			if (FAILED(CreateVertexBuffer(vertexBufferDesc, &vertexInitialData)))
			{
				m_eState = STATE::LOADFAIL;
				return E_FAIL;
			}



			D3D11_BUFFER_DESC           IndexBufferDesc{};
			IndexBufferDesc.ByteWidth = m_iNumIndices * m_iIndexStride;
			IndexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
			IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			IndexBufferDesc.StructureByteStride = m_iIndexStride;
			IndexBufferDesc.CPUAccessFlags = 0;
			IndexBufferDesc.MiscFlags = 0;

			D3D11_SUBRESOURCE_DATA indexInitialData{};
			indexInitialData.pSysMem = pIndices.get();

			if (FAILED(CreateIndexBuffer(IndexBufferDesc, &indexInitialData)))
			{
				m_eState = STATE::LOADFAIL;
				return E_FAIL;
			}

			CloseHandle(hFile);
		}



	}
	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResTerrainVIBuffer::Unload(const std::any& arg)
{
	return S_OK;
}

SPtr<CResTerrainVIBuffer> CResTerrainVIBuffer::Create(const _string& sPath)
{
	return ToSPtr(new CResTerrainVIBuffer{ sPath, CGameInstance::Get().GetGraphicDevice() , CGameInstance::Get().GetGraphicDeviceContext() });
}



