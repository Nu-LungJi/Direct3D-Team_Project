#include "pch.h"
#include "Beam_CPU.h"
#include "GameInstance.h"
#include "Resources.h"

CBeam_CPU::CBeam_CPU()
{
}

CBeam_CPU::CBeam_CPU(const CBeam_CPU& rhs)
    : CParticle(rhs)
    , m_Desc{ rhs.m_Desc }
{
}

CBeam_CPU::~CBeam_CPU()
{
}

HRESULT CBeam_CPU::Initialize(void* pArg)
{
    auto pDesc = static_cast<DESC*>(pArg);
    if (pDesc == nullptr)
        return E_FAIL;

    m_Desc = *pDesc;
    m_eType = pDesc->type;

    m_iSegmentCount = 1u << m_Desc.iDisplacementIterations; // 2^iterations
    m_iVerticesPerPlane = (m_iSegmentCount + 1) * 2;
    uint32_t iMaxVertices = m_iVerticesPerPlane * 2; // 십자 평면 두 장

    if (auto res = CResDynamicBuffer::Create())
    {
        CResDynamicBuffer::DESC bufDesc{};
        bufDesc.desc = {
            .ByteWidth = (uint32_t)sizeof(BEAM_VERTEX) * iMaxVertices,
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

    m_vecJaggedPoints.resize(m_iSegmentCount + 1);
    m_vecBeamVertices.reserve(iMaxVertices);

    return S_OK;
}

void CBeam_CPU::PriorityUpdate(_float fTimeDelta)
{
}

void CBeam_CPU::Update(_float fTimeDelta)
{
    if (!m_bActive)
        return;

    m_fElapsedTime += fTimeDelta;

    if (m_fDuration > 0.f && m_fElapsedTime >= m_fDuration)
    {
        SetBeamActive(false);
        return;
    }

    // 지그재그 모양은 매 프레임이 아니라 fFlickerInterval마다만 다시 뽑는다
    // (매 프레임 다시 뽑으면 너무 어지럽게 흔들리고, 실제 번개도 짧은 간격으로 "스냅"하듯 바뀐다)
    m_fFlickerTimer += fTimeDelta;
    if (m_fFlickerTimer >= m_Desc.fFlickerInterval)
    {
        m_fFlickerTimer = 0.f;
        //RegenerateJaggedPath();
        BuildBeamGeometry();
    }
}

void CBeam_CPU::LateUpdate(_float fTimeDelta)
{
}

HRESULT CBeam_CPU::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
    if (!m_bActive || m_vecBeamVertices.empty())
        return S_OK;

    pContext->IASetInputLayout(m_pResVertexShader->GetInputLayout().Get());
    pContext->VSSetShader(m_pResVertexShader->GetVertexShader().Get(), nullptr, 0);
    pContext->PSSetShader(m_pResPixelShader->GetPixelShader().Get(), nullptr, 0);

    {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(pContext->Map(m_pResVertexBuffer->GetBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            std::memcpy(mapped.pData, m_vecBeamVertices.data(),
                sizeof(BEAM_VERTEX) * m_vecBeamVertices.size());
            pContext->Unmap(m_pResVertexBuffer->GetBuffer().Get(), 0);
        }
    }

    ID3D11Buffer* vertexBuffers[] = { m_pResVertexBuffer->GetBuffer().Get() };
    uint32_t strides[] = { sizeof(BEAM_VERTEX) };
    uint32_t offsets[] = { 0 };

    pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    pContext->PSSetShaderResources(0, 1, m_pParticleTexture->GetSRV().GetAddressOf());

    {
        const auto& sampler = CGameInstance::GetConst().GetResourceFirst<CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
        pContext->PSSetSamplers(0, 1, sampler->GetSamplerState().GetAddressOf());
    }

    pContext->Draw(m_iVerticesPerPlane, 0);
    pContext->Draw(m_iVerticesPerPlane, m_iVerticesPerPlane);

    ID3D11ShaderResourceView* nullSRV[] = { nullptr };
    pContext->PSSetShaderResources(0, 1, nullSRV);

    return S_OK;
}

HRESULT CBeam_CPU::Spawn(uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData)
{
    return E_FAIL; // SetStartPos/SetEndPos/SetBeamActive로 직접 제어
}

void CBeam_CPU::SetBeamActive(_bool bActive, _float fDuration)
{
    m_bActive = bActive;
    if (bActive)
    {
        m_fElapsedTime = 0.f;
        m_fDuration = fDuration;
        m_fFlickerTimer = m_Desc.fFlickerInterval; // 즉시 첫 지그재그를 뽑도록

        RegenerateJaggedPath();
        BuildBeamGeometry();
    }
}

// 시작~끝 사이를 재귀적으로 세분화하며, 매번 중점을 right1/right2 평면 위에서 랜덤하게 튕긴다.
// depth가 커질수록(재귀가 깊어질수록) fAmplitude가 fDisplacementDamping배씩 줄어들어
// 큰 줄기는 크게 꺾이고 세부는 미세하게 들쭉날쭉한 프랙탈 모양이 나온다.
static void MidpointDisplace(std::vector<_float3>& points, uint32_t iStartIdx, uint32_t iEndIdx,
    const XMVECTOR& right1, const XMVECTOR& right2,
    _float fAmplitude, _float fDamping, uint32_t iDepth)
{
    if (iDepth == 0 || iEndIdx - iStartIdx < 2)
        return;

    uint32_t iMidIdx = (iStartIdx + iEndIdx) / 2;

    XMVECTOR a = XMLoadFloat3(&points[iStartIdx]);
    XMVECTOR b = XMLoadFloat3(&points[iEndIdx]);
    XMVECTOR mid = (a + b) * 0.5f;

    _float r1 = ((_float)rand() / RAND_MAX) * 2.f - 1.f; // -1~1
    _float r2 = ((_float)rand() / RAND_MAX) * 2.f - 1.f;
    mid += right1 * (r1 * fAmplitude) + right2 * (r2 * fAmplitude);

    XMStoreFloat3(&points[iMidIdx], mid);

    MidpointDisplace(points, iStartIdx, iMidIdx, right1, right2, fAmplitude * fDamping, fDamping, iDepth - 1);
    MidpointDisplace(points, iMidIdx, iEndIdx, right1, right2, fAmplitude * fDamping, fDamping, iDepth - 1);
}

void CBeam_CPU::RegenerateJaggedPath()
{
    XMVECTOR start = XMLoadFloat4(&m_vStartPos);
    XMVECTOR end = XMLoadFloat4(&m_vEndPos);
    XMVECTOR segDir = XMVector3Normalize(end - start);

    XMVECTOR worldUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
    if (fabsf(XMVectorGetX(XMVector3Dot(segDir, worldUp))) > 0.99f)
        worldUp = XMVectorSet(1.f, 0.f, 0.f, 0.f);

    XMVECTOR right1 = XMVector3Normalize(XMVector3Cross(segDir, worldUp));
    XMVECTOR right2 = XMVector3Normalize(XMVector3Cross(segDir, right1));

    XMStoreFloat3(&m_vecJaggedPoints[0], start);
    XMStoreFloat3(&m_vecJaggedPoints[m_iSegmentCount], end);

    MidpointDisplace(m_vecJaggedPoints, 0, m_iSegmentCount,
        right1, right2, m_Desc.fDisplacementAmplitude, m_Desc.fDisplacementDamping,
        m_Desc.iDisplacementIterations);
}

void CBeam_CPU::BuildBeamGeometry()
{
    m_vecBeamVertices.clear();

    XMVECTOR segDir = XMVector3Normalize(XMLoadFloat4(&m_vEndPos) - XMLoadFloat4(&m_vStartPos));

    XMVECTOR worldUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
    if (fabsf(XMVectorGetX(XMVector3Dot(segDir, worldUp))) > 0.99f)
        worldUp = XMVectorSet(1.f, 0.f, 0.f, 0.f);

    // 폭 방향은 전체 빔의 대략적인 진행 방향 기준 고정 축 - 지그재그로 꺾여도 폭 계산은 단순하게 유지
    XMVECTOR right1 = XMVector3Normalize(XMVector3Cross(segDir, worldUp));
    XMVECTOR right2 = XMVector3Normalize(XMVector3Cross(segDir, right1));

    auto buildPlane = [&](const XMVECTOR& vRight)
        {
            for (uint32_t i = 0; i <= m_iSegmentCount; ++i)
            {
                _float t = (_float)i / (_float)m_iSegmentCount;
                XMVECTOR pos = XMLoadFloat3(&m_vecJaggedPoints[i]);

                XMVECTOR halfWidth = vRight * (m_Desc.fWidth * 0.5f);
                XMVECTOR top = pos + halfWidth;
                XMVECTOR bottom = pos - halfWidth;

                BEAM_VERTEX vTop{};
                XMStoreFloat3(&vTop.vPosition, top);
                vTop.vUV = { 0.f, t };

                BEAM_VERTEX vBottom{};
                XMStoreFloat3(&vBottom.vPosition, bottom);
                vBottom.vUV = { 1.f, t };

                m_vecBeamVertices.push_back(vTop);
                m_vecBeamVertices.push_back(vBottom);
            }
        };

    buildPlane(right1);
    buildPlane(right2);
}