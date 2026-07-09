#include "pch.h"
#include "Beam_CPU.h"
#include "GameInstance.h"
#include "Resources.h"

CBeam_CPU::CBeam_CPU()
{
}

CBeam_CPU::~CBeam_CPU()
{
    m_vecBeams.clear();
    m_vecBeamVertices.clear();
}

HRESULT CBeam_CPU::Initialize(void* pArg)
{
    auto pDesc = static_cast<DESC*>(pArg);
    if (pDesc == nullptr)
        return E_FAIL;

    m_Desc = *pDesc;
    m_eType = pDesc->type;

    // 슬롯만 미리 준비 (세그먼트 정보는 AddBeam 시점에 개별로 채워짐)
    m_vecBeams.resize(m_Desc.iMaxBeams);

    // 버퍼 크기는 "허용 가능한 최대 세그먼트 수" 기준으로 넉넉하게 산정
    uint32_t iMaxSegmentCount = 1u << m_Desc.iMaxDisplacementIterations;
    uint32_t iMaxVerticesPerPlane = (iMaxSegmentCount + 1) * 2;
    uint32_t iMaxVerticesTotal = iMaxVerticesPerPlane * 2 * m_Desc.iMaxBeams;

    if (auto res = CResDynamicBuffer::Create())
    {
        CResDynamicBuffer::DESC bufDesc{};
        bufDesc.desc = {
            .ByteWidth = (uint32_t)sizeof(BEAM_VERTEX) * iMaxVerticesTotal,
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

    m_vecBeamVertices.reserve(iMaxVerticesTotal);

    return S_OK;
}

void CBeam_CPU::PriorityUpdate(_float fTimeDelta)
{
}

void CBeam_CPU::Update(_float fTimeDelta)
{
    _bool bAnyActive = false;
    _bool bNeedRebuild = false;

    for (auto& beam : m_vecBeams)
    {
        if (!beam.bActive)
            continue;

        bAnyActive = true;
        beam.fElapsedTime += fTimeDelta;

        if (beam.fDuration > 0.f && beam.fElapsedTime >= beam.fDuration)
        {
            beam.bActive = false;
            bNeedRebuild = true;
            continue;
        }

        beam.fFlickerTimer += fTimeDelta;
        if (beam.fFlickerTimer >= beam.fFlickerInterval)
        {
            beam.fFlickerTimer = 0.f;
            RegenerateJaggedPath(beam);
            bNeedRebuild = true;
        }
    }

    if (bNeedRebuild || bAnyActive)
        BuildBeamGeometry();
}

void CBeam_CPU::LateUpdate(_float fTimeDelta)
{
}

HRESULT CBeam_CPU::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
    if (m_vecBeamVertices.empty())
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

    // 각 빔은 이제 자기만의 verticesPerPlane을 갖고 있으니, 그 값 기준으로 Draw
    for (auto& range : m_vecDrawRanges)
    {
        pContext->Draw(range.verticesPerPlane, range.startVertex);
        pContext->Draw(range.verticesPerPlane, range.startVertex + range.verticesPerPlane);
    }

    ID3D11ShaderResourceView* nullSRV[] = { nullptr };
    pContext->PSSetShaderResources(0, 1, nullSRV);

    return S_OK;
}

HRESULT CBeam_CPU::Spawn(uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData)
{
    return E_FAIL;
}

int32_t CBeam_CPU::AddBeam(const _float4& vStart, const _float4& vEnd,
    _float fDisplacementAmplitude, uint32_t iDisplacementIterations, _float fDisplacementDamping,
    _float fFlickerInterval, _float4 emissive, _float fDuration)
{
    // 안전장치: 버퍼 크기 산정 기준(iMaxDisplacementIterations)을 넘지 않도록 클램프
    if (iDisplacementIterations > m_Desc.iMaxDisplacementIterations)
        iDisplacementIterations = m_Desc.iMaxDisplacementIterations;

    for (uint32_t i = 0; i < m_vecBeams.size(); ++i)
    {
        if (!m_vecBeams[i].bActive)
        {
            auto& beam = m_vecBeams[i];
            beam.bActive = true;
            beam.vStartPos = vStart;
            beam.vEndPos = vEnd;
            beam.fElapsedTime = 0.f;
            beam.fDuration = fDuration;
            beam.vEmissive = emissive;
            beam.fDisplacementAmplitude = fDisplacementAmplitude;
            beam.iDisplacementIterations = iDisplacementIterations;
            beam.fDisplacementDamping = fDisplacementDamping;
            beam.fFlickerInterval = fFlickerInterval;
            beam.fFlickerTimer = fFlickerInterval;

            // 이 빔만의 세그먼트 개수를 여기서 계산하고, 배열 크기도 그에 맞게 재조정
            beam.iSegmentCount = 1u << beam.iDisplacementIterations;
            beam.iVerticesPerPlane = (beam.iSegmentCount + 1) * 2;
            beam.vecJaggedPoints.assign(beam.iSegmentCount + 1, _float3{});
            
            RegenerateJaggedPath(beam);
            BuildBeamGeometry();
            return (int32_t)i;
        }
    }
    return -1;
}

void CBeam_CPU::SetBeamActive(uint32_t beamIndex, _bool bActive, _float fDuration)
{
    if (beamIndex >= m_vecBeams.size())
        return;

    auto& beam = m_vecBeams[beamIndex];
    beam.bActive = bActive;
    if (bActive)
    {
        beam.fElapsedTime = 0.f;
        beam.fDuration = fDuration;
        beam.fFlickerTimer = beam.fFlickerInterval;
        RegenerateJaggedPath(beam);
    }
    BuildBeamGeometry();
}

void CBeam_CPU::SetStartPos(uint32_t beamIndex, const _float4& vPos)
{
    if (beamIndex < m_vecBeams.size())
        m_vecBeams[beamIndex].vStartPos = vPos;
}

void CBeam_CPU::SetEndPos(uint32_t beamIndex, const _float4& vPos)
{
    if (beamIndex < m_vecBeams.size())
        m_vecBeams[beamIndex].vEndPos = vPos;
}

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

    _float r1 = ((_float)rand() / RAND_MAX) * 2.f - 1.f;
    _float r2 = ((_float)rand() / RAND_MAX) * 2.f - 1.f;
    mid += right1 * (r1 * fAmplitude) + right2 * (r2 * fAmplitude);

    XMStoreFloat3(&points[iMidIdx], mid);

    MidpointDisplace(points, iStartIdx, iMidIdx, right1, right2, fAmplitude * fDamping, fDamping, iDepth - 1);
    MidpointDisplace(points, iMidIdx, iEndIdx, right1, right2, fAmplitude * fDamping, fDamping, iDepth - 1);
}

