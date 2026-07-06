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
    }

    std::vector<uint32_t> initDeadIndices(m_iNumElements);
    for (uint32_t i = 0; i < m_iNumElements; i++)
        initDeadIndices[i] = i;

    //Init 버퍼
    m_pComInitCBuffer = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_INIT_PARTICLE);
    if (!m_pComInitCBuffer)
        return E_FAIL;

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
  


    if (m_Desc.whatKind == MESHORTEXTURE::TEX) {

        m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(pDesc->VSID.first, pDesc->VSID.second);
        if (FAILED(m_pResVertexShader->Load()))
            return E_FAIL;

        m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(pDesc->PSID.first, pDesc->PSID.second);
        if (FAILED(m_pResPixelShader->Load()))
            return E_FAIL;
    
        if (FAILED(LoadParticleTexture(m_Desc.textureID)))
            return E_FAIL;

    }
    else if (m_Desc.whatKind == MESHORTEXTURE::MESH) {


        m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(pDesc->VSID.first, pDesc->VSID.second);
        if (FAILED(m_pResVertexShader->Load()))
            return E_FAIL;

        m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(pDesc->PSID.first, pDesc->PSID.second);
        if (FAILED(m_pResPixelShader->Load()))
            return E_FAIL;




        // 모델 인스턴스는 컴포넌트 프로토타입 clone이 필요하다면 아래처럼
        // (AddComponentFromProto 대신, GameObject 없이도 쓸 수 있는 형태로)
        {

            CComStaticModelInstance::DESC Desc{};
            Desc.sGroupTag = m_Desc.sGroupTag;
            Desc.sResTag = m_Desc.sResTag;
            //Desc.sGroupTag = "TEST";
            //Desc.sResTag = "Static_Model_Resource";

            auto pProto = CGameInstance::Get().ClonePrototype("PERMANENT", "Prototype_Component_StaticModelInstance", &Desc);
            if (pProto == nullptr)
                return E_FAIL;
            m_pComModelInstance = UPtr<CComStaticModelInstance>(static_cast<CComStaticModelInstance*>(pProto.release()));

            if (!m_pComModelInstance)
                return E_FAIL;
        }
    }


 



    // 셰이더 3종(VS/PS) + CS 3종은 모든 GPU 파티클이 공유하는 범용 파이프라인이라 고정


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


    //초기화 버퍼 초기화
    {
        CB_INIT_PARTICLE cb{};
        cb.g_iMaxParticles = m_iNumElements;

        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(context->Map(m_pComInitCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            memcpy(mapped.pData, &cb, sizeof(cb));
            context->Unmap(m_pComInitCBuffer->GetCBuffer().Get(), 0);
            context->CSSetConstantBuffers(0, 1, m_pComInitCBuffer->GetCBuffer().GetAddressOf());
        }
    }


    // 죽은 파티클 초기화 (딱 한 번, InitDead CS Dispatch)
    ID3D11UnorderedAccessView* uav = m_pDeadListBuffer->GetUAV().Get();
    UINT initCount = 0;
    context->CSSetUnorderedAccessViews(0, 1, &uav, &initCount);
    context->CSSetShader(m_pResInitDeadCS->GetComputeShader().Get(), nullptr, 0);

    uint32_t group = (m_iNumElements + 255) / 256;
    context->Dispatch(group, 1, 1);

    ID3D11UnorderedAccessView* nullUAV[] = { nullptr };
    context->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
    context->CSSetShader(nullptr, nullptr, 0);

    return S_OK;
}
void CParticle_GPU::DebugPrintDeadListCount()
{
    auto pDevice = CGameInstance::Get().GetGraphicDevice();
    auto pContext = CGameInstance::Get().GetGraphicDeviceContext();

    ComPtr<ID3D11Buffer> pCounterStaging;
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = 4;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    pDevice->CreateBuffer(&desc, nullptr, &pCounterStaging);

    pContext->CopyStructureCount(pCounterStaging.Get(), 0, m_pDeadListBuffer->GetUAV().Get());

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (SUCCEEDED(pContext->Map(pCounterStaging.Get(), 0, D3D11_MAP_READ, 0, &mapped)))
    {
        uint32_t counterValue = *(uint32_t*)mapped.pData;
        char buf[64];
        sprintf_s(buf, "DeadList counter = %u\n", counterValue);
        OutputDebugStringA(buf);
        pContext->Unmap(pCounterStaging.Get(), 0);
    }
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

    DebugPrintDeadListCount();
}

