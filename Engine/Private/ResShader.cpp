#include "pch.h"
#include "ResShader.h"

NS_USING(Engine)

CResShader::CResShader(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
	: CResource{ sPath }
    , m_pDevice{ pDevice }
    , m_pContext{ pContext }
{
}

CResShader::~CResShader()
{
}

HRESULT CResShader::CompileShader()
{
    uint32_t        iFlag = {};
    iFlag |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
    iFlag |= D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    iFlag |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    //iFlag |= D3DCOMPILE_OPTIMIZATION_LEVEL1;
    iFlag |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

    HRESULT hr = D3DCompileFromFile(
        StringToWString(m_sPath).c_str(),
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        m_sEntryPoint.c_str(),
        m_sTarget.c_str(),
        iFlag,
        0,
        &m_pBlob,
        &m_pErrorBlob
    );
    if (FAILED(hr))
    {
        if (m_pErrorBlob)
        {
            const char* err = (const char*)m_pErrorBlob->GetBufferPointer();
            std::string str = err;
            MSG_BOX_STR(StringToWString(str).c_str());
            OutputDebugStringA(err);
        }
        else
        {
            MSG_BOX("CompileShader FAILED");
        }
        return E_FAIL;
    }
    return S_OK;
}
//
//HRESULT CAssetShader::Load()
//{
//    m_eState = STATE::LOADING;
//    if (FAILED(D3DCompileFromFile(
//        m_sPath.c_str(),
//        nullptr,
//        D3D_COMPILE_STANDARD_FILE_INCLUDE,
//        m_sEntryPoint.c_str(),
//        m_sTarget.c_str(),
//        D3DCOMPILE_ENABLE_STRICTNESS,
//        0,
//        &m_Blob,
//        &m_ErrorBlob
//    )))
//    {
//        m_eState = STATE::LOADFAIL;
//        return E_FAIL;
//    }
//    m_eState = STATE::LOADED;
//    return S_OK;
//}
//
//HRESULT CAssetShader::Unload()
//{
//    m_Blob.Reset();
//    m_ErrorBlob.Reset();
//    m_eState = STATE::UNLOAD;
//    return S_OK;
//}
