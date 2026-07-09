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
    auto vertexes = std::make_shared<std::vector<VTXMESH>>();
    auto indices = std::make_shared<std::vector<uint32_t>>();

    uint32_t nameLen = *(uint32_t*)pPoint;
    pPoint += sizeof(uint32_t);



    std::string name;
    name.resize(nameLen);

    memcpy(name.data(), pPoint, nameLen);
    pPoint += nameLen;


    uint32_t materialIndex = *(uint32_t*)pPoint;
    pPoint += sizeof(uint32_t);


	memcpy(&m_vMinPos, pPoint, sizeof(XMFLOAT3));
	pPoint += sizeof(XMFLOAT3);

	memcpy(&m_vMaxPos, pPoint, sizeof(XMFLOAT3));
	pPoint += sizeof(XMFLOAT3);

	uint32_t vCount = *(uint32_t*)pPoint;
	pPoint += sizeof(uint32_t);

	uint32_t iCount = *(uint32_t*)pPoint;
	pPoint += sizeof(uint32_t);

    vertexes->resize(vCount);
    memcpy(vertexes->data(), pPoint, sizeof(VTXMESH) * vCount);
    pPoint += sizeof(VTXMESH) * vCount;


    indices->resize(iCount);
    memcpy(indices->data(), pPoint, sizeof(uint32_t) * iCount);
    pPoint += sizeof(uint32_t) * iCount;

    m_iMaterialIndex = materialIndex;
    //m_iNumVertexBuffers = 1;
    m_iNumVertices = vCount;

    m_iNumIndices = (UINT)indices->size();
    m_iIndexStride = sizeof(uint32_t);
    m_eIndexFormat = DXGI_FORMAT_R32_UINT;
    m_ePrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;


    for (size_t i = 0; i < m_iNumVertices; i++)
    {

        XMStoreFloat3(&(*vertexes)[i].vPosition, XMVector3TransformCoord(XMLoadFloat3(&(*vertexes)[i].vPosition), PreTransformMatrix));

        XMStoreFloat3(&(*vertexes)[i].vNormal, XMVector3TransformNormal(XMLoadFloat3(&(*vertexes)[i].vNormal), PreTransformMatrix));


        //----------------------- 더 추가 할 예정 ----------------------------------------------------------------------------------
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
    VertexInitialData.pSysMem = vertexes->data();

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
    IndexInitialData.pSysMem = indices->data();


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
