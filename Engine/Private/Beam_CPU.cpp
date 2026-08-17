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
	m_fAccumulationTime += fTimeDelta;
	_bool bNeedRebuild = false;

	for (auto& beam : m_vecBeams)
	{
		if (!beam.bActive)
			continue;

		beam.fElapsedTime += fTimeDelta;
		const _bool bElectricBundle =
			beam.iGeometryType == static_cast<uint32_t>(
				BEAM_GEOMETRY_TYPE::ELECTRIC_BUNDLE);

		// [LSY] 전기다발은 AddBeam에서 경과 시간을 음수로 시작해 기존 수명 계산으로 지연시킨다.
		if (beam.fElapsedTime < 0.f)
			continue;

		const _float fLifeRatio = std::clamp(
			beam.fElapsedTime / std::max(beam.fDuration, 0.001f),
			0.f,
			1.f);

		if (bElectricBundle)
		{
			// [LSY] 전기다발만 Flicker 주기마다 굴곡을 갱신한다.
			beam.fFlickerTimer -= fTimeDelta;
			if (beam.fFlickerTimer <= 0.f)
			{
				RegenerateJaggedPath(beam);
				beam.fFlickerTimer += std::max(
					beam.fFlickerInterval,
					0.001f);
			}
		}

		// [LSY] 전기다발도 기존 수명 상태를 공유하되 STRAIGHTEN 단계만 건너뛴다.
		if (beam.fElapsedTime < beam.fGrowEndTime)
		{
			beam.ePhase = BEAM_PHASE::GROW;

			float ratio = std::clamp(
				beam.fElapsedTime / std::max(beam.fGrowEndTime, 0.001f),
				0.f, 1.f
			);

			beam.fGrowRatio = 1.f - (1.f - ratio) * (1.f - ratio);
			beam.fStraightRatio = 0.f;
			beam.fFadeRatio = 0.f;
		}
		else if (!bElectricBundle &&
			beam.fElapsedTime < beam.fStraightEndTime)
		{
			beam.ePhase = BEAM_PHASE::STRAIGHTEN;
			beam.fGrowRatio = 1.f;

			float ratio = std::clamp(
				(beam.fElapsedTime - beam.fGrowEndTime) /
				std::max(beam.fStraightEndTime - beam.fGrowEndTime, 0.001f),
				0.f, 1.f
			);

			beam.fStraightRatio = 1.f - (1.f - ratio) * (1.f - ratio) * (1.f - ratio);
			beam.fFadeRatio = 0.f;
		}
		else if (beam.fElapsedTime < beam.fHoldEndTime)
		{
			beam.ePhase = BEAM_PHASE::HOLD;
			beam.fGrowRatio = 1.f;
			beam.fStraightRatio = 1.f;
			if (bElectricBundle)
				beam.fStraightRatio = 0.f;
			beam.fFadeRatio = 0.f;
		}
		else
		{
			beam.ePhase = BEAM_PHASE::FADE;
			beam.fGrowRatio = 1.f;
			beam.fStraightRatio = 1.f;
			if (bElectricBundle)
				beam.fStraightRatio = 0.f;

			float ratio = std::clamp(
				(beam.fElapsedTime - beam.fHoldEndTime) /
				std::max(beam.fDuration - beam.fHoldEndTime, 0.001f),
				0.f, 1.f
			);

			beam.fFadeRatio = ratio;
		}
		if (fLifeRatio >= 1.f)
			beam.bActive = false;

		bNeedRebuild = true;
	}

	if (bNeedRebuild)
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

    pContext->PSSetShaderResources(1, 1, m_pParticleTexture->GetSRV().GetAddressOf());

	if (m_pNormalTexture) {
		pContext->PSSetShaderResources(2, 1, m_pNormalTexture->GetSRV().GetAddressOf());
	}


	if (m_pDistortionTexture)
	{
		ID3D11ShaderResourceView* pDistortionSRV = m_pDistortionTexture->GetSRV().Get();
		pContext->PSSetShaderResources(3, 1, &pDistortionSRV);
	}
	if (m_pNoiseTexture)
	{
		ID3D11ShaderResourceView* pNoiseSRV = m_pNoiseTexture->GetSRV().Get();
		pContext->PSSetShaderResources(4, 1, &pNoiseSRV);
	}
	if (m_pAnyTexture)
	{
		ID3D11ShaderResourceView* pAnySRV = m_pAnyTexture->GetSRV().Get();
		pContext->PSSetShaderResources(5, 1, &pAnySRV);

	}


    // 각 빔은 이제 자기만의 verticesPerPlane을 갖고 있으니, 그 값 기준으로 Draw
	for (const auto& range : m_vecDrawRanges)
	{
		CB_BEAM cb{};
		cb.fAgeRatio = range.fAgeRatio;
		cb.fAccumulationTime = m_fAccumulationTime;
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

    ID3D11ShaderResourceView* nullSRV[] = { nullptr,nullptr,nullptr,nullptr,nullptr,nullptr };
    pContext->PSSetShaderResources(0, 6, nullSRV);
	pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);


	ID3D11Buffer* nullBuffer = nullptr;
	pContext->PSSetConstantBuffers(11, 1, &nullBuffer);

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

	const float distance =XMVectorGetX( XMVector3Length(end - start));
	const float growSpeed = 100.f;
	const float growDuration = distance / growSpeed;
	for (uint32_t i = 0; i < m_vecBeams.size(); ++i)
	{
		if (m_vecBeams[i].bActive)
			continue;

		auto& beam = m_vecBeams[i];
		beam.bActive = true;
		beam.vStartPos = p.beamStart;
		beam.vEndPos = p.beamEnd;
		beam.fElapsedTime = 0.f;
		beam.vColor = p.color;
		beam.vEmissive = p.emissive;
		beam.vEndEmissive = p.endEmissive;
		beam.ownerId = p.ownerId;


		// [LSY] 기존 Beam의 거리 비례 진폭은 유지하고 전기다발 모드만 JSON 진폭을 그대로 사용한다.
		if (p.geometryType == static_cast<int>(
			BEAM_GEOMETRY_TYPE::ELECTRIC_BUNDLE))
		{
			beam.fElapsedTime = -std::max(p.fSpawnDelay, 0.f);
			beam.fDisplacementAmplitude =
				std::max(p.fDisplacementAmplitude, 0.f);
		}
		else
		{
			beam.fDisplacementAmplitude = distance * 0.08f;
		}
		beam.iDisplacementIterations = iterations;
		beam.fDisplacementDamping = std::clamp(p.fDisplacementDamping, 0.f, 1.f);
		beam.fFlickerInterval = std::max(p.flickerTimeInverval, 0.001f);
		beam.fFlickerTimer = beam.fFlickerInterval;
		beam.iSegmentCount = 1u << iterations;
		beam.vecJaggedPoints.assign(beam.iSegmentCount + 1, _float3{});
		beam.ePhase = BEAM_PHASE::GROW;
		beam.fPhaseTime = 0.f;
		beam.fGrowRatio = 0.f;
		beam.fStraightRatio = 0.f;
		beam.fFadeRatio = 0.f;
		beam.fbeamWidth = p.fBeamWidth;
		beam.fStartTaperRatio = std::clamp(p.fStartTaperRatio, 0.f, 1.f);
		beam.fEndTaperRatio = std::clamp(p.fEndTaperRatio, 0.f, 1.f);
		const float straightDuration = std::max(p.fStraightEndTime - p.fGrowEndTime,0.f);

		const float holdDuration = std::max(p.fHoldEndTime - p.fStraightEndTime,0.f);

		const float fadeDuration = std::max(p.beamDuration - p.fHoldEndTime,0.f);

		beam.fGrowEndTime = growDuration;
		beam.fStraightEndTime = beam.fGrowEndTime + straightDuration;
		beam.fHoldEndTime = beam.fStraightEndTime + holdDuration;
		beam.fDuration = beam.fHoldEndTime + fadeDuration;
		beam.fFadeEndTime = beam.fDuration;
		beam.iGeometryType = p.geometryType;
		beam.fspawnDelay = p.fSpawnDelay;
		

		if (beam.iGeometryType == static_cast<uint32_t>(
			BEAM_GEOMETRY_TYPE::SINE))
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
		if (beam.iGeometryType == static_cast<uint32_t>(
			BEAM_GEOMETRY_TYPE::ELECTRIC_BUNDLE))
		{
			beam.fElapsedTime = -std::max(beam.fspawnDelay, 0.f);
		}
		beam.fDuration = fDuration;
		beam.fFlickerTimer = 0.f;

		beam.ePhase = BEAM_PHASE::GROW;
		beam.fPhaseTime = 0.f;
		beam.fGrowRatio = 0.f;
		beam.fStraightRatio = 0.f;
		beam.fFadeRatio = 0.f;

		if (beam.iGeometryType == static_cast<uint32_t>(
			BEAM_GEOMETRY_TYPE::SINE))
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

		if (beam.iGeometryType == static_cast<uint32_t>(
			BEAM_GEOMETRY_TYPE::SINE))
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

	struct VISIBLE_POINT
	{
		XMVECTOR position;
		_float t;
	};

	for (auto& beam : m_vecBeams)
	{
		if (!beam.bActive || beam.iSegmentCount == 0)
			continue;

		float growRatio = std::clamp(beam.fGrowRatio, 0.f, 1.f);
		if (growRatio <= 0.f)
			continue;

		XMVECTOR start = XMLoadFloat4(&beam.vStartPos);
		XMVECTOR end = XMLoadFloat4(&beam.vEndPos);
		XMVECTOR direction = end - start;

		if (XMVectorGetX(XMVector3LengthSq(direction)) < 0.000001f)
			continue;

		XMVECTOR segDir = XMVector3Normalize(direction);

		XMVECTOR worldUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
		if (fabsf(XMVectorGetX(XMVector3Dot(segDir, worldUp))) > 0.99f)
			worldUp = XMVectorSet(1.f, 0.f, 0.f, 0.f);

		XMVECTOR right1 = XMVector3Normalize(
			XMVector3Cross(segDir, worldUp)
		);

		XMVECTOR right2 = XMVector3Normalize(
			XMVector3Cross(segDir, right1)
		);

		float scaledSegment =
			growRatio * static_cast<float>(beam.iSegmentCount);

		uint32_t fullSegmentCount =
			static_cast<uint32_t>(scaledSegment);

		fullSegmentCount = std::min(
			fullSegmentCount,
			beam.iSegmentCount
		);

		float partialRatio =
			scaledSegment - static_cast<float>(fullSegmentCount);

		std::vector<VISIBLE_POINT> visiblePoints;
		visiblePoints.reserve(fullSegmentCount + 2);

		for (uint32_t i = 0; i <= fullSegmentCount; ++i)
		{
			float t =
				static_cast<float>(i) /
				static_cast<float>(beam.iSegmentCount);

			XMVECTOR jaggedPos =
				XMLoadFloat3(&beam.vecJaggedPoints[i]);

			XMVECTOR straightPos =
				XMVectorLerp(start, end, t);

			XMVECTOR finalPos = XMVectorLerp(
				jaggedPos,
				straightPos,
				beam.fStraightRatio
			);

			visiblePoints.push_back({ finalPos,t });
		}

		if (partialRatio > 0.0001f &&
			fullSegmentCount < beam.iSegmentCount)
		{
			uint32_t nextIndex = fullSegmentCount + 1;

			XMVECTOR jaggedA =
				XMLoadFloat3(&beam.vecJaggedPoints[fullSegmentCount]);

			XMVECTOR jaggedB =
				XMLoadFloat3(&beam.vecJaggedPoints[nextIndex]);

			XMVECTOR jaggedPos = XMVectorLerp(
				jaggedA,
				jaggedB,
				partialRatio
			);

			float t = growRatio;
			XMVECTOR straightPos = XMVectorLerp(start, end, t);

			XMVECTOR finalPos = XMVectorLerp(
				jaggedPos,
				straightPos,
				beam.fStraightRatio
			);

			visiblePoints.push_back({ finalPos,t });
		}

		if (visiblePoints.size() < 2)
			continue;

		uint32_t startVertex =
			static_cast<uint32_t>(m_vecBeamVertices.size());

		float fadeAlpha = 1.f - std::clamp(
			beam.fFadeRatio,
			0.f,
			1.f
		);

		const _float headT = std::clamp(beam.fElapsedTime / beam.fGrowEndTime, 0.f, 1.f);
		const float tailPower = 1.f;

		const float tailLength = 0.8f;
		auto SmoothStep = [](_float edge0, _float edge1, _float x)
			{
				if (edge0 == edge1)
					return x < edge0 ? 0.f : 1.f;

				x = std::clamp((x - edge0) / (edge1 - edge0), 0.f, 1.f);
				return x * x * (3.f - 2.f * x);
			};

		auto buildPlane = [&](const XMVECTOR& right)
			{
			

				XMVECTOR halfWidth = right * (beam.fbeamWidth * 0.5f);


				for (const auto& point : visiblePoints)
				{
					const float tailLength = 1.f;
					const float minimumTailAlpha = 0.8f;

					const float distanceBehindHead = std::max(growRatio - point.t, 0.f);
					const float localTailRatio = std::clamp(distanceBehindHead / tailLength, 0.f, 1.f);

					const float tailFade = std::pow(1.f - localTailRatio, 0.25f);
					const float tailAlpha = std::lerp(minimumTailAlpha, 1.f, tailFade);

					// [LSY] Ratio가 명시된 Beam만 양 끝의 폭과 알파를 줄여 평평한 절단면을 없앤다.
					float endTaperScale = 1.f;
					if (beam.fEndTaperRatio > 0.f)
					{
						endTaperScale = SmoothStep(
							0.f,
							beam.fEndTaperRatio,
							distanceBehindHead);
					}

					float startTaperScale = 1.f;
					if (beam.fStartTaperRatio > 0.f)
					{
						startTaperScale = SmoothStep(
							0.f,
							beam.fStartTaperRatio,
							point.t);
					}

					const float endpointTaperScale =
						endTaperScale * startTaperScale;
					const XMVECTOR taperedHalfWidth =
						halfWidth * endpointTaperScale;
					const float finalAlpha =
						fadeAlpha * tailAlpha * endpointTaperScale;

					BEAM_VERTEX top{};
					XMStoreFloat3(&top.vPosition, point.position + taperedHalfWidth);
					top.vUV = { 0.f,localTailRatio };
					top.vColor = beam.vColor;
					top.vColor.w *= finalAlpha;
					top.vEmissive = beam.vEmissive;
					top.vEndEmissive = beam.vEndEmissive;

					BEAM_VERTEX bottom{};
					XMStoreFloat3(&bottom.vPosition, point.position - taperedHalfWidth);
					bottom.vUV = { 1.f,localTailRatio };
					bottom.vColor = beam.vColor;
					bottom.vColor.w *= finalAlpha;
					bottom.vEmissive = beam.vEmissive;
					bottom.vEndEmissive = beam.vEndEmissive;

					m_vecBeamVertices.push_back(top);
					m_vecBeamVertices.push_back(bottom);
				}
			};
		buildPlane(right1);
		buildPlane(right2);

		BEAM_DRAW_RANGE range{};
		range.startVertex = startVertex;
		range.verticesPerPlane =
			static_cast<uint32_t>(visiblePoints.size()) * 2;
		range.fAgeRatio = beam.fFadeRatio;

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
	uint32_t randomCurveNumber = RandInt(2, 5);

	for (uint32_t i = 0; i <= beam.iSegmentCount; ++i)
	{
		float t = (float)i / beam.iSegmentCount;

		XMVECTOR basePos = XMVectorLerp(start, end, t);

		float envelope = sinf(t * XM_PI);

		float wave1 = sinf(t * randomCurveNumber *XM_2PI + phase1);
		float wave2 = cosf(t * randomCurveNumber *XM_2PI + phase2);

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
void CBeam_CPU::SetBeamPositions(
	uint32_t beamIndex,
	const _float4& start,
	const _float4& end)
{
	if (beamIndex >= m_vecBeams.size())
		return;

	auto& beam = m_vecBeams[beamIndex];

	if (!beam.bActive)
		return;

	XMVECTOR startPosition = XMLoadFloat4(&start);
	XMVECTOR endPosition = XMLoadFloat4(&end);

	if (XMVectorGetX(
		XMVector3LengthSq(endPosition - startPosition)
	) < 0.000001f)
		return;

	beam.vStartPos = start;
	beam.vEndPos = end;

	// 아직 곡선이 남아 있을 때만 경로 재계산
	if (beam.fStraightRatio < 1.f)
	{
		if (beam.iGeometryType == static_cast<uint32_t>(
			BEAM_GEOMETRY_TYPE::SINE))
			RegenerateSinPath(beam);
		else
			RegenerateJaggedPath(beam);
	}

	BuildBeamGeometry();
}

void CBeam_CPU::SetBeamPositionsByOwner(uint32_t ownerId,const _float3& start,const _float3& end)
{
	const XMVECTOR startPosition = XMLoadFloat3(&start);
	const XMVECTOR endPosition = XMLoadFloat3(&end);

	if (XMVectorGetX(
		XMVector3LengthSq(endPosition - startPosition)) < 0.000001f)
	{
		return;
	}

	bool changed = false;

	for (BEAM_INSTANCE& beam : m_vecBeams)
	{
		if (!beam.bActive || beam.ownerId != ownerId)
			continue;

		beam.vStartPos = _float4 (start.x, start.y, start.z, 1);
		beam.vEndPos = _float4(end.x, end.y, end.z, 1);

		if (beam.fStraightRatio < 1.f)
		{
			if (beam.iGeometryType == static_cast<uint32_t>(
				BEAM_GEOMETRY_TYPE::SINE))
				RegenerateSinPath(beam);
			else
				RegenerateJaggedPath(beam);
		}

		changed = true;
	}

	if (changed)
		BuildBeamGeometry();
}
