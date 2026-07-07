#pragma once
#include "Engine_Defines.h"
#include "Particle.h"
NS_BEGIN(Engine)
class ENGINE_DLL CBeam_CPU : public CParticle
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
        uint32_t    iDisplacementIterations = 6;      // 기본값 (AddBeam에서 안 넘기면 이걸 씀)
        _float      fDisplacementAmplitude = 2.5f;
        _float      fDisplacementDamping = 0.25f;
        _float      fFlickerInterval = 0.01f;
        uint32_t    iMaxBeams = 16;
        uint32_t    iMaxDisplacementIterations = 10;  // 버퍼 크기 산정용 - 실제 사용 가능한 최댓값
    };

    struct BEAM_INSTANCE
    {
        _bool       bActive = false;
        _float4     vStartPos = {};
        _float4     vEndPos = {};
        _float      fElapsedTime = 0.f;
        _float      fDuration = 0.f;
        uint32_t    iDisplacementIterations = 6;
        _float      fDisplacementAmplitude = 2.5f;
        _float      fDisplacementDamping = 0.25f;
        _float      fFlickerInterval = 0.1f;
        _float      fFlickerTimer = 0.f;
        _float4     vColor = _float4(1, 1, 1, 1);
        _float4     vEmissive = _float4(1, 1, 1, 1);
        // 이 빔만의 세그먼트 정보 (개별적으로 다를 수 있음)
        uint32_t    iSegmentCount = 0;        // = 2^iDisplacementIterations
        uint32_t    iVerticesPerPlane = 0;    // = (iSegmentCount+1) * 2

        std::vector<_float3> vecJaggedPoints;
    };

protected:
    CBeam_CPU();
    virtual ~CBeam_CPU();
public:
    virtual HRESULT Initialize(void* pArg) override;
    virtual void    PriorityUpdate(_float fTimeDelta) override;
    virtual void    Update(_float fTimeDelta) override;
    virtual void    LateUpdate(_float fTimeDelta) override;
    virtual HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;
    virtual HRESULT Spawn(uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData) override;

public:
    int32_t AddBeam(const _float4& vStart, const _float4& vEnd,
        _float fDisplacementAmplitude, uint32_t iDisplacementIterations, _float fDisplacementDamping,
        _float fFlickerInterval, _float4 emissive, _float fDuration = 0.f);
    void    SetBeamActive(uint32_t beamIndex, _bool bActive, _float fDuration = 0.f);
    void    SetStartPos(uint32_t beamIndex, const _float4& vPos);
    void    SetEndPos(uint32_t beamIndex, const _float4& vPos);

private:
    void RegenerateJaggedPath(BEAM_INSTANCE& beam);
    void BuildBeamGeometry();

private:
    DESC        m_Desc;
    std::vector<BEAM_INSTANCE> m_vecBeams;

    std::vector<BEAM_VERTEX> m_vecBeamVertices;
    struct BEAM_DRAW_RANGE
    {
        uint32_t startVertex;
        uint32_t verticesPerPlane;   // 이 빔의 평면 하나당 버텍스 수 (Draw 시 필요)
    };
    std::vector<BEAM_DRAW_RANGE> m_vecDrawRanges;

    SPtr<class CResDynamicBuffer> m_pResVertexBuffer;
};
NS_END
