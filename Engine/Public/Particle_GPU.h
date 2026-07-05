#pragma once
#include "Engine_Defines.h"
#include "Particle.h"

NS_BEGIN(Engine)

// GPU 파티클: 하나의 concrete 클래스가 모든 GPU 이펙트를 처리한다.
// 이펙트마다 클래스를 새로 만들지 않고, Initialize(pArg)에 DESC를 넘겨서
// 텍스처/behaviorType/최대 파티클 개수만 다르게 주입한다.
// Update/Spawn/Render 파이프라인(Compute Shader 3종 + StructuredBuffer 3개)은
// 모든 이펙트가 공유하며, HLSL 쪽에서 g_iBehaviorType 분기로 실제 움직임을 다르게 처리한다.
class ENGINE_DLL CParticle_GPU final : public CParticle
{
public:
    DECLARE_DERIVED_TYPE(CParticle_GPU, CParticle)

public:
    // Initialize(void* pArg)에 이 구조체의 포인터를 넘긴다.
    // 이펙트별로 달라지는 값은 전부 여기로 뺐다 ? 하드코딩 금지.
    struct DESC
    {
        uint32_t     iMaxParticles = 1000;   
        int32_t      iBehaviorType ;      //  (HLSL 쪽 분기 인덱스)
        PARTICLE_TYPE       type;
        std::pair<StringID, StringID> textureID;  // 파티클 텍스처


    };

private:
    explicit CParticle_GPU();
    virtual ~CParticle_GPU();

public:
    virtual HRESULT Initialize(void* pArg) override;
    virtual void PriorityUpdate(E::_float fTimeDelta) override;
    virtual void Update(E::_float fTimeDelta) override;
    virtual void LateUpdate(E::_float fTimeDelta) override;
    virtual HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;
    virtual HRESULT Spawn(uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData) override;


private:
    DESC m_Desc; // Initialize에서 받은 이펙트별 설정 (behaviorType, 텍스처 등)

private:
    uint32_t m_iNumElements = 0;

    SPtr<class CResStructuredBuffer> m_pParticleStructuredBuffer = nullptr;
    SPtr<CResStructuredBuffer>       m_pDeadListBuffer = nullptr;
    SPtr<CResStructuredBuffer>       m_pSpawnListBuffer = nullptr;

    SPtr<class CResComputeShader>    m_pResSpawnComputeShader = nullptr;
    SPtr<CResComputeShader>          m_pResUpdateComputeShader = nullptr;
    SPtr<CResComputeShader>          m_pResInitDeadCS = nullptr;
    SPtr<class CResSamplerState>     m_pResSamplerState = nullptr;

    SPtr<class CResCBuffer> m_pComCBuffer;
    SPtr<CResCBuffer>       m_pComSpawnCBuffer;


    uint32_t                         m_iCurrentSpawnCount = 0;
};

NS_END