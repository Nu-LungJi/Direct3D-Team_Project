#pragma once
#include "Engine_Defines.h"
#include "Particle.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBeam_CPU  : public CParticle
{
public:
    struct DESC
    {
        std::pair<StringID, StringID> textureID;
        std::pair<StringID, StringID> VSID;
        std::pair<StringID, StringID> PSID;
        PARTICLE_TYPE type;

        _float      fWidth = 1.f;
        _float      fScrollSpeed = 1.f;

        // 번개 지그재그(중점 변위) 관련 - 사인파 파라미터를 대체
        uint32_t    iDisplacementIterations = 5;     // 재귀 횟수. 세그먼트 개수 = 2^iterations
        _float      fDisplacementAmplitude = 1.5f;  // 첫 반복에서의 최대 좌우 편차
        _float      fDisplacementDamping = 0.5f;  // 반복마다 편차가 줄어드는 비율
        _float      fFlickerInterval = 0.05f; // 이 주기마다 지그재그 모양을 새로 뽑음 (깜빡임 느낌)
    };

protected:
    CBeam_CPU();
    CBeam_CPU(const CBeam_CPU& rhs);
    virtual ~CBeam_CPU();

public:
    virtual HRESULT Initialize(void* pArg) override;
    virtual void    PriorityUpdate(_float fTimeDelta) override;
    virtual void    Update(_float fTimeDelta) override;
    virtual void    LateUpdate(_float fTimeDelta) override;
    virtual HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;
    virtual HRESULT Spawn(uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData) override;

public:
    void SetBeamActive(_bool bActive, _float fDuration = 0.f);
    void SetStartPos(const _float4& vPos) { m_vStartPos = vPos; }
    void SetEndPos(const _float4& vPos) { m_vEndPos = vPos; }

private:
    void RegenerateJaggedPath();  // 중점 변위로 지그재그 중심선을 새로 뽑음 (깜빡일 때마다 호출)
    void BuildBeamGeometry();     // m_vecJaggedPoints를 바탕으로 리본 정점 생성

private:
    DESC        m_Desc;
    _bool       m_bActive = false;
    _float4     m_vStartPos = {};
    _float4     m_vEndPos = {};
    _float      m_fElapsedTime = 0.f;
    _float      m_fDuration = 0.f;
    _float      m_fFlickerTimer = 0.f;

    std::vector<_float3>     m_vecJaggedPoints;   // 중점 변위로 만든 지그재그 중심선
    std::vector<BEAM_VERTEX> m_vecBeamVertices;
    uint32_t    m_iSegmentCount = 0;      // = 2^iDisplacementIterations
    uint32_t    m_iVerticesPerPlane = 0;

    SPtr<class CResDynamicBuffer> m_pResVertexBuffer;
};

NS_END