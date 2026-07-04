#include "pch.h"
#include "ParticleManager.h"
#include "Particle.h"
#include "Beam_CPU.h"

NS_USING(Engine)

CParticleManager::CParticleManager()
{
}

CParticleManager::~CParticleManager()
{
}

void CParticleManager::Update(_float fTimeDelta)
{
    for (auto& particle : m_Particles)
    {
        particle->PriorityUpdate(fTimeDelta);
        particle->Update(fTimeDelta);
        particle->LateUpdate(fTimeDelta); // 여기서 전역 렌더 큐에 등록됨
    }

    // 반복 스폰 요청 처리
    for (auto& req : m_LoopRequests)
    {
        req.fElapsed += fTimeDelta;
        if (req.fElapsed < req.fSpawnInterval)
            continue;

        req.fElapsed -= req.fSpawnInterval;

        // bLoop=false로 내부 호출 - 여기서 또 등록해버리면 무한 중복 등록됨
        Spawn(req.eType, (uint32_t)req.vecSpawnData.size(), req.vecSpawnData.data());
    }
}

HRESULT CParticleManager::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
    for (auto& particle : m_Particles) {
        particle->Render(pContext, ctx);
    }
   
    return S_OK;
}

void CParticleManager::UpdateGUI()
{
    ImGui::Begin("CParticleManager");

    static int spawnCountInput = 10;
    static float posX = 10;
    static float posY = 10;
    static float posZ = 10;
    static float size = 10;
    ImGui::InputInt("SpawnCount", &spawnCountInput);
    ImGui::InputFloat("X", &posX);
    ImGui::InputFloat("Y", &posY);
    ImGui::InputFloat("Z", &posZ);
    ImGui::InputFloat("SIZE", &size);
    spawnCountInput = std::clamp(spawnCountInput, 1, (int)MAX_SPAWN_PER_CALL);
    if (ImGui::Button("RandParticle"))
    {
        const uint32_t spawnCount = static_cast<uint32_t>(spawnCountInput);
        _float centerX = Randf(0.f, 50.f);
        _float centerZ = Randf(0.f, 50.f);

        std::vector<PARTICLE_SPAWN_DATA> spawnList(spawnCount);
        for (uint32_t i = 0; i < spawnCount; i++)
        {
            spawnList[i].position = _float3(
                posX, posY, posZ);

            spawnList[i].velocity = _float3(
                ((rand() % 100) / 10.f) - 5.f,
                ((rand() % 100) / 10.f) + 5.f,
                ((rand() % 100) / 10.f) - 5.f);

            spawnList[i].life = 3.f;
            spawnList[i].size = size;
        }

        Spawn(PARTICLE_TYPE::FIRE, spawnCount, spawnList.data());

    }
    static _float4 vRibbonStart = { 0.f, 0.f, 0.f,1.f };
    static _float4 vRibbonEnd = { 10.f, 0.f, 0.f ,1.f};

    ImGui::InputFloat4("RibbonStart", &vRibbonStart.x);
    ImGui::InputFloat4("RibbonEnd", &vRibbonEnd.x);

    if (ImGui::Button("RandRibbon"))
    {
        SpawnRibbon(vRibbonStart, vRibbonEnd);
    }
   
    ImGui::End();
}



HRESULT CParticleManager::Add_Particle(UPtr<CParticle> particle)
{
    if (particle == nullptr)
        return E_FAIL;

    for (auto& p : m_Particles)
    {
        if (p->Get_Type() == particle->Get_Type())
            return E_FAIL; // 이미 등록된 타입

    }

    // 각 파티클 클래스가 자기 설정을 스스로 채우므로 nullptr만 넘기면 된다.
    if (FAILED(particle->Initialize(nullptr)))
        return E_FAIL;

    m_Particles.push_back(std::move(particle));
    return S_OK;
}

HRESULT CParticleManager::Spawn(PARTICLE_TYPE type, uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData,
    _bool bLoop, _float fSpawnInterval)
{
    if (pSpawnData == nullptr || count == 0)
        return E_FAIL;

    HRESULT hr = E_FAIL;
    for (auto& particle : m_Particles)
    {
        if (particle->Get_Type() == type)
        {
            hr = particle->Spawn(count, pSpawnData);
            break;
        }
    }

    if (FAILED(hr))
        return hr; // 해당 타입이 없거나 스폰 실패 - 루프 등록도 하지 않음

    if (bLoop)
    {
        PARTICLE_LOOP_REQUEST req{};
        req.eType = type;
        req.vecSpawnData.assign(pSpawnData, pSpawnData + count); // 호출 시점 데이터를 복사해서 보관
        req.fSpawnInterval = fSpawnInterval;
        req.fElapsed = 0.f;

        m_LoopRequests.push_back(std::move(req));
    }

    return hr;
}

HRESULT CParticleManager::SpawnRibbon(const _float4& start, const _float4& end)
{
    for (auto& particle : m_Particles)
    {
        if (particle->Get_Type() == PARTICLE_TYPE::RIBBON)
        {
            auto pBeam = static_cast<CBeam_CPU*>(particle.get());
            pBeam->SetStartPos(start);
            pBeam->SetEndPos(end);
            pBeam->SetBeamActive(true);
            break;
        }
    }
    return S_OK;
}

UPtr<CParticleManager> CParticleManager::Create()
{
    return UPtr<CParticleManager>(new CParticleManager{});
}