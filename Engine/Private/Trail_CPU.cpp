#include "pch.h"
#include "Trail_CPU.h"
#include "GameInstance.h"
#include "Resources.h"

CTrail_CPU::CTrail_CPU()
{
}



CTrail_CPU::~CTrail_CPU()
{
}

HRESULT CTrail_CPU::Initialize(void* pArg)
{
    // CTrail_CPU는 CParticle_CPU와 같은 역할 - 버퍼/셰이더/텍스처 로딩 같은 공통 처리만 하고,
    // 실제 값(desc)은 자식 클래스(CTrail_Example 등)가 채워서 넘겨준다.
    auto pDesc = static_cast<DESC*>(pArg);
    if (pDesc == nullptr)
        return E_FAIL;

    m_Desc = *pDesc;
    m_eType = pDesc->type;


    // 프레임 하나당 정점 2개(밑동/칼끝) - 정점 자체가 이미 폭의 양 끝
    uint32_t iMaxVertices = m_Desc.iMaxFrames * 2;

    if (auto res = CResDynamicBuffer::Create())
    {
        CResDynamicBuffer::DESC bufDesc{};
        bufDesc.desc = {
            .ByteWidth = (uint32_t)sizeof(TRAIL_VERTEX) * iMaxVertices,
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_VERTEX_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
            .MiscFlags = 0,
            .StructureByteStride = 0,
        };

        if (FAILED(res->Load(bufDesc)))
            return E_FAIL;

        m_pResVertexBuffer = res;
    }

    m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(pDesc->VSID.first, pDesc->VSID.second);
    if (FAILED(m_pResVertexShader->Load()))
        return E_FAIL;

    m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(pDesc->PSID.first, pDesc->PSID.second);
    if (FAILED(m_pResPixelShader->Load()))
        return E_FAIL;

    if (FAILED(LoadParticleTexture(m_Desc.textureID)))
        return E_FAIL;

    m_vecVertices.reserve(iMaxVertices);

    return S_OK;
}

void CTrail_CPU::PriorityUpdate(_float fTimeDelta)
{
}

void CTrail_CPU::Update(_float fTimeDelta)
{
    for (auto& frame : m_dequeFrames)
        frame.fAge += fTimeDelta;

    while (!m_dequeFrames.empty() && m_dequeFrames.back().fAge >= m_Desc.fMaxDuration)
        m_dequeFrames.pop_back();

    BuildTrailGeometry();
}

void CTrail_CPU::LateUpdate(_float fTimeDelta)
{
}

void CTrail_CPU::AddPoint(const _float3& vStart, const _float3& vEnd)
{
    TRAIL_FRAME frame{};
    frame.vStart = vStart;
    frame.vEnd = vEnd;
    frame.fAge = 0.f;

    m_dequeFrames.push_front(frame);

    while (m_dequeFrames.size() > m_Desc.iMaxFrames)
        m_dequeFrames.pop_back();
}

void CTrail_CPU::Clear()
{
    m_dequeFrames.clear();
    m_vecVertices.clear();
}

HRESULT CTrail_CPU::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
    if (m_vecVertices.size() < 4) // 최소 프레임 2개(=4정점)는 있어야 스윕 면이 성립
        return S_OK;

    pContext->IASetInputLayout(m_pResVertexShader->GetInputLayout().Get());
    pContext->VSSetShader(m_pResVertexShader->GetVertexShader().Get(), nullptr, 0);
    pContext->PSSetShader(m_pResPixelShader->GetPixelShader().Get(), nullptr, 0);




    {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(pContext->Map(m_pResVertexBuffer->GetBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            std::memcpy(mapped.pData, m_vecVertices.data(),
                sizeof(TRAIL_VERTEX) * m_vecVertices.size());
            pContext->Unmap(m_pResVertexBuffer->GetBuffer().Get(), 0);
        }
    }

    ID3D11Buffer* vertexBuffers[] = { m_pResVertexBuffer->GetBuffer().Get() };
    uint32_t strides[] = { sizeof(TRAIL_VERTEX) };
    uint32_t offsets[] = { 0 };

    pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    pContext->PSSetShaderResources(0, 1, m_pParticleTexture->GetSRV().GetAddressOf());

    {
        const auto& sampler = CGameInstance::GetConst().GetResourceFirst<CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
        pContext->PSSetSamplers(0, 1, sampler->GetSamplerState().GetAddressOf());
    }

    pContext->Draw((UINT)m_vecVertices.size(), 0);

    ID3D11ShaderResourceView* nullSRV[] = { nullptr };
    pContext->PSSetShaderResources(0, 1, nullSRV);

    pContext->RSSetState(nullptr); // 다음에 그려질 오브젝트에 영향 안 주도록 기본 상태로 복구

    return S_OK;
}

HRESULT CTrail_CPU::Spawn(uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData)
{
    return E_FAIL; // AddPoint(start, end)로 직접 제어 - 이 인터페이스는 안 씀
}

void CTrail_CPU::BuildTrailGeometry()
{
    m_vecVertices.clear();

    uint32_t iCount = (uint32_t)m_dequeFrames.size();
    if (iCount < 2)
        return;

    for (uint32_t i = 0; i < iCount; ++i)
    {
        const auto& frame = m_dequeFrames[i];

        // t를 "배열 안 몇 번째냐"가 아니라 "이 점이 얼마나 늙었냐"로 계산한다.
        // 인덱스 기반이면 매 프레임 배열 크기(iCount)가 바뀔 때마다 같은 점의 t가
        // 요동쳐서 폭이 부채처럼 접혔다 펴졌다 하는 문제가 생긴다.
        // 나이는 그 점 고유의 값이라 주변에 점이 몇 개 있든 절대 안 흔들린다.
        _float fAgeRatio = frame.fAge / m_Desc.fMaxDuration; // 0(방금 생김)~1(소멸 직전) - 안정적
        _float fLifeRatio = 1.f - fAgeRatio;
        _float t = fAgeRatio; // UV 세로축도 같은 기준으로

        // 양 끝(t=0, t=1)에서 폭이 0에 가깝게, 중간에서 원래 폭이 되도록 테이퍼링.
        _float fWidthScale = sinf(t * XM_PI);

        _float3 vMid = {
            (frame.vStart.x + frame.vEnd.x) * 0.5f,
            (frame.vStart.y + frame.vEnd.y) * 0.5f,
            (frame.vStart.z + frame.vEnd.z) * 0.5f
        };

        _float3 vTip = {
            vMid.x + (frame.vEnd.x - vMid.x) * fWidthScale,
            vMid.y + (frame.vEnd.y - vMid.y) * fWidthScale,
            vMid.z + (frame.vEnd.z - vMid.z) * fWidthScale
        };
        _float3 vBase = {
            vMid.x + (frame.vStart.x - vMid.x) * fWidthScale,
            vMid.y + (frame.vStart.y - vMid.y) * fWidthScale,
            vMid.z + (frame.vStart.z - vMid.z) * fWidthScale
        };

        TRAIL_VERTEX vTop{};
        vTop.vPosition = vTip;
        vTop.vUV = { 0.f, t };
        vTop.vColor = { 1.f, 1.f, 1.f, fLifeRatio };

        TRAIL_VERTEX vBottom{};
        vBottom.vPosition = vBase;
        vBottom.vUV = { 1.f, t };
        vBottom.vColor = { 1.f, 1.f, 1.f, fLifeRatio };

        m_vecVertices.push_back(vTop);
        m_vecVertices.push_back(vBottom);
    }
}