#include "pch.h"
#include "Particle_CPU.h"
#include "GameInstance.h"
#include "Resources.h"

NS_USING(Engine)

CParticle_CPU::CParticle_CPU()
{
}

CParticle_CPU::CParticle_CPU(const CParticle_CPU& rhs)
    : CParticle(rhs)
    , m_Desc{ rhs.m_Desc }
    , m_iNumElements{ rhs.m_iNumElements }
    , m_viBufferID{ rhs.m_viBufferID }
{
}

CParticle_CPU::~CParticle_CPU()
{
}

HRESULT CParticle_CPU::Initialize(void* pArg)
{
    m_vecInstancedData.clear();



    auto pDesc = static_cast<DESC*>(pArg);
    if (pDesc == nullptr)
        return E_FAIL;

    m_Desc = *pDesc;
    m_iNumElements = m_Desc.iMaxParticles;
    m_viBufferID = m_Desc.viBufferID;
    m_eType = pDesc->type;
    m_Particles.assign(m_iNumElements, PARTICLE_CPU_DATA{});

    if (auto res = CResDynamicBuffer::Create())
    {
        CResDynamicBuffer::DESC bufDesc{};
        bufDesc.desc = {
            .ByteWidth = (uint32_t)sizeof(VTX_PARTICLE_INSTANCED_DATA) * m_iNumElements,
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_VERTEX_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
            .MiscFlags = 0,
            .StructureByteStride = 0,
        };

        if (FAILED(res->Load(bufDesc)))
            return E_FAIL;

        m_pResInstancedBuffer = res;
    }

    m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(pDesc->VSID.first, pDesc->VSID.second);
    if (FAILED(m_pResVertexShader->Load()))
        return E_FAIL;

    m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(pDesc->PSID.first, pDesc->PSID.second);
    if (FAILED(m_pResPixelShader->Load()))
        return E_FAIL;


    if (FAILED(LoadParticleTexture(m_Desc.textureID)))
        return E_FAIL;

    return S_OK;
}

void CParticle_CPU::PriorityUpdate(E::_float fTimeDelta)
{
}

void CParticle_CPU::Update(E::_float fTimeDelta)
{
    Simulate(fTimeDelta);
}

void CParticle_CPU::LateUpdate(E::_float fTimeDelta)
{
}

void CParticle_CPU::Simulate(E::_float fTimeDelta)
{
    m_vecInstancedData.clear();

    for (auto& p : m_Particles)
    {
        if (!p.bAlive)
            continue;

        p.fAge += fTimeDelta;
        if (p.fAge >= p.fLifeTime)
        {
            p.bAlive = false;
            continue;
        }

        UpdateBehavior(p, fTimeDelta);

        if (m_vecInstancedData.size() >= m_iNumElements)
            continue;

        VTX_PARTICLE_INSTANCED_DATA inst{};
        _matrix matScale = XMMatrixScaling(p.fSize, p.fSize, p.fSize);
        _matrix matWorld = XMMatrixTranslation(p.vPosition.x, p.vPosition.y, p.vPosition.z);
        XMStoreFloat4x4(&inst.matWorld, matScale * matWorld);
        inst.vColor = p.vColor;

        m_vecInstancedData.push_back(inst);
    }
}

HRESULT CParticle_CPU::Spawn(uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData)
{
    if (pSpawnData == nullptr || count == 0)
        return E_FAIL;

    uint32_t iSpawned = 0;
    for (uint32_t i = 0; i < m_Particles.size() && iSpawned < count; ++i)
    {
        if (m_Particles[i].bAlive)
            continue;

        const auto& src = pSpawnData[iSpawned];
        m_Particles[i].vPosition = src.position;
        m_Particles[i].vVelocity = src.velocity;
        m_Particles[i].fLifeTime = src.life;
        m_Particles[i].fAge = 0.f;
        m_Particles[i].bAlive = true;
        m_Particles[i].fSize = src.size;

        ++iSpawned;
    }

    return (iSpawned == count) ? S_OK : E_FAIL;
}

HRESULT CParticle_CPU::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
    if (m_vecInstancedData.empty())
        return S_OK;

    const auto& viBuffer = CGameInstance::Get().GetResourceFirst<CResVIBuffer>(m_viBufferID.first, m_viBufferID.second);

    pContext->IASetInputLayout(m_pResVertexShader->GetInputLayout().Get());
    pContext->VSSetShader(m_pResVertexShader->GetVertexShader().Get(), nullptr, 0);
    pContext->PSSetShader(m_pResPixelShader->GetPixelShader().Get(), nullptr, 0);

    ID3D11Buffer* vertexBuffers[] = {
        viBuffer->GetVertexBuffer().Get(),
        m_pResInstancedBuffer->GetBuffer().Get()
    };
    uint32_t strides[] = {
        viBuffer->GetVertexStride(),
        (uint32_t)sizeof(VTX_PARTICLE_INSTANCED_DATA),
    };
    uint32_t offsets[] = { 0, 0 };

    pContext->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);
    pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
    pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

    {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(pContext->Map(m_pResInstancedBuffer->GetBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            std::memcpy(mapped.pData, m_vecInstancedData.data(),
                sizeof(VTX_PARTICLE_INSTANCED_DATA) * m_vecInstancedData.size());
            pContext->Unmap(m_pResInstancedBuffer->GetBuffer().Get(), 0);
        }
    }

    pContext->PSSetShaderResources(0, 1, m_pParticleTexture->GetSRV().GetAddressOf());

    {
        const auto& sampler = CGameInstance::GetConst().GetResourceFirst<CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
        pContext->PSSetSamplers(0, 1, sampler->GetSamplerState().GetAddressOf());
    }

    pContext->DrawIndexedInstanced((UINT)viBuffer->GetNumIndices(), (UINT)m_vecInstancedData.size(), 0, 0, 0);

    ID3D11ShaderResourceView* nullSRV[] = { nullptr };
    pContext->PSSetShaderResources(0, 1, nullSRV);

    return S_OK;
}