#include "pch.h"
#include "ResModelMesh.h"

#include <fstream>

NS_USING(Engine)

CResModelMesh::CResModelMesh(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
    : CResVIBuffer{ sPath, pDevice,pContext }
{
}

CResModelMesh::~CResModelMesh()
{
}

HRESULT CResModelMesh::Load(const std::any& arg)
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
        if (FAILED(Ready_AnimMesh(pModel, ptr, descArg->iRecordSize)))
            return E_FAIL;
    }
   

    m_eState = STATE::LOADED;
    return S_OK;
}

HRESULT CResModelMesh::Unload(const std::any& arg)
{
	m_MorphTargets.clear();
	m_pMorphDeltaBuffer.reset();
	m_pMorphTargetRangeBuffer.reset();
    m_eState = STATE::UNLOAD;
    return S_OK;
}

HRESULT CResModelMesh::Ready_AnimMesh(CResModel* pModel, _char* pPoint, uint32_t iRecordSize)
{
	_char* const pRecordBegin = pPoint;
	_char* const pRecordEnd = pRecordBegin + iRecordSize;
    uint32_t materialIndex = 0;
    memcpy(&materialIndex, pPoint, sizeof(uint32_t));
    pPoint += sizeof(uint32_t);

    uint32_t vCount = 0;
    memcpy(&vCount, pPoint, sizeof(uint32_t));
    pPoint += sizeof(uint32_t);

    uint32_t iCount = 0;
    memcpy(&iCount, pPoint, sizeof(uint32_t));
    pPoint += sizeof(uint32_t);

    const VTXANIMMESH* pVertexData =
        reinterpret_cast<const VTXANIMMESH*>(pPoint);
    pPoint += sizeof(VTXANIMMESH) * vCount;

    const uint32_t* pIndexData =
        reinterpret_cast<const uint32_t*>(pPoint);
    pPoint += sizeof(uint32_t) * iCount;

    memcpy(&m_iNumBones, pPoint, sizeof(uint32_t));
    pPoint += sizeof(uint32_t);

    uint32_t BoneIndicesCount = 0;
    memcpy(&BoneIndicesCount, pPoint, sizeof(uint32_t));
    pPoint += sizeof(uint32_t);

    uint32_t BoneMatricesCount = 0;
    memcpy(&BoneMatricesCount, pPoint, sizeof(uint32_t));
    pPoint += sizeof(uint32_t);

    uint32_t OffsetMatricesCount = 0;
    memcpy(&OffsetMatricesCount, pPoint, sizeof(uint32_t));
    pPoint += sizeof(uint32_t);

    m_BoneIndices.resize(BoneIndicesCount);
    memcpy(m_BoneIndices.data(), pPoint, sizeof(uint32_t) * BoneIndicesCount);
    pPoint += sizeof(uint32_t) * BoneIndicesCount;

    m_BoneMatrices.resize(BoneMatricesCount);
    memcpy(m_BoneMatrices.data(), pPoint, sizeof(_float4x4) * BoneMatricesCount);
    pPoint += sizeof(_float4x4) * BoneMatricesCount;

    m_OffsetMatrices.resize(OffsetMatricesCount);
    memcpy(m_OffsetMatrices.data(), pPoint, sizeof(_float4x4) * OffsetMatricesCount);
    pPoint += sizeof(_float4x4) * OffsetMatricesCount;

	m_MorphTargets.clear();

	auto CanRead = [&pPoint, pRecordEnd](size_t iByteSize)
		{
			return pPoint <= pRecordEnd &&
				iByteSize <= static_cast<size_t>(pRecordEnd - pPoint);
		};

	if (CanRead(sizeof(uint32_t) * 2))
	{
		uint32_t iMagic = 0;
		memcpy(&iMagic, pPoint, sizeof(uint32_t));

		if (MORPH_BINARY_MAGIC == iMagic)
		{
			pPoint += sizeof(uint32_t);

			uint32_t iMorphTargetCount = 0;
			memcpy(&iMorphTargetCount, pPoint, sizeof(uint32_t));
			pPoint += sizeof(uint32_t);

			constexpr size_t MORPH_TARGET_MIN_BYTE_SIZE = sizeof(uint32_t) * 2;
			if (iMorphTargetCount >
				static_cast<size_t>(pRecordEnd - pPoint) / MORPH_TARGET_MIN_BYTE_SIZE)
				return E_FAIL;

			m_MorphTargets.reserve(iMorphTargetCount);

			for (uint32_t iMorphTargetIndex = 0;
				iMorphTargetIndex < iMorphTargetCount;
				++iMorphTargetIndex)
			{
				if (!CanRead(sizeof(uint32_t)))
					return E_FAIL;

				uint32_t iNameLength = 0;
				memcpy(&iNameLength, pPoint, sizeof(uint32_t));
				pPoint += sizeof(uint32_t);

				if (!CanRead(iNameLength))
					return E_FAIL;

				MORPH_TARGET MorphTarget{};
				MorphTarget.sName.assign(pPoint, iNameLength);
				pPoint += iNameLength;

				if (!CanRead(sizeof(uint32_t)))
					return E_FAIL;

				uint32_t iMorphDeltaCount = 0;
				memcpy(&iMorphDeltaCount, pPoint, sizeof(uint32_t));
				pPoint += sizeof(uint32_t);

				if (iMorphDeltaCount >
					static_cast<size_t>(pRecordEnd - pPoint) / sizeof(MORPH_VERTEX_DELTA))
					return E_FAIL;

				MorphTarget.Deltas.resize(iMorphDeltaCount);
				if (iMorphDeltaCount > 0)
				{
					const size_t iMorphDeltaByteSize =
						sizeof(MORPH_VERTEX_DELTA) * iMorphDeltaCount;
					memcpy(MorphTarget.Deltas.data(), pPoint, iMorphDeltaByteSize);
					pPoint += iMorphDeltaByteSize;
				}

				for (const MORPH_VERTEX_DELTA& MorphDelta : MorphTarget.Deltas)
				{
					if (MorphDelta.iVertexIndex >= vCount)
						return E_FAIL;
				}

				m_MorphTargets.push_back(std::move(MorphTarget));
			}
		}
	}

	if (FAILED(Ready_MorphBuffers()))
		return E_FAIL;

    m_iMaterialIndex = materialIndex;
    m_iNumVertices = vCount;
    m_iNumIndices = iCount;
    m_iIndexStride = sizeof(uint32_t);
    m_eIndexFormat = DXGI_FORMAT_R32_UINT;
    m_ePrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    m_iVertexStride = sizeof(VTXANIMMESH);

    D3D11_BUFFER_DESC VertexBufferDesc{};
    VertexBufferDesc.ByteWidth = m_iNumVertices * sizeof(VTXANIMMESH);
    VertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    VertexBufferDesc.StructureByteStride = m_iVertexStride;

    D3D11_SUBRESOURCE_DATA VertexInitialData{};
    VertexInitialData.pSysMem = pVertexData;

    if (FAILED(CreateVertexBuffer(VertexBufferDesc, &VertexInitialData)))
    {
        m_eState = STATE::LOADFAIL;
        return E_FAIL;
    }
    {
        CResStructuredBuffer::DESC skinInputDesc{};
        skinInputDesc.iNumElements = m_iNumVertices;
        skinInputDesc.iStructureByteStride = sizeof(VTXANIMMESH);
        skinInputDesc.iBindFlags = D3D11_BIND_SHADER_RESOURCE;
        skinInputDesc.pInitialData = pVertexData;
        m_pSkinningInputBuffer = CResStructuredBuffer::Create();
        if (!m_pSkinningInputBuffer || FAILED(m_pSkinningInputBuffer->Load(skinInputDesc)))
            return E_FAIL;


    }

    D3D11_BUFFER_DESC IndexBufferDesc{};
    IndexBufferDesc.ByteWidth = m_iNumIndices * m_iIndexStride;
    IndexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    IndexBufferDesc.StructureByteStride = m_iIndexStride;

    D3D11_SUBRESOURCE_DATA IndexInitialData{};
    IndexInitialData.pSysMem = pIndexData;

    if (FAILED(CreateIndexBuffer(IndexBufferDesc, &IndexInitialData)))
    {
        m_eState = STATE::LOADFAIL;
        return E_FAIL;
    }

    return S_OK;
}

