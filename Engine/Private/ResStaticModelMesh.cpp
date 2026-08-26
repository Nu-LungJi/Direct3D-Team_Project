#include "pch.h"
#include "ResStaticModelMesh.h"

#include <fstream>

NS_USING(Engine)

CResStaticModelMesh::CResStaticModelMesh(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
    : CResVIBuffer{ sPath, pDevice,pContext }
{
}

CResStaticModelMesh::~CResStaticModelMesh()
{
}

HRESULT CResStaticModelMesh::Load(const std::any& arg)
{

    auto descArg = std::any_cast<DESC>(&arg);
    if (!descArg)
    {
        return E_FAIL;
    }

    if (m_eState == STATE::LOADED)
    {
        return S_OK;
    }
    m_eState = STATE::LOADING;
    auto ptr = descArg->ptr;
    auto eType = descArg->eType;
    auto& pModel = descArg->pModel;
    auto& PreTransformMatrix = descArg->PreTransformMatrix;

    {
        if (FAILED(Ready_NonAnimMesh(ptr, PreTransformMatrix)))
            return E_FAIL;
    }


    m_eState = STATE::LOADED;
    return S_OK;
}

HRESULT CResStaticModelMesh::Unload(const std::any& arg)
{

    m_eState = STATE::UNLOAD;
    return S_OK;
}





HRESULT CResStaticModelMesh::LoadAssimp(std::string name,uint32_t materialIndex,const XMFLOAT3& minPos,const XMFLOAT3& maxPos,std::vector<VTXMESH>&& vertices,std::vector<uint32_t>&& indices,_fmatrix PreTransformMatrix)
{
	if (m_eState == STATE::LOADED)
		return S_OK;

	if (vertices.empty() || indices.empty())
	{
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}

	m_eState = STATE::LOADING;

	// 필요하면 멤버에 저장
	 //m_sName = std::move(name);
	 //m_vMin = minPos;
	 //m_vMax = maxPos;

	m_iMaterialIndex = materialIndex;

	//m_iNumVertexBuffers = 1;
	m_iNumVertices = static_cast<UINT>(vertices.size());

	m_iNumIndices = static_cast<UINT>(indices.size());
	m_iIndexStride = sizeof(uint32_t);
	m_eIndexFormat = DXGI_FORMAT_R32_UINT;
	m_ePrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	m_iVertexStride = sizeof(VTXMESH);

	m_vMinPos = { FLT_MAX, FLT_MAX, FLT_MAX };
	m_vMaxPos = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

	// ------------------------------------------------------------
	// PreTransform 적용
	// ------------------------------------------------------------
	for (UINT i = 0; i < m_iNumVertices; ++i)
	{
		VTXMESH& v = vertices[i];

		XMStoreFloat3(
			&v.vPosition,
			XMVector3TransformCoord(
				XMLoadFloat3(&v.vPosition),
				PreTransformMatrix
			)
		);

		m_vMinPos.x = std::min(m_vMinPos.x, v.vPosition.x);
		m_vMinPos.y = std::min(m_vMinPos.y, v.vPosition.y);
		m_vMinPos.z = std::min(m_vMinPos.z, v.vPosition.z);

		m_vMaxPos.x = std::max(m_vMaxPos.x, v.vPosition.x);
		m_vMaxPos.y = std::max(m_vMaxPos.y, v.vPosition.y);
		m_vMaxPos.z = std::max(m_vMaxPos.z, v.vPosition.z);

		XMStoreFloat3(
			&v.vNormal,
			XMVector3Normalize(
				XMVector3TransformNormal(
					XMLoadFloat3(&v.vNormal),
					PreTransformMatrix
				)
			)
		);

		XMStoreFloat3(
			&v.vTangent,
			XMVector3Normalize(
				XMVector3TransformNormal(
					XMLoadFloat3(&v.vTangent),
					PreTransformMatrix
				)
			)
		);

		XMStoreFloat3(
			&v.vBinormal,
			XMVector3Normalize(
				XMVector3TransformNormal(
					XMLoadFloat3(&v.vBinormal),
					PreTransformMatrix
				)
			)
		);
	}

	// ------------------------------------------------------------
	// Vertex Buffer 생성
	// ------------------------------------------------------------
	D3D11_BUFFER_DESC VertexBufferDesc{};
	VertexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(VTXMESH) * vertices.size());
	VertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	VertexBufferDesc.CPUAccessFlags = 0;
	VertexBufferDesc.MiscFlags = 0;
	VertexBufferDesc.StructureByteStride = m_iVertexStride;

	D3D11_SUBRESOURCE_DATA VertexInitialData{};
	VertexInitialData.pSysMem = vertices.data();

	if (FAILED(CreateVertexBuffer(VertexBufferDesc, &VertexInitialData)))
	{
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}

	// ------------------------------------------------------------
	// Index Buffer 생성
	// ------------------------------------------------------------
	D3D11_BUFFER_DESC IndexBufferDesc{};
	IndexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * indices.size());
	IndexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	IndexBufferDesc.CPUAccessFlags = 0;
	IndexBufferDesc.MiscFlags = 0;
	IndexBufferDesc.StructureByteStride = m_iIndexStride;

	D3D11_SUBRESOURCE_DATA IndexInitialData{};
	IndexInitialData.pSysMem = indices.data();

	if (FAILED(CreateIndexBuffer(IndexBufferDesc, &IndexInitialData)))
	{
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}

	m_eState = STATE::LOADED;

	return S_OK;
}


