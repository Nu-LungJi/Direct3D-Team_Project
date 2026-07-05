#include "pch.h"
#include "Particle_GPU.h"
#include "GameInstance.h"
#include "Resources.h"
#include "Particle_CPU.h"

NS_USING(Engine)

CParticle_GPU::CParticle_GPU()
{
}

CParticle_GPU::~CParticle_GPU()
{
}

HRESULT CParticle_GPU::Initialize(void* pArg)
{
    auto context = CGameInstance::Get().GetGraphicDeviceContext();

    auto pDesc = static_cast<DESC*>(pArg);
    if (pDesc == nullptr)
        return E_FAIL;

    m_Desc = *pDesc;

    // 예전: m_iNumElements = 1000; (하드코딩) → 이제 DESC에서 주입
    m_iNumElements = m_Desc.iMaxParticles;
    m_eType = pDesc->type;
    // 파티클을 다 죽은 상태로 초기화
    std::vector<PARTICLE> initParticles(m_iNumElements);
    for (uint32_t i = 0; i < m_iNumElements; i++)
    {
        initParticles[i].position = _float3(0.f, 0.f, 0.f);
        initParticles[i].velocity = _float3(0.f, 0.f, 0.f);
        initParticles[i].life = 0.f;
        initParticles[i].maxLife = 0.f;
        initParticles[i].size = 0.5f;
        initParticles[i].color = _float4(1, 1, 1, 1);
        initParticles[i].alive = false;
        initParticles[i].loop = false;
        initParticles[i].texIndex = 0;
    }

    std::vector<uint32_t> initDeadIndices(m_iNumElements);
    for (uint32_t i = 0; i < m_iNumElements; i++)
        initDeadIndices[i] = i;

    // 파티클 구조체 버퍼
    if (auto res = CResStructuredBuffer::Create())
    {
        CResStructuredBuffer::DESC bufDesc{};
        bufDesc.iNumElements = m_iNumElements;
        bufDesc.iStructureByteStride = sizeof(PARTICLE);
        bufDesc.pInitialData = initParticles.data();
        bufDesc.bAppendConsume = false;
        if (FAILED(res->Load(bufDesc)))
            return E_FAIL;
        m_pParticleStructuredBuffer = res;
    }

    // 죽은 파티클 인덱스 버퍼
    if (auto res = CResStructuredBuffer::Create())
    {
        CResStructuredBuffer::DESC bufDesc{};
        bufDesc.iNumElements = m_iNumElements;
        bufDesc.iStructureByteStride = sizeof(uint32_t);
        bufDesc.pInitialData = initDeadIndices.data();
        bufDesc.bAppendConsume = true;
        if (FAILED(res->Load(bufDesc)))
            return E_FAIL;
        m_pDeadListBuffer = res;
    }

    // 스폰 데이터 버퍼
    if (auto res = CResStructuredBuffer::Create())
    {
        std::vector<PARTICLE_SPAWN_DATA> initSpawnData(MAX_SPAWN_PER_CALL);

        CResStructuredBuffer::DESC bufDesc{};
        bufDesc.iNumElements = MAX_SPAWN_PER_CALL;
        bufDesc.iStructureByteStride = sizeof(PARTICLE_SPAWN_DATA);
        bufDesc.pInitialData = initSpawnData.data();
        bufDesc.bAppendConsume = false;
        if (FAILED(res->Load(bufDesc)))
            return E_FAIL;
        m_pSpawnListBuffer = res;
    }
    {
        m_pComCBuffer = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_PARTICLE);
        if (!m_pComCBuffer)
            return E_FAIL;

        m_pComSpawnCBuffer = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_SPAWN_PARTICLE);
        if (!m_pComSpawnCBuffer)
            return E_FAIL;
    }
   
    // 상수 버퍼 2종 (Update용 / Spawn용) - 그대로 재사용
    //{
    //    CComConstantBuffer::DESC cbDesc{};
    //    cbDesc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_PARTICLE };
    //    m_pComCBuffer = CComConstantBuffer::Create(cbDesc);
    //    if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerParticle", &cbDesc, &m_pComCBuffer)))
    //        return E_FAIL;
    //}
    //{
    //    CComConstantBuffer::DESC cbDesc{};
    //    cbDesc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_SPAWN_PARTICLE };
    //    if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferSpawnParticle", &cbDesc, &m_pComSpawnCBuffer)))
    //        return E_FAIL;
    //}

   //텍스쳐 로드
    if (FAILED(LoadParticleTexture(m_Desc.textureID)))
        return E_FAIL;

    // 셰이더 3종(VS/PS) + CS 3종은 모든 GPU 파티클이 공유하는 범용 파이프라인이라 고정
    m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>("SAMPLE_CLIENT_SHADER", "VS_VTX_PARTICLE_TEX");
    if (FAILED(m_pResVertexShader->Load()))
        return E_FAIL;

    m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>("SAMPLE_CLIENT_SHADER", "PS_VTX_PARTICLE_TEX");
    if (FAILED(m_pResPixelShader->Load()))
        return E_FAIL;

    m_pResUpdateComputeShader = CGameInstance::Get().GetResourceFirst<CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_UpdateParticle");
    if (FAILED(m_pResUpdateComputeShader->Load()))
        return E_FAIL;

    m_pResSpawnComputeShader = CGameInstance::Get().GetResourceFirst<CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_SpawnParticle");
    if (FAILED(m_pResSpawnComputeShader->Load()))
        return E_FAIL;

    m_pResInitDeadCS = CGameInstance::Get().GetResourceFirst<CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_InitParticle");
    if (FAILED(m_pResInitDeadCS->Load()))
        return E_FAIL;

    m_pResSamplerState = CGameInstance::Get().GetResourceFirst<CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
    if (!m_pResSamplerState)
        return E_FAIL;

    // 죽은 파티클 초기화 (딱 한 번, InitDead CS Dispatch)
    ID3D11UnorderedAccessView* uav = m_pDeadListBuffer->GetUAV().Get();
    UINT initCount = m_iNumElements;
    context->CSSetUnorderedAccessViews(0, 1, &uav, &initCount);
    context->CSSetShader(m_pResInitDeadCS->GetComputeShader().Get(), nullptr, 0);

    uint32_t group = (m_iNumElements + 255) / 256;
    context->Dispatch(group, 1, 1);

    ID3D11UnorderedAccessView* nullUAV[] = { nullptr };
    context->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
    context->CSSetShader(nullptr, nullptr, 0);

    return S_OK;
}

