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

	m_pParticleShaderCache = pDesc->pShaderCache;

	if (!m_pParticleShaderCache)
		return E_FAIL;

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
	if (auto res = CResCBuffer::Create())
	{
		CResCBuffer::CBUFFER_DESC bufDesc{};
		bufDesc.byteWidth = sizeof(CB_BEAM);
		if (FAILED(res->Load(bufDesc)))
			return E_FAIL;
		m_pComBeamCBuffer = res;
	}

	switch (m_Desc.blendState) {
	case 0:
		m_pBlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_EFFECT");
		break;
	case 1:
		m_pBlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_ADDITIVE");
		break;
	case 2:
		m_pBlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_BLEND_NONE");
		break;
	default:
		m_pBlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_EFFECT");
		break;
	}


	{
		m_pResVertexShader = m_pParticleShaderCache->GetVertexShader(pDesc->VSID.first, pDesc->VSID.second, m_Desc.sVEntryPoint);
		if (!m_pResVertexShader)
			return E_FAIL;
		m_pResPixelShader = m_pParticleShaderCache->GetPixelShader(pDesc->PSID.first, pDesc->PSID.second, m_Desc.sPEntryPoint);
		if (!m_pResPixelShader)
			return E_FAIL;
	}

	if (m_Desc.normalTextureID.first != "") {
		m_pNormalTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(m_Desc.normalTextureID.first, m_Desc.normalTextureID.second);
	}
	if (m_Desc.distortionTextureID.first != "") {
		m_pDistortionTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(m_Desc.distortionTextureID.first, m_Desc.distortionTextureID.second);
	}
	if (m_Desc.noiseTextureID.first != "") {
		m_pNoiseTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(m_Desc.noiseTextureID.first, m_Desc.noiseTextureID.second);
	}
	if (m_Desc.anyTextureID.second != "") {
		m_pAnyTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(m_Desc.anyTextureID.first, m_Desc.anyTextureID.second);
	}
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

			if (m_Desc.geometryType == 1)
				RegenerateSinPath(beam);
			else
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

	auto Rasterizer = E::CGameInstance::GetConst().GetResourceFirst<E::CResRasterizerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_NOCULL);
	pContext->RSSetState(Rasterizer->GetRasterizerState().Get());
	pContext->OMSetBlendState(m_pBlendState->GetBlendState().Get(), nullptr, 0xffffffff);

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
	for (const auto& range : m_vecDrawRanges)
	{
		CB_BEAM cb{};
		cb.fAgeRatio = range.fAgeRatio;

		D3D11_MAPPED_SUBRESOURCE mapped{};
		if (SUCCEEDED(pContext->Map(m_pComBeamCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		{
			memcpy(mapped.pData, &cb, sizeof(cb));
			pContext->Unmap(m_pComBeamCBuffer->GetCBuffer().Get(), 0);
		}

		pContext->PSSetConstantBuffers(11, 1, m_pComBeamCBuffer->GetCBuffer().GetAddressOf());

		pContext->Draw(range.verticesPerPlane, range.startVertex);
		pContext->Draw(range.verticesPerPlane, range.startVertex + range.verticesPerPlane);
	}

    ID3D11ShaderResourceView* nullSRV[] = { nullptr };
    pContext->PSSetShaderResources(0, 1, nullSRV);
	pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);

	pContext->PSSetConstantBuffers(11, 1, nullptr);

    return S_OK;
}

HRESULT CBeam_CPU::Spawn(uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData)
{
    return E_FAIL;
}

int32_t CBeam_CPU::AddBeam(const BEAM_PARAMS& p)
{
	XMVECTOR start = XMLoadFloat4(&p.beamStart);
	XMVECTOR end = XMLoadFloat4(&p.beamEnd);

	if (XMVectorGetX(XMVector3LengthSq(end - start)) < 0.000001f)
		return -1;

	uint32_t iterations = std::clamp(
		static_cast<uint32_t>(std::max(p.iDisplacementIterations, 1)),
		1u,
		m_Desc.iMaxDisplacementIterations);

	for (uint32_t i = 0; i < m_vecBeams.size(); ++i)
	{
		if (m_vecBeams[i].bActive)
			continue;

		auto& beam = m_vecBeams[i];
		beam.bActive = true;
		beam.vStartPos = p.beamStart;
		beam.vEndPos = p.beamEnd;
		beam.fElapsedTime = 0.f;
		beam.fDuration = std::max(p.beamDuration, 0.f);
		beam.vColor = p.color;
		beam.vEmissive = p.emissive;
		beam.ownerId = p.ownerId;
		beam.fDisplacementAmplitude = std::max(p.fDisplacementAmplitude, 0.f);
		beam.iDisplacementIterations = iterations;
		beam.fDisplacementDamping = std::clamp(p.fDisplacementDamping, 0.f, 1.f);
		beam.fFlickerInterval = std::max(p.flickerTimeInverval, 0.001f);
		beam.fFlickerTimer = beam.fFlickerInterval;
		beam.iSegmentCount = 1u << iterations;
		beam.iVerticesPerPlane = (beam.iSegmentCount + 1) * 2;
		beam.vecJaggedPoints.assign(beam.iSegmentCount + 1, _float3{});

		if (m_Desc.geometryType == 1)
			RegenerateSinPath(beam);
		else
			RegenerateJaggedPath(beam);

		BuildBeamGeometry();
		return static_cast<int32_t>(i);
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

		if (m_Desc.geometryType == 1)
			RegenerateSinPath(beam);
		else
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

void CBeam_CPU::TranslateOwner(uint32_t ownerId, const _float3& delta)
{
}

void CBeam_CPU::TransformOwner(uint32_t ownerId, const _float4x4& deltaMatrixData)
{
	XMMATRIX deltaMatrix = XMLoadFloat4x4(&deltaMatrixData);

	for (auto& beam : m_vecBeams)
	{
		if (!beam.bActive || beam.ownerId != ownerId)
			continue;

		XMStoreFloat4(
			&beam.vStartPos,
			XMVector3TransformCoord(
				XMLoadFloat4(&beam.vStartPos),
				deltaMatrix));

		XMStoreFloat4(
			&beam.vEndPos,
			XMVector3TransformCoord(
				XMLoadFloat4(&beam.vEndPos),
				deltaMatrix));

		if (m_Desc.geometryType == 1)
			RegenerateSinPath(beam);
		else
			RegenerateJaggedPath(beam);
	}

	BuildBeamGeometry();
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
					vTop.vColor = beam.vColor;
					vTop.vEmissive = beam.vEmissive;

                    BEAM_VERTEX vBottom{};
                    XMStoreFloat3(&vBottom.vPosition, bottom);
                    vBottom.vUV = { 1.f, t };
					vBottom.vColor = beam.vColor;
					vBottom.vEmissive = beam.vEmissive;
                    m_vecBeamVertices.push_back(vTop);
                    m_vecBeamVertices.push_back(vBottom);
                }
            };

        buildPlane(right1);
        buildPlane(right2);

		BEAM_DRAW_RANGE range{};
		range.startVertex = startVertex;
		range.verticesPerPlane = beam.iVerticesPerPlane;
		range.fAgeRatio = beam.fDuration > 0.f ? std::clamp(beam.fElapsedTime / beam.fDuration, 0.f, 1.f) : 0.f;
		m_vecDrawRanges.push_back(range);
    }
}