HRESULT CResModelMesh::Ready_MorphBuffers()
{
	m_pMorphDeltaBuffer.reset();
	m_pMorphTargetRangeBuffer.reset();

	if (m_MorphTargets.empty())
		return S_OK;

	std::vector<MORPH_VERTEX_DELTA> MorphDeltas;
	std::vector<GPU_MORPH_TARGET_RANGE> MorphTargetRanges;
	MorphTargetRanges.reserve(m_MorphTargets.size());

	size_t iTotalMorphDeltaCount = 0;
	for (const MORPH_TARGET& MorphTarget : m_MorphTargets)
	{
		iTotalMorphDeltaCount += MorphTarget.Deltas.size();
		if (iTotalMorphDeltaCount > UINT32_MAX)
			return E_FAIL;
	}
	MorphDeltas.reserve(iTotalMorphDeltaCount);

	for (const MORPH_TARGET& MorphTarget : m_MorphTargets)
	{
		GPU_MORPH_TARGET_RANGE MorphTargetRange{};
		MorphTargetRange.iDeltaOffset = static_cast<uint32_t>(MorphDeltas.size());
		MorphTargetRange.iDeltaCount = static_cast<uint32_t>(MorphTarget.Deltas.size());
		MorphTargetRanges.push_back(MorphTargetRange);

		MorphDeltas.insert(
			MorphDeltas.end(),
			MorphTarget.Deltas.begin(),
			MorphTarget.Deltas.end());
	}

	if (!MorphDeltas.empty())
	{
		CResStructuredBuffer::DESC MorphDeltaBufferDesc{};
		MorphDeltaBufferDesc.iNumElements = static_cast<uint32_t>(MorphDeltas.size());
		MorphDeltaBufferDesc.iStructureByteStride = sizeof(MORPH_VERTEX_DELTA);
		MorphDeltaBufferDesc.pInitialData = MorphDeltas.data();
		MorphDeltaBufferDesc.iBindFlags = D3D11_BIND_SHADER_RESOURCE;

		m_pMorphDeltaBuffer = CResStructuredBuffer::Create();
		if (nullptr == m_pMorphDeltaBuffer ||
			FAILED(m_pMorphDeltaBuffer->Load(MorphDeltaBufferDesc)))
			return E_FAIL;
	}

	CResStructuredBuffer::DESC MorphTargetRangeBufferDesc{};
	MorphTargetRangeBufferDesc.iNumElements = static_cast<uint32_t>(MorphTargetRanges.size());
	MorphTargetRangeBufferDesc.iStructureByteStride = sizeof(GPU_MORPH_TARGET_RANGE);
	MorphTargetRangeBufferDesc.pInitialData = MorphTargetRanges.data();
	MorphTargetRangeBufferDesc.iBindFlags = D3D11_BIND_SHADER_RESOURCE;

	m_pMorphTargetRangeBuffer = CResStructuredBuffer::Create();
	if (nullptr == m_pMorphTargetRangeBuffer ||
		FAILED(m_pMorphTargetRangeBuffer->Load(MorphTargetRangeBufferDesc)))
		return E_FAIL;

	return S_OK;
}
SPtr<CResStructuredBuffer> CResModelMesh::GetSkinningInputBuffer() const
{
    return m_pSkinningInputBuffer;
}