void CParticle_GPU::PriorityUpdate(E::_float fTimeDelta)
{
}

void CParticle_GPU::Update(E::_float fTimeDelta)
{
    auto pContext = CGameInstance::Get().GetGraphicDeviceContext();
    UINT initialCounts[] = { (UINT)-1, (UINT)-1 };
    ID3D11UnorderedAccessView* nullUAVs[] = { nullptr, nullptr };

    // 1. 스폰
    if (m_iCurrentSpawnCount > 0)
    {
        pContext->CSSetConstantBuffers(6, 1, m_pComSpawnCBuffer->GetCBuffer().GetAddressOf());

        ID3D11ShaderResourceView* spawnSRV = m_pSpawnListBuffer->GetSRV().Get();
        pContext->CSSetShaderResources(0, 1, &spawnSRV);

        ID3D11UnorderedAccessView* spawnUAVs[] = {
            m_pDeadListBuffer->GetUAV().Get(),
            m_pParticleStructuredBuffer->GetUAV().Get()
        };
        UINT spawnInitialCounts[] = { (UINT)-1, (UINT)-1 };
        pContext->CSSetUnorderedAccessViews(0, 2, spawnUAVs, spawnInitialCounts);

        pContext->CSSetShader(m_pResSpawnComputeShader->GetComputeShader().Get(), nullptr, 0);

        uint32_t spawnGroup = (m_iCurrentSpawnCount + 255) / 256;
        pContext->Dispatch(spawnGroup, 1, 1);

        ID3D11UnorderedAccessView* nullUAVs2[] = { nullptr, nullptr };
        pContext->CSSetUnorderedAccessViews(0, 2, nullUAVs2, nullptr);

        ID3D11ShaderResourceView* nullSRV[] = { nullptr };
        pContext->CSSetShaderResources(0, 1, nullSRV);

        m_iCurrentSpawnCount = 0;
    }

    // 2. Update
    CB_PER_PARTICLE cb{};
    cb.g_fTimeDelta = fTimeDelta;
    cb.g_iNumInstances = m_iNumElements;
    cb.g_iBehaviorType = m_Desc.iBehaviorType;

    {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(pContext->Map(m_pComCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            memcpy(mapped.pData, &cb, sizeof(cb));
            pContext->Unmap(m_pComCBuffer->GetCBuffer().Get(), 0);
            pContext->CSSetConstantBuffers(5, 1, m_pComCBuffer->GetCBuffer().GetAddressOf());
        }
    }

    ID3D11UnorderedAccessView* updateUAVs[] = {
        m_pDeadListBuffer->GetUAV().Get(),
        m_pParticleStructuredBuffer->GetUAV().Get()
    };
    pContext->CSSetUnorderedAccessViews(0, 2, updateUAVs, initialCounts);

    pContext->CSSetShader(m_pResUpdateComputeShader->GetComputeShader().Get(), nullptr, 0);

    uint32_t groupX = (m_iNumElements + 255) / 256;
    pContext->Dispatch(groupX, 1, 1);

    ID3D11Buffer* nullCBuffer[] = { nullptr, nullptr };
    pContext->CSSetConstantBuffers(5, 2, nullCBuffer);
    pContext->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);
    pContext->CSSetShader(nullptr, nullptr, 0);
}