void CBeam_CPU::RegenerateJaggedPath(BEAM_INSTANCE& beam)
{
    XMVECTOR start = XMLoadFloat4(&beam.vStartPos);
    XMVECTOR end = XMLoadFloat4(&beam.vEndPos);
    XMVECTOR segDir = XMVector3Normalize(end - start);

    XMVECTOR worldUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
    if (fabsf(XMVectorGetX(XMVector3Dot(segDir, worldUp))) > 0.99f)
        worldUp = XMVectorSet(1.f, 0.f, 0.f, 0.f);

    XMVECTOR right1 = XMVector3Normalize(XMVector3Cross(segDir, worldUp));
    XMVECTOR right2 = XMVector3Normalize(XMVector3Cross(segDir, right1));

    XMStoreFloat3(&beam.vecJaggedPoints[0], start);
    XMStoreFloat3(&beam.vecJaggedPoints[beam.iSegmentCount], end);   // 빔 개별 세그먼트 수 사용

    MidpointDisplace(beam.vecJaggedPoints, 0, beam.iSegmentCount,
        right1, right2, beam.fDisplacementAmplitude, beam.fDisplacementDamping,
        beam.iDisplacementIterations);
}

void CBeam_CPU::BuildBeamGeometry()
{
    m_vecBeamVertices.clear();
    m_vecDrawRanges.clear();

    for (auto& beam : m_vecBeams)
    {
        if (!beam.bActive)
            continue;

        uint32_t startVertex = (uint32_t)m_vecBeamVertices.size();

        XMVECTOR segDir = XMVector3Normalize(XMLoadFloat4(&beam.vEndPos) - XMLoadFloat4(&beam.vStartPos));

        XMVECTOR worldUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
        if (fabsf(XMVectorGetX(XMVector3Dot(segDir, worldUp))) > 0.99f)
            worldUp = XMVectorSet(1.f, 0.f, 0.f, 0.f);

        XMVECTOR right1 = XMVector3Normalize(XMVector3Cross(segDir, worldUp));
        XMVECTOR right2 = XMVector3Normalize(XMVector3Cross(segDir, right1));

        auto buildPlane = [&](const XMVECTOR& vRight)
            {
                for (uint32_t i = 0; i <= beam.iSegmentCount; ++i)   // 빔 개별 세그먼트 수 사용
                {
                    _float t = (_float)i / (_float)beam.iSegmentCount;
                    XMVECTOR pos = XMLoadFloat3(&beam.vecJaggedPoints[i]);

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

        BEAM_DRAW_RANGE range{};
        range.startVertex = startVertex;
        range.verticesPerPlane = beam.iVerticesPerPlane;   // 빔 개별 값 저장
        m_vecDrawRanges.push_back(range);
    }
}
