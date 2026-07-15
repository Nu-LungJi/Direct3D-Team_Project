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
	m_vColor = _float4(1,1,1,0);
	m_vEmissive = _float4(0, 0, 0, 0);

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

	if (auto res = CResCBuffer::Create())
	{
		CResCBuffer::CBUFFER_DESC bufDesc{};
		bufDesc.byteWidth = sizeof(CB_SCROLL);
		if (FAILED(res->Load(bufDesc)))
			return E_FAIL;
		m_pScrollCBuffer = res;
	}


	m_pNoiseTexture = m_pNoiseTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>("SAMPLE_CLINET_TEXTURE", "TEX_RIBBONNOISE");
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

	m_fTimeSinceLastAdd += fTimeDelta;
	m_fIdleTime += fTimeDelta; // AddPoint에서 0으로 리셋됨 (아래 참고)

	// 멈춘 상태면: 시간 기반 fAge와 무관하게, 꼬리를 강제로 순차 제거
	if (m_fIdleTime >= m_fIdleThreshold && !m_dequeFrames.empty())
	{
		m_fTimeSinceLastRetract += fTimeDelta;
		while (m_fTimeSinceLastRetract >= m_fRetractInterval && !m_dequeFrames.empty())
		{
			m_dequeFrames.pop_back(); // 꼬리(가장 오래된 것)부터 하나씩
			m_fTimeSinceLastRetract -= m_fRetractInterval;
		}
	}
	BuildTrailGeometry();
}

void CTrail_CPU::LateUpdate(_float fTimeDelta)
{
}

_float CTrail_CPU::DistanceSq(const _float3& a, const _float3& b)
{
	float x = a.x - b.x;
	float y = a.y - b.y;
	float z = a.z - b.z;

	return x * x + y * y + z * z;
}
void CTrail_CPU::AddPoint(const _float3& vStart, const _float3& vEnd)
{
	if (m_bHasLastPoint && m_fTimeSinceLastAdd < m_fSampleInterval)
		return;


	if (m_bHasLastPoint)
	{
		_float3 prevCenter = { (m_vLastStart.x + m_vLastEnd.x) * 0.5f,
								(m_vLastStart.y + m_vLastEnd.y) * 0.5f,
								(m_vLastStart.z + m_vLastEnd.z) * 0.5f };
		_float3 currCenter = { (vStart.x + vEnd.x) * 0.5f,
								(vStart.y + vEnd.y) * 0.5f,
								(vStart.z + vEnd.z) * 0.5f };

		XMVECTOR v0 = XMLoadFloat3(&prevCenter);
		XMVECTOR v1 = XMLoadFloat3(&currCenter);
		m_fTotalDistance += XMVectorGetX(XMVector3Length(v1 - v0));
	}
	m_bHasLastPoint = true;
	m_vLastStart = vStart;
	m_vLastEnd = vEnd;
	m_fTimeSinceLastAdd = 0.f;
	m_fIdleTime = 0.f;
	m_fTimeSinceLastRetract = 0.f;


	TRAIL_FRAME frame;
	frame.vStart = vStart;
	frame.vEnd = vEnd;
	frame.fAge = 0.f;
	frame.fDistance = m_fTotalDistance;

	m_dequeFrames.push_front(frame);

	while (m_dequeFrames.size() > m_Desc.iMaxFrames)
		m_dequeFrames.pop_back();
}

void CTrail_CPU::Clear()
{
	m_dequeFrames.clear();
	m_vecVertices.clear();

	m_bHasLastPoint = false;
}

void CTrail_CPU::SetPosition(const _float3& pos)
{
}

void CTrail_CPU::SetVelocity(const _float3& vel)
{
}

void CTrail_CPU::SetSize(const _float& size)
{
}

void CTrail_CPU::SetColor(const _float4& color)
{
	m_vColor.x = color.x;
	m_vColor.y = color.y;
	m_vColor.z = color.z;
}

