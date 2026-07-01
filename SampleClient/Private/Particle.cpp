#include "pch.h"
#include "Particle.h"
#include "GameInstance.h"
#include "Resources.h"

NS_USING(Client)

CParticle::CParticle()
{
}

CParticle::~CParticle()
{
}

HRESULT CParticle::Initialize(void* pArg)
{
    if (FAILED(CGameObject::Initialize(pArg)))
    {
        return E_FAIL;
    }

    m_iNumElements = 1000;
    m_RenderPassFlags = ETOUI(RENDERPASS::DEFAULT);

    // 1. 계산 셰이더용 텍스쳐/샘플러 및 셰이더 리소스 로드
    // (기존의 VS, PS 외에 Compute Shader인 "CS_DropBlock"을 추가로 로드해둡니다.)

    // 2. StructuredBuffer 생성 (기존 DynamicBuffer 대신 사용)
    // 이 버퍼는 구조체 배열 형태로 GPU 내부에서 직접 읽고 쓰기(UAV)가 가능해야 합니다.

    std::vector<PARTICLE> initParticles(m_iNumElements);

    // 2. 랜덤 장치나 특정 규칙을 이용해 초기 데이터 정의
    for (size_t i = 0; i < m_iNumElements; ++i)
    {
        // 예: 원점에서 분수처럼 사방으로 튀는 파티클 세팅
        initParticles[i].position = _float3(0.f, 0.f, 0.f);

        // 속도는 사방으로 무작위 (-5.0 ~ 5.0)
        initParticles[i].velocity = _float3(
            ((rand() % 100) / 10.f) - 5.f,
            ((rand() % 100) / 10.f) + 5.f, // 위쪽으로 더 잘 튀게
            ((rand() % 100) / 10.f) - 5.f
        );

        float randomLife = ((rand() % 100) / 50.f) + 1.f; // 1.0 ~ 3.0초
        initParticles[i].life = randomLife;
        initParticles[i].maxLife = randomLife;
        initParticles[i].color = _float4(1.f, 0.5f, 0.f, 1.f); // 주황색 불꽃
        initParticles[i].size = 0.5f;
        initParticles[i].alive = true;
        initParticles[i].loop = false;
    }


    if (auto res = CResStructuredBuffer::Create())
    {
        CResStructuredBuffer::DESC Desc{};
        Desc.iNumElements = m_iNumElements;
        Desc.iStructureByteStride = sizeof(PARTICLE);
        Desc.pInitialData = initParticles.data();
        // 팁: Compute Shader가 쓰고(UAV), Vertex Shader가 읽어야(SRV) 하므로 
        // 셰이더 결합 플래그(BindFlags)를 적절히 설정하여 엔진의 Buffer 클래스를 호출해야 합니다.
        if (FAILED(res->Load(Desc)))
        {
            return E_FAIL;
        }
        m_pParticleStructuredBuffer = res;
    }



    {
        CComConstantBuffer::DESC Desc{};
        Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
        if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject", &Desc, &m_pComCBuffer)))
        {
            return E_FAIL;
        };
    }
    //"SAMPLE_CLIENT_TEX", "TEX2D_Terrain_Tile0"
    m_pParticleTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>("SAMPLE_CLIENT_TEX", "TEX2D_Terrain_Tile0");
    //m_pResTerrainTexture2D = CResTexture2D::Create("./Resources/SampleClient/Textures/Terrain/Tile0.dds");
    if (!m_pParticleTexture)
    {
        return E_FAIL;
    }

    m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>("SAMPLE_CLIENT_SHADER", "VS_VTX_PARTICLE_TEX");
    //m_pResVertexShader = CResVertexShader::Create("./Resources/SampleClient/Shader/Shader_VtxNorTex.hlsl");
    if (FAILED(m_pResVertexShader->Load()))
    {
        return E_FAIL;
    }
    m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>("SAMPLE_CLIENT_SHADER", "PS_VTX_PARTICLE_TEX");
    //m_pResPixelShader = CResPixelShader::Create("./Resources/SampleClient/Shader/Shader_VtxNorTex.hlsl");
    if (FAILED(m_pResPixelShader->Load()))
    {
        return E_FAIL;
    }

    m_pResComputeShader = E::CGameInstance::Get().GetResourceFirst<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_Particle");
    //m_pResPixelShader = CResPixelShader::Create("./Resources/SampleClient/Shader/Shader_VtxNorTex.hlsl");
    if (FAILED(m_pResComputeShader->Load()))
    {
        return E_FAIL;
    }

    m_pResSamplerState = CGameInstance::Get().GetResourceFirst<CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
    
    if (!m_pResSamplerState)
    {
        return E_FAIL;
    }
    // 초기 파티클 데이터(위치, 수명 등)를 최초 1회 생성하여 
    // m_pParticleStructuredBuffer에 서브리소스 데이터로 처음 한 번만 채워줍니다.
    // (매 프레임 Map/Unmap을 하지 않기 위함)

    return S_OK;
}

