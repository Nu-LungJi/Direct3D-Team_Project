#include "pch.h"
#include "ParticleManager.h"
#include "Particle.h"
#include "Beam_CPU.h"
#include "Trail_CPU.h"

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
        particle->LateUpdate(fTimeDelta); 
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
static _float3 vStart = { 10.f, 9.f, 10.f };
static _float3 vEnd = { 10.f, 11.f, 10.f };

static _float3 vSwingPivot = { 10.f, 10.f, 10.f }; // 어깨/손목 위치 (고정 피벗)
static _float  fBladeGrip = 0.5f;  // 피벗~밑동 거리 (손잡이 길이)
static _float  fBladeLength = 3.f;   // 밑동~칼끝 길이
static _float  fSwingStartDeg = -60.f;
static _float  fSwingEndDeg = 150.f;
static _float  fSwingDuration = 0.3f; // 이 시간 동안 -60도~60도로 휘두름
static _float  fSwingElapsed = 0.f;
static _bool   bSwinging = false;


void CParticleManager::UpdateGUI()
{
    ImGui::Begin("CParticleManager");


    ImGui::InputFloat3("SwingPivot", &vSwingPivot.x);
    ImGui::InputFloat("BladeLength", &fBladeLength);
    ImGui::InputFloat("SwingDuration", &fSwingDuration);


    auto pTrail = static_cast<CTrail_CPU*>(m_Particles[ETOUI(PARTICLE_TYPE::TRAIL)].get());


    if (ImGui::Button("Swing!"))
    {
        fSwingElapsed = 0.f;
        bSwinging = true;
    }

    if (bSwinging)
    {
        _float fDeltaTime = ImGui::GetIO().DeltaTime; // UpdateGUI엔 엔진 fTimeDelta가 안 넘어오니 ImGui 델타 사용
        fSwingElapsed += fDeltaTime;

        _float t = fSwingElapsed / fSwingDuration;
        if (t >= 1.f)
        {
            t = 1.f;
            bSwinging = false; // 이번이 마지막 기록
        }

        // -60도~60도 사이를 선형 보간, XZ 평면(수평)에서 회전하는 부채꼴 궤적
        _float fAngleDeg = fSwingStartDeg + (fSwingEndDeg - fSwingStartDeg) * t;
        _float fAngleRad = XMConvertToRadians(fAngleDeg);
        _float3 vDir = { 0.f, sinf(fAngleRad), cosf(fAngleRad) };
        //_float3 vDir = { cosf(fAngleRad), 0.f, sinf(fAngleRad) };

        _float3 vBase = {
            vSwingPivot.x + vDir.x * fBladeGrip,
            vSwingPivot.y + vDir.y * fBladeGrip,
            vSwingPivot.z + vDir.z * fBladeGrip
        };
        _float3 vTip = {
            vSwingPivot.x + vDir.x * (fBladeGrip + fBladeLength),
            vSwingPivot.y + vDir.y * (fBladeGrip + fBladeLength),
            vSwingPivot.z + vDir.z * (fBladeGrip + fBladeLength)
        };

        pTrail->AddPoint(vBase, vTip);
    }


    static int spawnCountInput = 10;
    static float posX = 10;
    static float posY = 10;
    static float posZ = 10;
    static float size = 10;
    static int isLoop = 0;
    ImGui::InputInt("SpawnCount", &spawnCountInput);
    ImGui::InputFloat("X", &posX);
    ImGui::InputFloat("Y", &posY);
    ImGui::InputFloat("Z", &posZ);
    ImGui::InputFloat("SIZE", &size);
    ImGui::InputInt("Bool", &isLoop);
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

        if (isLoop == 0) {
            Spawn(PARTICLE_TYPE::FIRE, spawnCount, spawnList.data());
        }
        else {
            Spawn(PARTICLE_TYPE::FIRE, spawnCount, spawnList.data(),true,0.5f);

        }

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

    // Initialize()가 m_eType을 채워주므로, 타입 비교는 반드시 이 뒤에 해야 한다.
    // 순서가 바뀌면 아직 초기화 안 된(쓰레기값) m_eType으로 비교하게 되어
    // 엉뚱하게 "이미 등록된 타입"으로 오판하고 튕겨낼 수 있다.
    if (FAILED(particle->Initialize(nullptr)))
        return E_FAIL;

    for (auto& p : m_Particles)
    {
        if (p->Get_Type() == particle->Get_Type())
            return E_FAIL; // 이미 등록된 타입

    }

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