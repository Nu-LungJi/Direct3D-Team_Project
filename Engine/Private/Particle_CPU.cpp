#include "pch.h"
#include "Particle_CPU.h"
#include "GameInstance.h"
#include "Resources.h"

NS_USING(Engine)

CParticle_CPU::CParticle_CPU()
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
    m_pResSamplerState = CGameInstance::Get().GetResourceFirst<CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
    if (!m_pResSamplerState)
        return E_FAIL;

    //m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(pDesc->VSID.first, pDesc->VSID.second);
    //if (FAILED(m_pResVertexShader->Load()))
    //    return E_FAIL;
    //
    //m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(pDesc->PSID.first, pDesc->PSID.second);
    //if (FAILED(m_pResPixelShader->Load()))
    //    return E_FAIL;

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


        m_pComCBuffer = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT);
        if (!m_pComCBuffer)
            return E_FAIL;

        // 모델 인스턴스는 컴포넌트 프로토타입 clone이 필요하다면 아래처럼
        // (AddComponentFromProto 대신, GameObject 없이도 쓸 수 있는 형태로)
        {
    

            CComStaticModelInstance::DESC modelDesc{};
            modelDesc.sGroupTag = m_Desc.sGroupTag;   // 밖에서 주입
            modelDesc.sResTag = m_Desc.sResTag;     // 밖에서 주입


            auto pProto = CGameInstance::Get().ClonePrototype("PERMANENT", "Prototype_Component_StaticModelInstance", &modelDesc);
            if (pProto == nullptr)
                return E_FAIL;
            m_pComModelInstance = UPtr<CComStaticModelInstance>(static_cast<CComStaticModelInstance*>(pProto.release()));

            if (!m_pComModelInstance)
                return E_FAIL;
        }
    }
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
static int a = 0;

void CParticle_CPU::Simulate(E::_float fTimeDelta)
{
    m_vecInstancedData.clear();

    for (auto& p : m_Particles)
    {

        if (!p.bAlive)
            continue;

        a++;
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
        inst.emissive = p.emissive;

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
       // m_Particles[i].vVelocity = src.velocity;
        m_Particles[i].fLifeTime = src.life;
        m_Particles[i].fAge = 0.f;
        m_Particles[i].bAlive = true;
        m_Particles[i].fSize = src.size;
        m_Particles[i].vColor = src.color;
        m_Particles[i].emissive = src.emissive;

        ++iSpawned;
    }

    return (iSpawned == count) ? S_OK : E_FAIL;
}
HRESULT CParticle_CPU::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{

    if (m_vecInstancedData.empty())
        return S_OK;

    if (m_Desc.whatKind == MESHORTEXTURE::MESH)
        return Render_Mesh(pContext, ctx);

    return Render_Texture(pContext, ctx); // 기존 텍스처 파티클 렌더 코드
}


HRESULT CParticle_CPU::Render_Mesh(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{


    if (m_vecInstancedData.empty())
        return S_OK;

    const auto& vs = m_pResVertexShader;
    const auto& ps = m_pResPixelShader;
    pContext->IASetInputLayout(vs->GetInputLayout().Get());
    pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
    pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

    auto pModel = m_pComModelInstance->GetModel();
    uint32_t iNumMeshes = pModel->Get_NumMeshes();




    // 인스턴스 데이터(월드행렬/컬러) 업로드 -- 텍스처 버전과 동일
    {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(pContext->Map(m_pResInstancedBuffer->GetBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            std::memcpy(mapped.pData, m_vecInstancedData.data(),
                sizeof(VTX_PARTICLE_INSTANCED_DATA) * m_vecInstancedData.size());
            pContext->Unmap(m_pResInstancedBuffer->GetBuffer().Get(), 0);
        }
    }
  //  auto& viBuffer0 = pModel->GetMeshes()[1];
    for (uint32_t i = 0; i < iNumMeshes; ++i)
    {
        const auto& viBuffer = pModel->GetMeshes()[i];
        ID3D11Buffer* vertexBuffers[] = {
            viBuffer->GetVertexBuffer().Get(),
            m_pResInstancedBuffer->GetBuffer().Get()  // 슬롯1: 인스턴스별 월드행렬/컬러
        };
        uint32_t strides[] = {
            viBuffer->GetVertexStride(),
            (uint32_t)sizeof(VTX_PARTICLE_INSTANCED_DATA)
        };
        uint32_t offsets[] = { 0, 0 };


        pContext->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);
        pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
        pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

		SPtr<CResTexture2D> DiffuseTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_DIFFUSE");
		if (auto Resource = m_pComModelInstance->Get_MeshTexture(i, AI_TEXTURE_TYPE::aiTextureType_DIFFUSE, 0)) {
			DiffuseTexture = Resource;
		}
		pContext->PSSetShaderResources(0, 1, DiffuseTexture->GetSRV().GetAddressOf());
		SPtr<CResTexture2D> NormalTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_NORMAL");
		if (auto Resource = m_pComModelInstance->Get_MeshTexture(i, AI_TEXTURE_TYPE::aiTextureType_NORMALS, 0)) {
			NormalTexture = Resource;
		}
		pContext->PSSetShaderResources(1, 1, NormalTexture->GetSRV().GetAddressOf());

		SPtr<CResTexture2D> SMROTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_SMRO");
		if (auto Resource = m_pComModelInstance->Get_MeshTexture(i, AI_TEXTURE_TYPE::aiTextureType_METALNESS, 0)) {
			SMROTexture = Resource;
		}
		pContext->PSSetShaderResources(2, 1, SMROTexture->GetSRV().GetAddressOf());

		SPtr<CResTexture2D> EmissiveTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_EMISSIVE");
		if (auto Resource = m_pComModelInstance->Get_MeshTexture(i, AI_TEXTURE_TYPE::aiTextureType_EMISSIVE, 0)) {
			EmissiveTexture = Resource;
		}
		pContext->PSSetShaderResources(3, 1, EmissiveTexture->GetSRV().GetAddressOf());
        //m_pComModelInstance->Bind_Materials(pContext, i, AI_TEXTURE_TYPE::aiTextureType_DIFFUSE, 0);
        //m_pComModelInstance->Bind_Materials(pContext, i, AI_TEXTURE_TYPE::aiTextureType_NORMALS, 0);

        pContext->PSSetSamplers(0, 1, m_pResSamplerState->GetSamplerState().GetAddressOf());

        auto rasterizer = E::CGameInstance::GetConst().GetResourceFirst<E::CResRasterizerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_NOCULL);
        pContext->RSSetState(rasterizer->GetRasterizerState().Get());

        pContext->DrawIndexedInstanced((UINT)viBuffer->GetNumIndices(), (UINT)m_vecInstancedData.size(), 0, 0, 0);
    }


    return S_OK;
}


HRESULT CParticle_CPU::Render_Texture(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
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
