#pragma once
#include "Engine_Defines.h"
#include "Particle.h"
NS_BEGIN(Engine)
class ENGINE_DLL CBeam_CPU final: public CParticle
{
public:
    struct DESC
    {
        std::pair<StringID, StringID> textureID;
        std::pair<StringID, StringID> VSID;
        std::pair<StringID, StringID> PSID;
		std::pair<StringID, StringID> normalTextureID;
		std::pair<StringID, StringID> distortionTextureID;
		std::pair<StringID, StringID> noiseTextureID;
        PARTICLE_TYPE type;
        _float      fWidth = 1.f;
        _float      fScrollSpeed = 1.f;
        uint32_t    iDisplacementIterations = 6;      // 기본값 (AddBeam에서 안 넘기면 이걸 씀)
        _float      fDisplacementAmplitude = 2.5f;
        _float      fDisplacementDamping = 0.25f;
        _float      fFlickerInterval = 0.01f;
        uint32_t    iMaxBeams = 16;
        uint32_t    iMaxDisplacementIterations = 10;  // 버퍼 크기 산정용 - 실제 사용 가능한 최댓값
		uint32_t    geometryType = 0; // 0은 기본 번개 모양 1은 부드러운 곡선 모양

		_string sVEntryPoint = "";
		_string sPEntryPoint = "";
		uint32_t blendState = 0;

    };

    struct BEAM_INSTANCE
    {
        _bool       bActive = false;
        _float4     vStartPos = {};
        _float4     vEndPos = {};
        _float      fElapsedTime = 0.f;  // 빔 수명
        _float      fDuration = 0.f;
        uint32_t    iDisplacementIterations = 6;		 //변위를 몇 겹으로(예: 여러 주파수의 사인파를 겹쳐서) 계산할지. 값이 클수록 더 복잡하고 디테일한 떨림 패턴이 나옴
        _float      fDisplacementAmplitude = 2.5f;		 //빔이 원래 직선에서 얼마나 크게 흔들릴지(진폭). 값이 클수록 더 크게 출렁임.
        _float      fDisplacementDamping = 0.25f;		//반복(iteration)마다 진폭을 얼마나 감쇠시킬지. 1보다 작은 값이면 고주파 성분일수록 흔들림이 약해져서 자연스러운 지글거림이 됨.
        _float      fFlickerInterval = 0.1f;			//빔이 깜빡이는(flicker) 주기. 예를 들어 0.05초마다 한 번씩 랜덤하게 위치/밝기를 갱신하는 식의 전기 스파크 느낌을 낼 때 쓰는 간격.
        _float      fFlickerTimer = 0.f;				 //다음 깜빡임까지 남은 시간을 세는 카운트다운 타이머.
        _float4     vColor = _float4(1, 1, 1, 1);
        _float4     vEmissive = _float4(1, 1, 1, 1);
        // 이 빔만의 세그먼트 정보 (개별적으로 다를 수 있음)
        uint32_t    iSegmentCount = 0;        // = 2^iDisplacementIterations
        uint32_t    iVerticesPerPlane = 0;    // = (iSegmentCount+1) * 2

        std::vector<_float3> vecJaggedPoints;
    };

private:
    CBeam_CPU();
    virtual ~CBeam_CPU();
public:
    virtual HRESULT Initialize(void* pArg) override;
    virtual void    PriorityUpdate(_float fTimeDelta) override;
    virtual void    Update(_float fTimeDelta) override;
    virtual void    LateUpdate(_float fTimeDelta) override;
    virtual HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;
    virtual HRESULT Spawn(uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData) override;
	virtual void ClearByOwner(uint32_t ownerID) override;

public:
    int32_t AddBeam(const _float4& vStart, const _float4& vEnd,
        _float fDisplacementAmplitude, uint32_t iDisplacementIterations, _float fDisplacementDamping,
        _float fFlickerInterval, const _float4& vColor, _float4 emissive, _float fDuration = 0.f);
	int32_t AddBeam(const _float4& vStart, const _float4& vEnd);
    void    SetBeamActive(uint32_t beamIndex, _bool bActive, _float fDuration = 0.f);
    void    SetStartPos(uint32_t beamIndex, const _float4& vPos);
    void    SetEndPos(uint32_t beamIndex, const _float4& vPos);

private:
    void RegenerateJaggedPath(BEAM_INSTANCE& beam);
    void BuildBeamGeometry();
	void RegenerateSinPath(BEAM_INSTANCE& beam);

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

private:
	_float      m_fElapsedTime = 0.f;
	_float      m_fDuration = 0.f;
	uint32_t    m_iDisplacementIterations = 6;
	_float      m_fDisplacementAmplitude = 2.5f;
	_float      m_fDisplacementDamping = 0.25f;
	_float      m_fFlickerInterval = 0.1f;
	_float      m_fFlickerTimer = 0.f;
	_float4     m_vColor = _float4(1, 1, 1, 1);
	_float4     m_vEmissive = _float4(1, 1, 1, 1);
	// 이 빔만의 세그먼트 정보 (개별적으로 다를 수 있음)
	uint32_t    m_iSegmentCount = 0;        // = 2^iDisplacementIterations
	uint32_t    m_iVerticesPerPlane = 0;    // = (iSegmentCount+1) * 2
public:
	static UPtr<CParticle> Create(void* pArg);
};
NS_END
