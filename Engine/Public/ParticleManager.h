#pragma once
#include "Engine_Defines.h"
NS_BEGIN(Engine)
class CParticle;

struct PARTICLE_LOOP_REQUEST
{
    StringID                          sGroupTag;
    StringID                          sTypeTag;
    std::vector<PARTICLE_SPAWN_DATA>  vecSpawnData;
    _float                            fSpawnInterval = 0.1f;
    _float                            fElapsed = 0.f;
};
enum class SPAWN_COMMAND_KIND
{
    STANDARD,   // 위치/속도/수명/크기/색 - 대부분의 파티클
    BEAM,       // 시작점/끝점/지속시간 - CBeam_CPU 계열
	STAIR,
};

struct STANDARD_PARAMS
{
    uint32_t count = 1;
    _float3  position = {};
    _float3  velocity = {};
    _float   life = 1.f;
    _float   size = 1.f;
    _float4  color = { 1.f, 1.f, 1.f, 1.f };
    _float4  emissive = { 1.f, 1.f, 1.f, 1.f };
    _bool    bLoop = false;
    _float   fSpawnInterval = 0.1f;
};

struct BEAM_PARAMS
{
    _float4  beamStart = {};
    _float4  beamEnd = {};
    _float4  color = { 1.f, 1.f, 1.f, 1.f };
    _float4  emissive = { 1.f, 1.f, 1.f, 1.f };
    int      iDisplacementIterations = 6;
    _float   fDisplacementAmplitude = 2.5f;
    _float   fDisplacementDamping = 0.25f;
    _float   flickerTimeInverval = 0.25f;
    _float   beamDuration = 0.f;
};
struct STAIR_PARAMS
{
	_float3  vStartPos = {};
	uint32_t iStepCount = 5;
	_float   fStepWidth = 2.f;
	_float   fStepHeight = 1.5f;
	_float   fStepDepth = 2.f;
	_float   life = 1.f;
	_float4  color = { 1.f, 1.f, 1.f, 1.f };
	_float4  emissive = { 1.f, 1.f, 1.f, 1.f };
};

// 나중에 새 파티클 종류(예: RIBBON, DECAL 등) 추가되면 여기 구조체만 추가하면 됨
struct SPAWN_COMMAND
{
    SPAWN_COMMAND_KIND sGroupTag_KindTag = SPAWN_COMMAND_KIND::STANDARD; // 이름은 기존 kind 그대로 사용해도 됨
    StringID sGroupTag;
    StringID sTypeTag;
    std::variant<STANDARD_PARAMS, BEAM_PARAMS, STAIR_PARAMS> params;
};
struct PARTICLE_EFFECT_PRESET
{
    std::string sEffectName;              // 저장 시 식별용 이름 (예: "Explosion_Fire")
    std::vector<SPAWN_COMMAND> commands;  // 이 이펙트를 구성하는 여러 스폰 명령
};
//struct SPAWN_COMMAND
//{
//    SPAWN_COMMAND_KIND kind = SPAWN_COMMAND_KIND::STANDARD;
//    StringID sGroupTag;
//    StringID sTypeTag;
//    //공용
//    uint32_t count = 1;
//    _float4  emissive = { 1.f, 1.f, 1.f, 1.f };
//
//    // STANDARD 용
//    _float3  position = {};
//    _float3  velocity = {};
//    _float   life = 1.f;
//    _float   size = 1.f;
//    _float4  color = { 1.f, 1.f, 1.f, 1.f };
//    _bool    bLoop = false;
//    _float   fSpawnInterval = 0.1f;
//    
//    // BEAM 용
//    _float4  beamStart = {};
//    _float4  beamEnd = {};
//    int    iDisplacementIterations = 6;   // 재귀 세분화 횟수 → 세그먼트 개수 = 2^6 = 64개
//    _float      fDisplacementAmplitude = 2.5f; // 첫 세분화 단계에서 중점을 얼마나 크게 흔들지
//    _float      fDisplacementDamping = 0.25f;// 몇 초마다 지그재그 모양을 새로 뽑을지 (짧을수록 번개가 파르르 떠는 느낌)
//    _float      flickerTimeInverval = 0.25f; // fFlickerInterval에 도달할 때마다 리셋되는 타이머 (지그재그 재생성 시점 체크용)
//    _float   beamDuration = 0.f;
//};
class ENGINE_DLL CParticleManager final : public CEngineBase, public IRenderable
{
private:
    explicit CParticleManager();
    virtual ~CParticleManager();
public:
    CParticleManager(const CParticleManager&) = delete;
    CParticleManager& operator=(const CParticleManager& rhs) = delete;
public:
    void Update(_float fTimeDelta);
    HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;
    void UpdateGUI();
    bool HasRenderPass(RENDERPASS ePass) const override { return ePass == RENDERPASS::DEFAULT; };

public:
    // 사전 등록 - [대분류][소분류]로 저장
    HRESULT Add_Particle(const StringID& sGroupTag, const StringID& sTypeTag, UPtr<CParticle> particle);

    // 정확히 지정해서 스폰
    HRESULT Spawn(const StringID& sGroupTag, const StringID& sTypeTag,
        uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData,
        _bool bLoop = false, _float fSpawnInterval = 0.1f);

    // 대분류 안에서 랜덤하게 하나 골라 스폰 (예: "stone" 안에서 아무 파편이나)
    HRESULT SpawnRandomInGroup(const StringID& sGroupTag,
        uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData,
        _bool bLoop = false, _float fSpawnInterval = 0.1f);

    // 대분류 전체에 동시에 스폰 (필요하다면)
    HRESULT SpawnAllInGroup(const StringID& sGroupTag,
        uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData);

    HRESULT SpawnRibbon(uint32_t quantity, const _float4& start, const _float4& end,
        _float fDisplacementAmplitude, _float iDisplacementIterations, _float fDisplacementDamping,
        _float fFlickerInterval, const _float4& vColor, _float4 emissive, _float fDuration);

public:
    // 조회 헬퍼
    CParticle* GetParticle(const StringID& sGroupTag, const StringID& sTypeTag) const;
    bool HasGroup(const StringID& sGroupTag) const;

public:
    static UPtr<CParticleManager> Create();

private:
    // [대분류][소분류] -> 파티클 인스턴스
    std::unordered_map<StringID, std::unordered_map<StringID, UPtr<CParticle>>> m_Particles;
    std::vector<PARTICLE_LOOP_REQUEST> m_LoopRequests;



private:
    std::vector<SPAWN_COMMAND> m_vecCommandQueue;
    // 큐 전체를 실행
    HRESULT ExecuteCommandQueue();
};
NS_END
