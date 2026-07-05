#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)

class CParticle;

// 반복 스폰 요청 - Spawn()에 bLoop=true로 넘기면 여기 등록되고,
// Update()에서 fSpawnInterval마다 자동으로 다시 Spawn 된다.
struct PARTICLE_LOOP_REQUEST
{
    PARTICLE_TYPE                     eType;
    std::vector<PARTICLE_SPAWN_DATA>  vecSpawnData; // 호출 시점의 spawnData를 복사해서 보관
    _float                            fSpawnInterval = 0.1f;
    _float                            fElapsed = 0.f;
};

class ENGINE_DLL CParticleManager final: public CEngineBase, public IRenderable
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
    // 사전 등록 - 게임 시작 시점에 타입별로 한 번씩 호출
    HRESULT Add_Particle(UPtr<CParticle> particle);

    // bLoop=false: 한 번만 스폰.
    // bLoop=true : 한 번 스폰 후, fSpawnInterval마다 같은 spawnData로 반복 스폰 (내부 등록됨)
    HRESULT Spawn(PARTICLE_TYPE type, uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData,
        _bool bLoop = false, _float fSpawnInterval = 0.1f);
    HRESULT SpawnRibbon( const _float4& start, const _float4& end);
public:
    static UPtr<CParticleManager> Create();

private:
    std::vector<UPtr<CParticle>>        m_Particles;
    std::vector<PARTICLE_LOOP_REQUEST>  m_LoopRequests;
};

NS_END