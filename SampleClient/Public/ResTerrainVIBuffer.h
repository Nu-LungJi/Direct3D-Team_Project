#pragma once

#include "ResVIBuffer.h"

NS_BEGIN(Client)

class CResTerrainVIBuffer : public CResVIBuffer
{
public:
    DECLARE_DERIVED_TYPE(CResTerrainVIBuffer, CResVIBuffer)

public:
	typedef struct tagDesc
	{

	} DESC;

protected:
    explicit CResTerrainVIBuffer(
        const _string& sPath,
        ComPtr<ID3D11Device> pDevice,
        ComPtr<ID3D11DeviceContext> pContext);
    ~CResTerrainVIBuffer() override;


public:
    HRESULT Load(const std::any& arg = {}) override;
    HRESULT Unload(const std::any& arg = {})  override;

public:
    const std::vector<VTX_NORMAL_TEX>& GetVertices() const { return m_vecVertices; }
    const std::vector<uint32_t>& GetIndices() const { return m_vecIndices; }

private:
    uint32_t			m_iNumVerticesX = {};
    uint32_t			m_iNumVerticesZ = {};
    //std::shared_ptr<VTX_NORMAL_TEX[]> m_pVertices{};
    std::vector<VTX_NORMAL_TEX> m_vecVertices{};
    std::vector<uint32_t> m_vecIndices{};

public:
    static SPtr<CResTerrainVIBuffer> Create(const _string& sPath);
};

NS_END