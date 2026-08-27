#pragma once
#include "Engine_Defines.h"
#include "Particle.h"
#include "Handle.h"

NS_BEGIN(Engine)


// 무기 궤적 트레일.
// 매 프레임 (밑동, 칼끝) 두 점을 기록해서, 그 쌍을 다음 프레임의 쌍과 이어붙이는 방식으로
// 검이 실제로 휩쓸고 지나간 면(스윕 서피스)을 그린다.
// 두 점이 이미 폭의 양 끝을 정의해주므로, 빔/단일점 트레일과 달리
// 카메라를 향한 폭 벡터를 따로 계산할 필요가 없다.
class ENGINE_DLL CTrail_CPU final : public CParticle
{
public:
	DECLARE_DERIVED_TYPE(CTrail_CPU, CParticle)

	// 한 프레임에 기록된 무기 궤적 한 쌍(칼날 밑동~칼끝) + 그 순간부터 흐른 시간
	struct TRAIL_FRAME
	{
		_float3 vStart; // 칼날 밑동(손잡이 쪽) 월드 위치
		_float3 vEnd;   // 칼날 끝(칼끝) 월드 위치
		_float3 vWidthDir;
		_float  fAge = 0.f;
		_float  fDistance = 0.f;
	};



	// 트레일 전용 정점 - BEAM_VERTEX와 달리 색상(알파)을 갖고 있어서
	// 나이 든 프레임일수록 투명해지는 걸 정점 단위로 표현할 수 있다.
	struct TRAIL_VERTEX
	{
		_float3 vPosition;
		_float2 vUV;
		_float4 vColor;
		_float4 vEmissive;
		_float  fAgeRatio;
	};


	enum class TRAIL_ALIGN_MODE
	{
		LOCAL,  // 검 궤적: 실제 vStart/vEnd 방향 그대로
		VIEW    // 마법 리본: 카메라 빌보드
	};

	enum class TRAIL_BEHAVIOR_MODE
	{
		LEGACY,
		STABILIZED
	};

    struct DESC
    {
        std::pair<StringID, StringID> textureID;
        std::pair<StringID, StringID> VSID;
        std::pair<StringID, StringID> PSID;
		std::pair<StringID, StringID> normalTextureID;
		std::pair<StringID, StringID> distortionTextureID;
		std::pair<StringID, StringID> noiseTextureID;
		std::pair<StringID, StringID> anyTextureID;
        TRAIL_TYPE  tType = TRAIL_TYPE::END;
        _float   fMaxDuration = 1.f; // 기록된 프레임 하나가 얼마나 오래 남아있을지 (꼬리 길이)
        uint32_t iMaxFrames = 700;    // 최대 보관 프레임 개수 (버퍼 크기 결정)
		TRAIL_ALIGN_MODE eAlignMode = TRAIL_ALIGN_MODE::VIEW;
		_string sVEntryPoint = "";
		_string sPEntryPoint = "";
		uint32_t blendState = 0;
		SPtr<CParticleShaderCache> pShaderCache;
		uint32_t TexRows = 1;
		uint32_t TexColumns = 1;
		_bool bShrinkWidth = true;
		// [LSY] true이면 입력이 멈춘 트레일의 꼬리를 기존 방식대로 일정 간격마다 제거한다.
		// false이면 강제 제거하지 않고 각 포인트가 fMaxDuration 동안 자연스럽게 페이드되도록 둔다.
		_bool bIdleRetractEnabled = true;
		TRAIL_BEHAVIOR_MODE eBehaviorMode = TRAIL_BEHAVIOR_MODE::STABILIZED;


    };

private:
    CTrail_CPU();
    virtual ~CTrail_CPU();

public:
    virtual HRESULT Initialize(void* pArg) override;
    virtual void    PriorityUpdate(_float fTimeDelta) override;
    virtual void    Update(_float fTimeDelta) override;
    virtual void    LateUpdate(_float fTimeDelta) override;
    virtual HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;
    virtual HRESULT Spawn(uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData) override; // 미사용
	virtual void ClearByOwner(uint32_t ownerID) override;

public:
	_float DistanceSq(const _float3& a, const _float3& b);
	// 매 프레임 호출 - 무기 애니메이션 재생 중 칼날 밑동/칼끝의 현재 월드 좌표를 같이 넘긴다.
    void AddPoint(const _float3& vStart, const _float3& vEnd);
    void AddPoint(const CHandle& hOwner, const _float3& vStart, const _float3& vEnd);

    void Clear();
	void Clear(const CHandle& hOwner);
	void SetBehaviorMode(TRAIL_BEHAVIOR_MODE eMode);
    uint32_t Debug_GetFrameCount() const;
    uint32_t Debug_GetVertexCount() const { return (uint32_t)m_vecVertices.size(); }
	virtual void SetPosition(const _float3& pos) override;
	virtual void SetVelocity(const _float3& vel) override;
	virtual void SetSize(const _float3& size) override;
	virtual void SetColor(const _float4& color) override;
	virtual void SetEmissive(const _float4& emissive) override;
	virtual void TranslateOwner(uint32_t ownerId, const _float3& delta) override;
	virtual void TransformOwner(uint32_t ownerId, const _float4x4& deltaMatrixData) override;
private:
    struct TRAIL_STREAM
    {
        std::deque<TRAIL_FRAME> Frames;
        _bool bHasLastPoint = false;
        _float3 vLastStart{};
        _float3 vLastEnd{};
        _float fTimeSinceLastAdd = 0.f;
        _float fIdleTime = 0.f;
        _float fTimeSinceLastRetract = 0.f;
        _float fTotalDistance = 0.f;
    };

    struct HANDLE_HASH
    {
        size_t operator()(const CHandle& hHandle) const noexcept
        {
            return std::hash<uint64_t>{}(hHandle.GetPackedValue());
        }
    };

    void AddPoint(TRAIL_STREAM& Stream, const _float3& vStart, const _float3& vEnd);
    void ResetStream(TRAIL_STREAM& Stream);
    void BuildTrailGeometry(const TRAIL_STREAM& Stream);

private:
    DESC     m_Desc;
    TRAIL_TYPE m_eTrailType;
    std::vector<TRAIL_VERTEX> m_vecVertices;
    std::unordered_map<CHandle, TRAIL_STREAM, HANDLE_HASH> m_TrailStreams;
	_float4 m_vColor{1.f,0.f,0.f,1.f};
	_float4 m_vEmissive{ 1.f, 0.f, 1.f, 1.f };
	_float   m_fSampleInterval = 1.f / 60.f;
	_float m_fIdleThreshold = 0.1f;    // 이 시간 이상 AddPoint 없으면 "멈췄다"고 판단
	_float m_fRetractInterval = 0.02f; // 멈춘 뒤, 이 간격마다 꼬리 1프레임씩 강제 제거
	_float m_ScrollOffset = 0.2f;
	_float totalLength = 0.0f;
	_float m_fAccumulationTime = 0;
	uint32_t diffuseFrames = 0;
	uint32_t currentFrame = 0;

public:
	static UPtr<CParticle> Create(void* pArg);
	float EaseOutQuad(float x);
	float EaseOutCubic(float x);
	float EaseOutPow(float x, float n);
	float EaseOutExpo(float x);
	float EaseOutSine(float x);
private:
	SPtr<class CResTexture2D> m_pDistortionTexture;
	SPtr<class CResDynamicBuffer> m_pResVertexBuffer;
	SPtr<class CResCBuffer> m_pScrollCBuffer;
};

NS_END