void CBeam_CPU::RegenerateSinPath(BEAM_INSTANCE& beam)
{
	XMVECTOR start = XMLoadFloat4(&beam.vStartPos);
	XMVECTOR end = XMLoadFloat4(&beam.vEndPos);

	XMVECTOR segDir = XMVector3Normalize(end - start);

	XMVECTOR worldUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	if (fabsf(XMVectorGetX(XMVector3Dot(segDir, worldUp))) > 0.99f)
		worldUp = XMVectorSet(1.f, 0.f, 0.f, 0.f);

	XMVECTOR right1 = XMVector3Normalize(XMVector3Cross(segDir, worldUp));
	XMVECTOR right2 = XMVector3Normalize(XMVector3Cross(segDir, right1));

	// 빔마다 한 번만 랜덤으로 정하면 더 좋음
	float phase1 = Randf(0.f, XM_2PI);
	float phase2 = Randf(0.f, XM_2PI);

	for (uint32_t i = 0; i <= beam.iSegmentCount; ++i)
	{
		float t = (float)i / beam.iSegmentCount;

		XMVECTOR basePos = XMVectorLerp(start, end, t);

		float envelope = sinf(t * XM_PI);

		float wave1 = sinf(t * XM_2PI + phase1);
		float wave2 = cosf(t * XM_2PI + phase2);

		XMVECTOR offset =
			right1 * (wave1 * beam.fDisplacementAmplitude * envelope) +
			right2 * (wave2 * beam.fDisplacementAmplitude * envelope);

		XMStoreFloat3(&beam.vecJaggedPoints[i], basePos + offset);
	}
}