HRESULT CResStaticModelMesh::Ready_NonAnimMesh(_char* pPoint, _fmatrix PreTransformMatrix)
{
    std::vector<VTXMESH> vertexes{};
    std::vector<uint32_t> indices{};

    uint32_t nameLen = *(uint32_t*)pPoint;
    pPoint += sizeof(uint32_t);



    std::string name;
    name.resize(nameLen);

    memcpy(name.data(), pPoint, nameLen);
    pPoint += nameLen;


    uint32_t materialIndex = *(uint32_t*)pPoint;
    pPoint += sizeof(uint32_t);


	XMFLOAT3 storedMinPos{};
	memcpy(&storedMinPos, pPoint, sizeof(XMFLOAT3));
	pPoint += sizeof(XMFLOAT3);

	XMFLOAT3 storedMaxPos{};
	memcpy(&storedMaxPos, pPoint, sizeof(XMFLOAT3));
	pPoint += sizeof(XMFLOAT3);

	uint32_t vCount = *(uint32_t*)pPoint;
	pPoint += sizeof(uint32_t);

	uint32_t iCount = *(uint32_t*)pPoint;
	pPoint += sizeof(uint32_t);

    vertexes.resize(vCount);
    memcpy(vertexes.data(), pPoint, sizeof(VTXMESH) * vCount);
    pPoint += sizeof(VTXMESH) * vCount;


    indices.resize(iCount);
    memcpy(indices.data(), pPoint, sizeof(uint32_t) * iCount);
    pPoint += sizeof(uint32_t) * iCount;

    m_iMaterialIndex = materialIndex;
    //m_iNumVertexBuffers = 1;
    m_iNumVertices = vCount;

    m_iNumIndices = static_cast<UINT>(indices.size());
    m_iIndexStride = sizeof(uint32_t);
    m_eIndexFormat = DXGI_FORMAT_R32_UINT;
    m_ePrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	if (XMMatrixIsIdentity(PreTransformMatrix))
	{
		// 맵 정적 모델은 항등 PreTransform을 사용한다. 바이너리에 저장된 Bounds를
		// 그대로 사용하면 모든 정점을 다시 변환하고 검사하는 비용을 피할 수 있다.
		m_vMinPos = storedMinPos;
		m_vMaxPos = storedMaxPos;
	}
	else
	{
		m_vMinPos = { FLT_MAX, FLT_MAX, FLT_MAX };
		m_vMaxPos = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

		for (VTXMESH& vertex : vertexes)
		{
			XMStoreFloat3(&vertex.vPosition,
				XMVector3TransformCoord(XMLoadFloat3(&vertex.vPosition), PreTransformMatrix));

			m_vMinPos.x = std::min(m_vMinPos.x, vertex.vPosition.x);
			m_vMinPos.y = std::min(m_vMinPos.y, vertex.vPosition.y);
			m_vMinPos.z = std::min(m_vMinPos.z, vertex.vPosition.z);
			m_vMaxPos.x = std::max(m_vMaxPos.x, vertex.vPosition.x);
			m_vMaxPos.y = std::max(m_vMaxPos.y, vertex.vPosition.y);
			m_vMaxPos.z = std::max(m_vMaxPos.z, vertex.vPosition.z);

			XMStoreFloat3(&vertex.vNormal,
				XMVector3TransformNormal(XMLoadFloat3(&vertex.vNormal), PreTransformMatrix));
		}
	}

    //-------------------------------------------------------------------
    m_iVertexStride = sizeof(VTXMESH);
    D3D11_BUFFER_DESC           VertexBufferDesc{};
    VertexBufferDesc.ByteWidth = m_iNumVertices * sizeof(VTXMESH);
    VertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    VertexBufferDesc.StructureByteStride = m_iVertexStride;
    VertexBufferDesc.CPUAccessFlags = 0;
    VertexBufferDesc.MiscFlags = 0;



    D3D11_SUBRESOURCE_DATA          VertexInitialData{};
    VertexInitialData.pSysMem = vertexes.data();

    if (FAILED(CreateVertexBuffer(VertexBufferDesc, &VertexInitialData)))
    {
        m_eState = STATE::LOADFAIL;
        return E_FAIL;
    }

    //-----------------------------------------------------------------------------------


    D3D11_BUFFER_DESC           IndexBufferDesc{};
    IndexBufferDesc.ByteWidth = m_iNumIndices * m_iIndexStride;
    IndexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    IndexBufferDesc.StructureByteStride = m_iIndexStride;
    IndexBufferDesc.CPUAccessFlags = 0;
    IndexBufferDesc.MiscFlags = 0;





    D3D11_SUBRESOURCE_DATA          IndexInitialData{};
    IndexInitialData.pSysMem = indices.data();


    if (FAILED(CreateIndexBuffer(IndexBufferDesc, &IndexInitialData)))
    {
        m_eState = STATE::LOADFAIL;
        return E_FAIL;
    }


    return S_OK;



}

SPtr<CResStaticModelMesh> CResStaticModelMesh::Create()
{
    return ToSPtr(new CResStaticModelMesh{ "",CGameInstance::Get().GetGraphicDevice(),CGameInstance::Get().GetGraphicDeviceContext() });
}