HRESULT CResModelMesh::EnsureSkinnedVertexBuffer(uint32_t iInstanceCapacity)
{
    if (iInstanceCapacity == 0 || m_iNumVertices == 0)
        return E_INVALIDARG;

    if (m_pSkinnedVertexBuffer && m_iSkinnedVertexInstanceCapacity >= iInstanceCapacity)
        return S_OK;

    CResStructuredBuffer::DESC desc{};
    desc.iNumElements = m_iNumVertices * iInstanceCapacity;
    desc.iStructureByteStride = sizeof(_float4) * 4;
    desc.iBindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

    auto pNewBuffer = CResStructuredBuffer::Create();
    if (!pNewBuffer || FAILED(pNewBuffer->Load(desc)))
        return E_FAIL;

    m_pSkinnedVertexBuffer = std::move(pNewBuffer);
    m_iSkinnedVertexInstanceCapacity = iInstanceCapacity;
    return S_OK;
}
SPtr<CResStructuredBuffer> CResModelMesh::GetSkinnedVertexBuffer() const
{
    return m_pSkinnedVertexBuffer;
}
SPtr<CResModelMesh> CResModelMesh::Create()
{
    return ToSPtr(new CResModelMesh{ "",CGameInstance::Get().GetGraphicDevice(),CGameInstance::Get().GetGraphicDeviceContext() });
}