//void CBeam_CPU::RegenerateSinPath(BEAM_INSTANCE& beam)
//{
//	XMVECTOR start = XMLoadFloat4(&beam.vStartPos);
//	XMVECTOR end = XMLoadFloat4(&beam.vEndPos);
//	XMVECTOR segDir = XMVector3Normalize(end - start);
//
//	XMVECTOR worldUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
//	if (fabsf(XMVectorGetX(XMVector3Dot(segDir, worldUp))) > 0.99f)
//		worldUp = XMVectorSet(1.f, 0.f, 0.f, 0.f);
//
//	XMVECTOR right1 = XMVector3Normalize(XMVector3Cross(segDir, worldUp));
//	XMVECTOR right2 = XMVector3Normalize(XMVector3Cross(segDir, right1));
//	float fLength = XMVectorGetX(XMVector3Length(end - start));
//
//	for (uint32_t i = 0; i <= beam.iSegmentCount; ++i)
//	{
//		float t = (_float)i / (_float)beam.iSegmentCount; // 0~1
//
//		// 직선 경로 위의 기준점
//		XMVECTOR basePos = XMVectorLerp(start, end, t);
//
//		// 사인파: t(길이 방향)에 파수(주파수)를 곱하고, 시간에 따라 위상(phase)을 흘려서 출렁이게 함
//		float fWave = sinf(t * beam.fWaveFrequency * XM_2PI + fTimeAccum * beam.fWaveSpeed);
//
//		// 시작/끝점은 흔들리지 않도록 envelope 처리 (양 끝에서 0에 수렴)
//		float fEnvelope = sinf(t * XM_PI); // t=0,1일 때 0, t=0.5일 때 1
//
//		XMVECTOR offset = right1 * (fWave * beam.fDisplacementAmplitude * fEnvelope);
//
//		XMVECTOR finalPos = basePos + offset;
//		XMStoreFloat3(&beam.vecJaggedPoints[i], finalPos);
//	}
//}
UPtr<CParticle> CBeam_CPU::Create(void* pArg)
{
	auto pInstance = E::ToUPtr(new CBeam_CPU{});
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Created : CBeam_CPU");
		return nullptr;
	}
	return  pInstance;
}
void CBeam_CPU::ClearByOwner(uint32_t ownerId)
{
	for (auto& beam : m_vecBeams)
	{
		if (beam.bActive && beam.ownerId == ownerId)
			beam.bActive = false;
	}

	BuildBeamGeometry();
}
void CBeam_CPU::SetBeamPositions(uint32_t beamIndex, const _float4& start, const _float4& end)
{
	if (beamIndex >= m_vecBeams.size())
		return;

	auto& beam = m_vecBeams[beamIndex];

	if (!beam.bActive)
		return;

	XMVECTOR startPosition = XMLoadFloat4(&start);
	XMVECTOR endPosition = XMLoadFloat4(&end);

	if (XMVectorGetX(XMVector3LengthSq(endPosition - startPosition)) < 0.000001f)
		return;

	beam.vStartPos = start;
	beam.vEndPos = end;

	if (m_Desc.geometryType == 1)
		RegenerateSinPath(beam);
	else
		RegenerateJaggedPath(beam);

	BuildBeamGeometry();
}