HRESULT CTrail_CPU::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
    if (m_vecVertices.size() < 4) // 최소 프레임 2개(=4정점)는 있어야 스윕 면이 성립
        return S_OK;


	//초기화 버퍼 초기화
	{
		CB_SCROLL cb{};
		cb.g_fScrollOffset = m_ScrollOffset;

		D3D11_MAPPED_SUBRESOURCE mapped{};
		if (SUCCEEDED(pContext->Map(m_pScrollCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		{
			memcpy(mapped.pData, &cb, sizeof(cb));
			pContext->Unmap(m_pScrollCBuffer->GetCBuffer().Get(), 0);
			pContext->PSSetConstantBuffers(0, 1, m_pScrollCBuffer->GetCBuffer().GetAddressOf());
		}
	}


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
    pContext->PSSetShaderResources(1, 1, m_pNoiseTexture->GetSRV().GetAddressOf());

    pContext->Draw((UINT)m_vecVertices.size(), 0);

    ID3D11ShaderResourceView* nullSRV[] = { nullptr,nullptr };
    pContext->PSSetShaderResources(0, 2, nullSRV);

	ID3D11Buffer* nullCBuffer[] = { nullptr };
	pContext->PSSetConstantBuffers(0, 1, nullCBuffer);

    //pContext->RSSetState(nullptr); // 다음에 그려질 오브젝트에 영향 안 주도록 기본 상태로 복구
	//{	/* --- 광윤 : 다른 RasterizerState 쓰시고 원래 상태로 돌려주시면 됩니다 --- */
	//	const auto& rasterizer = E::CGameInstance::GetConst().GetResourceFirst<E::CResRasterizerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_NOCULL);
	//	pContext->RSSetState(rasterizer->GetRasterizerState().Get());
	//}

    return S_OK;
}

HRESULT CTrail_CPU::Spawn(uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData)
{
    return E_FAIL; // AddPoint(start, end)로 직접 제어 - 이 인터페이스는 안 씀
}

void CTrail_CPU::ClearByOwner(uint32_t ownerID)
{
}

void CTrail_CPU::BuildTrailGeometry()
{
    m_vecVertices.clear();

    uint32_t iCount = (uint32_t)m_dequeFrames.size();
    if (iCount < 2)
        return;


	float fUVTileScale = 0.5f;



	for (uint32_t i = 0; i < iCount; ++i)
	{
		const auto& frame = m_dequeFrames[i];
		float fAgeRatio = frame.fAge / m_Desc.fMaxDuration; // 0(방금 생김) ~ 1(수명 다함)

		// "얼마나 죽었는지"를 이즈 아웃으로 계산 → 초반에 빨리 죽고, 후반은 천천히 남은 채 유지되다 사라짐
		float fDeath = powf(fAgeRatio, 3.f);   // EaseIn cubic (그냥 pow만 써도 됨)
		float fLifeRatio = 1.f - fDeath;
    // 알파: 초반에 빨리 옅어짐
		//float fWidthScale = powf(fLifeRatio, 2.f); // 폭: 거의 안 줄다가 막판에만

		//float fAgeRatio = frame.fAge / m_Desc.fMaxDuration;
		//float fLifeRatio = 1.f - fAgeRatio;
		//fLifeRatio = powf(fLifeRatio, 10.f);
		//float fAgeRatio = frame.fAge / m_Desc.fMaxDuration;
		//float fLifeRatio = 1.f - fAgeRatio;

		float t = frame.fDistance * fUVTileScale;

		// 폭은 시간 기준
		float fWidthScale = fLifeRatio;
		//float fWidthScale = 1.f;
		_float3 vMid =
		{
			(frame.vStart.x + frame.vEnd.x) * 0.5f,
			(frame.vStart.y + frame.vEnd.y) * 0.5f,
			(frame.vStart.z + frame.vEnd.z) * 0.5f
		};

		_float3 vTip =
		{
			vMid.x + (frame.vEnd.x - vMid.x) * fWidthScale,
			vMid.y + (frame.vEnd.y - vMid.y) * fWidthScale,
			vMid.z + (frame.vEnd.z - vMid.z) * fWidthScale
		};

		_float3 vBase =
		{
			vMid.x + (frame.vStart.x - vMid.x) * fWidthScale,
			vMid.y + (frame.vStart.y - vMid.y) * fWidthScale,
			vMid.z + (frame.vStart.z - vMid.z) * fWidthScale
		};

		TRAIL_VERTEX vTop{};
		vTop.vPosition = vTip;
		vTop.vUV = { t, 0.f };
		vTop.vEmissive = m_vEmissive;
		XMStoreFloat4(&vTop.vColor, XMVectorSetW(XMLoadFloat4(&m_vColor), fLifeRatio));


		TRAIL_VERTEX vBottom{};
		vBottom.vPosition = vBase;
		vBottom.vUV = { t, 1.f };
		vBottom.vEmissive = m_vEmissive;
		XMStoreFloat4(&vBottom.vColor, XMVectorSetW(XMLoadFloat4(&m_vColor), fLifeRatio));


		m_vecVertices.push_back(vTop);
		m_vecVertices.push_back(vBottom);
	}

}
UPtr<CParticle> CTrail_CPU::Create(void* pArg)
{
	auto pInstance = E::ToUPtr(new CTrail_CPU{});
	if (FAILED(pInstance->Initialize(pArg)))
	{	
		MSG_BOX("Failed to Created : CTrail_CPU");
		return nullptr;
	}
	return  pInstance;
}
// 1. Quad Ease-Out (2차) - 가장 흔하고 무난함
float CTrail_CPU::EaseOutQuad(float x)
{
	return 1.f - (1.f - x) * (1.f - x);
}

// 2. Cubic Ease-Out (3차) - 더 급격한 초반 변화
float CTrail_CPU::EaseOutCubic(float x)
{
	float t = 1.f - x;
	return 1.f - t * t * t;
}

// 3. 범용 (지수를 파라미터로) - Cubic까지 아래처럼 일반화 가능
float CTrail_CPU::EaseOutPow(float x, float n)
{
	return 1.f - pow(1.f - x, n); // n=2면 Quad, n=3이면 Cubic
}

// 4. Expo Ease-Out - 아주 극적인 초반 변화, 끝에서 아주 천천히 수렴
float CTrail_CPU::EaseOutExpo(float x)
{
	return (x >= 1.f) ? 1.f : 1.f - exp2(-10.f * x);
}

// 5. Sine Ease-Out - 부드럽고 완만함
float CTrail_CPU::EaseOutSine(float x)
{
	return sin(x * 3.14159265f * 0.5f);
}