void CParticle::PriorityUpdate(E::_float fTimeDelta)
{
}

void CParticle::Update(E::_float fTimeDelta)
{

    auto pContext = E::CGameInstance::Get().GetGraphicDeviceContext();

    {
        CB_ParticleUpdate cb{};
        cb.g_fTimeDelta = fTimeDelta;
        cb.g_iNumInstances = m_iNumElements;
        cb.g_iBehaviorType = 3; // 예: Blood 또는 Drop 등 번호 지정

        if (SUCCEEDED(m_pComCBuffer->MapDiscard(pContext.Get(), &cb, sizeof(cb))))
        {
            // Compute Shader 연산용이므로 CS에 바인딩합니다.
            pContext->CSSetConstantBuffers(0, 1, m_pComCBuffer->GetAdressOfBuffer());
            pContext->VSSetConstantBuffers(0, 1, m_pComCBuffer->GetAdressOfBuffer());
            pContext->PSSetConstantBuffers(0, 1, m_pComCBuffer->GetAdressOfBuffer());
        }

    }



    // 2. Compute Shader에 쓰기 버퍼(UAV) 바인딩
    ID3D11UnorderedAccessView* pUAV = m_pParticleStructuredBuffer->GetUAV().Get();
    pContext->CSSetUnorderedAccessViews(0, 1, &pUAV, nullptr);

    // 3. Compute Shader 바인딩 및 실행
    pContext->CSSetShader(m_pResComputeShader->GetComputeShader().Get(), nullptr, 0);

    // 스레드 그룹 실행 (스레드 256개당 1그룹)
    uint32_t groupX = (m_iNumElements + 255) / 256;
    pContext->Dispatch(groupX, 1, 1);

    // 4. 연산 완료 후 자원 해제 (파이프라인 꼬임 방지)

    ID3D11Buffer* nullCBuffer[] = { nullptr };
    ID3D11UnorderedAccessView* nullUAV[] = { nullptr };
    pContext->CSSetConstantBuffers(0, 1, nullCBuffer);
    pContext->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
    pContext->CSSetShader(nullptr, nullptr, 0);
}

void CParticle::LateUpdate(E::_float fTimeDelta)
{
    E::CGameInstance::Get().AddRenderObject(E::RENDERGROUP::NONBLEND, this);
}

HRESULT CParticle::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{

    const auto& vs = m_pResVertexShader;
    const auto& ps = m_pResPixelShader;


    pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
    pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

    pContext->IASetInputLayout(nullptr);

    ID3D11ShaderResourceView* pSRV = m_pParticleStructuredBuffer->GetSRV().Get();
    pContext->VSSetShaderResources(0, 1, &pSRV);
    {
        pContext->PSSetShaderResources(0, 1, m_pParticleTexture->GetSRV().GetAddressOf());
    }

    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    const auto& sampler = m_pResSamplerState;
    pContext->PSSetSamplers(0, 1, sampler->GetSamplerState().GetAddressOf());


    pContext->DrawInstanced(4, m_iNumElements, 0, 0);

    // 렌더링 종료 후 리소스 해제
    ID3D11ShaderResourceView* nullSRV[] = { nullptr };
    pContext->VSSetShaderResources(0, 1, nullSRV);

    return S_OK;
}
E::UPtr<CParticle> CParticle::Create()
{
    auto pInstance = E::ToUPtr(new CParticle{});
    if (FAILED(pInstance->InitializePrototype()))
    {
        MSG_BOX("Failed to Created : CParticle");
        return nullptr;
    }
    return  pInstance;
}

E::UPtr<E::CPrototype> CParticle::Clone(void* pArg)
{
    auto	pInstance = E::ToUPtr(new CParticle{ *this });
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CParticle");
        return nullptr;
    }

    return pInstance;
}
