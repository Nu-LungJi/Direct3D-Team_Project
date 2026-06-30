#pragma once

#include "ResVIBuffer.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResCubeBuffer : public CResVIBuffer
{
public:
    DECLARE_DERIVED_TYPE(CResCubeBuffer, CResVIBuffer)

protected:
    explicit CResCubeBuffer(
        const _string& sPath,
        ComPtr<ID3D11Device> pDevice,
        ComPtr<ID3D11DeviceContext> pContext)
        : CResVIBuffer(sPath, pDevice, pContext) {
    }
    ~CResCubeBuffer() override = default;
};

NS_END