void CParticle_GPU::LateUpdate(E::_float fTimeDelta)
{
}

HRESULT CParticle_GPU::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
    pContext->VSSetShader(m_pResVertexShader->GetVertexShader().Get(), nullptr, 0);
    pContext->PSSetShader(m_pResPixelShader->GetPixelShader().Get(), nullptr, 0);

    pContext->IASetInputLayout(nullptr);
    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    ID3D11ShaderResourceView* pSRV = m_pParticleStructuredBuffer->GetSRV().Get();
    pContext->VSSetShaderResources(0, 1, &pSRV);

    pContext->PSSetShaderResources(1, 1, m_pParticleTexture->GetSRV().GetAddressOf());
    pContext->PSSetSamplers(0, 1, m_pResSamplerState->GetSamplerState().GetAddressOf());

    pContext->DrawInstanced(4, m_iNumElements, 0, 0);

    ID3D11ShaderResourceView* nullSRV2[] = { nullptr, nullptr };
    ID3D11ShaderResourceView* nullSRV1[] = { nullptr };
    pContext->VSSetShaderResources(0, 2, nullSRV2);
    pContext->PSSetShaderResources(1, 1, nullSRV1);

    return S_OK;
}


HRESULT CParticle_GPU::Spawn(uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData)
{
    if (pSpawnData == nullptr || count == 0)
        return E_FAIL;

    if (count > MAX_SPAWN_PER_CALL)
        count = MAX_SPAWN_PER_CALL;

    auto context = CGameInstance::Get().GetGraphicDeviceContext();

    std::vector<PARTICLE_SPAWN_DATA> fullData(MAX_SPAWN_PER_CALL);
    memcpy(fullData.data(), pSpawnData, sizeof(PARTICLE_SPAWN_DATA) * count);

    context->UpdateSubresource(m_pSpawnListBuffer->GetBuffer().Get(), 0, nullptr, fullData.data(), 0, 0);

    CB_PARTICLE_SPAWN scb{};
    scb.g_iSpawnCount = count;

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(context->Map(m_pComSpawnCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        return E_FAIL;

    memcpy(mapped.pData, &scb, sizeof(scb));
    context->Unmap(m_pComSpawnCBuffer->GetCBuffer().Get(), 0);

    m_iCurrentSpawnCount = count;
    return S_OK;
}
