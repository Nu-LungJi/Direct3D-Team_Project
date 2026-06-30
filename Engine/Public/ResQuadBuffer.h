#pragma once

#include "ResVIBuffer.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResQuadBuffer : public CResVIBuffer
{
public:
    DECLARE_DERIVED_TYPE(CResQuadBuffer, CResVIBuffer)

protected:
    explicit CResQuadBuffer(
        const _string& sPath,
        ComPtr<ID3D11Device> pDevice,
        ComPtr<ID3D11DeviceContext> pContext)
        : CResVIBuffer(sPath, pDevice, pContext) { }

    ~CResQuadBuffer() override = default;
};

NS_END