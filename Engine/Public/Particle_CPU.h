#pragma once
#include "Engine_Defines.h"
#include "Particle.h"

NS_BEGIN(Engine)

struct PARTICLE_CPU_DATA
{
    _float3 vPosition;
    _float3 vVelocity;
    _float4 vColor = { 1.f, 1.f, 1.f, 1.f };
    _float  fSize = 1.f;
    _float  fAge = 0.f;
    _float  fLifeTime = 1.f;
    _bool   bAlive = false;
};

struct VTX_PARTICLE_INSTANCED_DATA
{
    _float4x4 matWorld;
    _float4   vColor;
};

// CPU 파티클 중간 추상 클래스.
// 슬롯 관리(Spawn/재활용), 인스턴스 버퍼 업로드, 렌더링은 여기서 공통으로 처리하고,
// "파티클이 실제로 어떻게 움직이는가"만 자식 클래스에게 맡긴다 (UpdateBehavior).
class ENGINE_DLL CParticle_CPU : public CParticle
{
public:
    struct DESC
    {
        uint32_t                     iMaxParticles;
        std::pair<StringID, StringID> viBufferID; // 파티클 쿼드 메쉬 (공유 리소스 조회 키)
        std::pair<StringID, StringID> textureID;  // 파티클 텍스처 (CResTexture2D 하나)
        std::pair<StringID, StringID> VSID;  // 버텍스 쉐이더
        std::pair<StringID, StringID> PSID;  // 픽셀 쉐이더
        PARTICLE_TYPE                  type;
        MESHORTEXTURE                  whatKind;
    };
public:
    DECLARE_DERIVED_TYPE(CParticle_CPU, CParticle)
protected:
    explicit CParticle_CPU();
    CParticle_CPU(const CParticle_CPU& rhs);
    virtual ~CParticle_CPU();
public:
    virtual HRESULT Initialize(void* pArg) override;
    virtual void PriorityUpdate(E::_float fTimeDelta) override;
    virtual void Update(E::_float fTimeDelta) override;
    virtual void LateUpdate(E::_float fTimeDelta) override;
    virtual HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;
    virtual HRESULT Spawn(uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData) override;
protected:
    // 자식 클래스가 반드시 구현해야 하는 실제 움직임 로직.
    // Simulate()가 살아있는 파티클 하나마다 이 함수를 호출해준다.
    virtual void UpdateBehavior(PARTICLE_CPU_DATA& p, E::_float fTimeDelta) = 0;
private:
    // m_Particles를 순회하며 수명/UpdateBehavior 처리 후 m_vecInstancedData 재구성
    void Simulate(E::_float fTimeDelta);
protected:
    DESC     m_Desc;
    uint32_t m_iNumElements = 0;
    std::vector<PARTICLE_CPU_DATA>           m_Particles;       // 슬롯 iMaxParticles개 고정 (재활용)
    std::vector<VTX_PARTICLE_INSTANCED_DATA> m_vecInstancedData; // 이번 프레임 살아있는 것만
    SPtr<class CResDynamicBuffer> m_pResInstancedBuffer;
    std::pair<StringID, StringID>  m_viBufferID;
    // m_pParticleTexture는 부모 CParticle이 CResTexture2D로 공통 소유

};
NS_END