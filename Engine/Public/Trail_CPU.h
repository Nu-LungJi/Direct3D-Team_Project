#pragma once
#include "Engine_Defines.h"
#include "Particle.h"

NS_BEGIN(Engine)

// 한 프레임에 기록된 무기 궤적 한 쌍(칼날 밑동~칼끝) + 그 순간부터 흐른 시간
struct TRAIL_FRAME
{
    _float3 vStart; // 칼날 밑동(손잡이 쪽) 월드 위치
    _float3 vEnd;   // 칼날 끝(칼끝) 월드 위치
    _float  fAge = 0.f;
};

// 트레일 전용 정점 - BEAM_VERTEX와 달리 색상(알파)을 갖고 있어서
// 나이 든 프레임일수록 투명해지는 걸 정점 단위로 표현할 수 있다.
struct TRAIL_VERTEX
{
    _float3 vPosition;
    _float2 vUV;
    _float4 vColor = { 1.f, 1.f, 1.f, 1.f };
};

// 무기 궤적 트레일.
// 매 프레임 (밑동, 칼끝) 두 점을 기록해서, 그 쌍을 다음 프레임의 쌍과 이어붙이는 방식으로
// 검이 실제로 휩쓸고 지나간 면(스윕 서피스)을 그린다.
// 두 점이 이미 폭의 양 끝을 정의해주므로, 빔/단일점 트레일과 달리
// 카메라를 향한 폭 벡터를 따로 계산할 필요가 없다.
class ENGINE_DLL CTrail_CPU  : public CParticle
{
public:
    struct DESC
    {
        std::pair<StringID, StringID> textureID;
        std::pair<StringID, StringID> VSID;
        std::pair<StringID, StringID> PSID;
        PARTICLE_TYPE type;
        TRAIL_TYPE  tType;
        _float   fMaxDuration = 1.25f; // 기록된 프레임 하나가 얼마나 오래 남아있을지 (꼬리 길이)
        uint32_t iMaxFrames = 64;    // 최대 보관 프레임 개수 (버퍼 크기 결정)
    };

protected:
    CTrail_CPU();
    virtual ~CTrail_CPU();

public:
    virtual HRESULT Initialize(void* pArg) override;
    virtual void    PriorityUpdate(_float fTimeDelta) override;
    virtual void    Update(_float fTimeDelta) override;
    virtual void    LateUpdate(_float fTimeDelta) override;
    virtual HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;
    virtual HRESULT Spawn(uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData) override; // 미사용

public:
    // 매 프레임 호출 - 무기 애니메이션 재생 중 칼날 밑동/칼끝의 현재 월드 좌표를 같이 넘긴다.
    void AddPoint(const _float3& vStart, const _float3& vEnd);

    void Clear();
    uint32_t Debug_GetFrameCount() const { return (uint32_t)m_dequeFrames.size(); }
    uint32_t Debug_GetVertexCount() const { return (uint32_t)m_vecVertices.size(); }

private:
    void BuildTrailGeometry();

private:
    DESC     m_Desc;
    TRAIL_TYPE m_eTrailType;
    std::deque<TRAIL_FRAME>  m_dequeFrames; // 앞(front)이 최신, 뒤(back)가 가장 오래된 프레임
    std::vector<TRAIL_VERTEX> m_vecVertices;

    SPtr<class CResDynamicBuffer> m_pResVertexBuffer;
};

NS_END