void CParticle_GPU::LateUpdate(E::_float fTimeDelta)
{
}

HRESULT CParticle_GPU::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
    if (m_Desc.whatKind == MESHORTEXTURE::MESH)
        return Render_Mesh(pContext, ctx);

    return Render_Texture(pContext, ctx); // 기존 텍스처 파티클 렌더 코드
}

HRESULT CParticle_GPU::Render_Mesh(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{


    ID3D11ShaderResourceView* pParticleSRV = m_pParticleStructuredBuffer->GetSRV().Get();
    pContext->VSSetShaderResources(0, 1, &pParticleSRV);

    const auto& vs = m_pResVertexShader; // 인스턴싱용 신규 VS 필요
    const auto& ps = m_pResPixelShader;
    pContext->IASetInputLayout(vs->GetInputLayout().Get());
    pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
    pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

    auto pModel = m_pComModelInstance->GetModel();
    uint32_t iNumMeshes = pModel->Get_NumMeshes();

    for (uint32_t i = 0; i < iNumMeshes; ++i)
    {
        const auto& viBuffer = pModel->GetMeshes()[i];

        ID3D11Buffer* vertexBuffers[] = { viBuffer->GetVertexBuffer().Get() };
        uint32_t strides[] = { viBuffer->GetVertexStride() };
        uint32_t offsets[] = { 0 };
        pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
        pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
        pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

        m_pComModelInstance->Bind_Materials(pContext, i, AI_TEXTURE_TYPE::aiTextureType_DIFFUSE, 0);

        pContext->PSSetSamplers(0, 1, m_pResSamplerState->GetSamplerState().GetAddressOf());

        auto rasterizer = E::CGameInstance::GetConst().GetResourceFirst<E::CResRasterizerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_NOCULL);
        pContext->RSSetState(rasterizer->GetRasterizerState().Get());

        // 핵심: DrawIndexed → DrawIndexedInstanced
        pContext->DrawIndexedInstanced(viBuffer->GetNumIndices(), m_iNumElements, 0, 0, 0);
    }

    ID3D11ShaderResourceView* nullSRV[] = { nullptr };
    pContext->VSSetShaderResources(0, 1, nullSRV);

    return S_OK;
}




HRESULT CParticle_GPU::Render_Texture(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
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

    uint32_t availableCount = GetDeadListCounterSync();
    if (availableCount == 0)
        return E_FAIL;

    if (count > availableCount)
        count = availableCount;

    if (count > MAX_SPAWN_PER_CALL)
        count = MAX_SPAWN_PER_CALL;

    auto context = CGameInstance::Get().GetGraphicDeviceContext();

    std::vector<PARTICLE_SPAWN_DATA> fullData(count);
    memcpy(fullData.data(), pSpawnData, sizeof(PARTICLE_SPAWN_DATA) * count);

    context->UpdateSubresource(m_pSpawnListBuffer->GetBuffer().Get(), 0, nullptr, fullData.data(), 0, 0);

    CB_PARTICLE_SPAWN scb{};
    scb.g_iSpawnCount = count;
    scb.g_iMaxParticles = m_Desc.iMaxParticles;

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(context->Map(m_pComSpawnCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        return E_FAIL;

    memcpy(mapped.pData, &scb, sizeof(scb));
    context->Unmap(m_pComSpawnCBuffer->GetCBuffer().Get(), 0);

    m_iCurrentSpawnCount = count;
    return S_OK;
}
uint32_t CParticle_GPU::GetDeadListCounterSync()
{
    auto pDevice = CGameInstance::Get().GetGraphicDevice();
    auto pContext = CGameInstance::Get().GetGraphicDeviceContext();

    ComPtr<ID3D11Buffer> pCounterStaging;
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = 4;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    pDevice->CreateBuffer(&desc, nullptr, &pCounterStaging);

    pContext->CopyStructureCount(pCounterStaging.Get(), 0, m_pDeadListBuffer->GetUAV().Get());

    uint32_t counterValue = 0;
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (SUCCEEDED(pContext->Map(pCounterStaging.Get(), 0, D3D11_MAP_READ, 0, &mapped)))
    {
        counterValue = *(uint32_t*)mapped.pData;
        pContext->Unmap(pCounterStaging.Get(), 0);
    }
    return counterValue;
}