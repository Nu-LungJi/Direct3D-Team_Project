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

static _float3 vStart = { 10.f, 9.f, 10.f };
static _float3 vEnd = { 10.f, 11.f, 10.f };

static _float3 vSwingPivot = { 10.f, 10.f, 10.f };
static _float  fBladeGrip = 0.5f;
static _float  fBladeLength = 3.f;
static _float  fSwingStartDeg = -60.f;
static _float  fSwingEndDeg = 150.f;
static _float  fSwingDuration = 0.3f;
static _float  fSwingElapsed = 0.f;
static _bool   bSwinging = false;
static _bool   bSpawn = false;
static int  iCount = 0;
static float fTime = 1;
void CParticleManager::UpdateGUI()
{
    ImGui::Begin("CParticleManager");

    static StringID selectedGroup;
    static StringID selectedType;
    static int groupIndex = 0;
    static int typeIndex = 0;
    static SPAWN_COMMAND_KIND currentKind = SPAWN_COMMAND_KIND::STANDARD;

    // ---- 대분류/소분류 선택 ----
    std::vector<StringID> groupKeys;
    groupKeys.reserve(m_Particles.size());
    for (auto& [groupTag, typeMap] : m_Particles)
        groupKeys.push_back(groupTag);

    if (!groupKeys.empty())
    {
        std::vector<std::string> groupNamesStorage;
        groupNamesStorage.reserve(groupKeys.size());
        for (auto& key : groupKeys)
            groupNamesStorage.emplace_back(key.GetDbgStr());

        std::vector<const char*> groupNames;
        for (auto& s : groupNamesStorage)
            groupNames.push_back(s.c_str());

        groupIndex = std::clamp(groupIndex, 0, (int)groupNames.size() - 1);
        if (ImGui::Combo("Group", &groupIndex, groupNames.data(), (int)groupNames.size()))
            typeIndex = 0;

        selectedGroup = groupKeys[groupIndex];

        auto& typeMap = m_Particles[selectedGroup];
        std::vector<StringID> typeKeys;
        typeKeys.reserve(typeMap.size());
        for (auto& [typeTag, particle] : typeMap)
            typeKeys.push_back(typeTag);

        if (!typeKeys.empty())
        {
            std::vector<std::string> typeNamesStorage;
            typeNamesStorage.reserve(typeKeys.size());
            for (auto& key : typeKeys)
                typeNamesStorage.emplace_back(key.GetDbgStr());

            std::vector<const char*> typeNames;
            for (auto& s : typeNamesStorage)
                typeNames.push_back(s.c_str());

            typeIndex = std::clamp(typeIndex, 0, (int)typeNames.size() - 1);
            ImGui::Combo("Type", &typeIndex, typeNames.data(), (int)typeNames.size());

            selectedType = typeKeys[typeIndex];

            // 선택된 파티클이 CBeam_CPU인지 판단해서 섹션 분기
            // dynamic_cast로 실제 타입 확인 (또는 CParticle에 GetKind() 같은 걸 추가해도 됨)
            auto pSelected = typeMap[selectedType].get();
            currentKind = (dynamic_cast<CBeam_CPU*>(pSelected) != nullptr)
                ? SPAWN_COMMAND_KIND::BEAM
                : SPAWN_COMMAND_KIND::STANDARD;
        }
    }

    ImGui::Separator();

    // ---- 종류별 매개변수 섹션 ----
    static SPAWN_COMMAND pending{};   // 지금 입력 중인 명령 (아직 큐에 안 들어감)

    pending.kind = currentKind;
    pending.sGroupTag = selectedGroup;
    pending.sTypeTag = selectedType;

    if (currentKind == SPAWN_COMMAND_KIND::STANDARD)
    {
        ImGui::Text("Standard Particle Params");
        int countInput = (int)pending.count;
        ImGui::InputInt("Count", &countInput);
        pending.count = (uint32_t)std::clamp(countInput, 1, (int)MAX_SPAWN_PER_CALL);

        ImGui::InputFloat3("Position", &pending.position.x);
        ImGui::InputFloat3("Velocity", &pending.velocity.x);
        ImGui::InputFloat("Life", &pending.life);
        ImGui::InputFloat("Size", &pending.size);
        ImGui::ColorEdit4("BaseColor", &pending.color.x);
        //mGui::ColorEdit4("Emissive", &pending.emissive.x);
        ImGui::ColorEdit3("Emissive Color", &pending.emissive.x);
        ImGui::InputFloat("Emissive Intensity", &pending.emissive.w);
        ImGui::Checkbox("Loop", &pending.bLoop);
        if (pending.bLoop)
            ImGui::InputFloat("Spawn Interval", &pending.fSpawnInterval);
    }
    else if (currentKind == SPAWN_COMMAND_KIND::BEAM)
    {
      
        std::vector<_float3> vecJaggedPoints;
        ImGui::Text("Beam Params");
        ImGui::InputFloat4("Start Pos", &pending.beamStart.x);
        ImGui::InputFloat4("End Pos", &pending.beamEnd.x);
        ImGui::InputInt("DisplacementIterations", &pending.iDisplacementIterations);
        ImGui::InputFloat("DisplacementAmplitude", &pending.fDisplacementAmplitude);
        ImGui::InputFloat("DisplacementDamping", &pending.fDisplacementDamping);
        ImGui::InputFloat("flickerTimeInverval", &pending.flickerTimeInverval);
        ImGui::InputFloat("Duration", &pending.beamDuration);
        ImGui::ColorEdit4("BaseColor", &pending.color.x);
        ImGui::ColorEdit3("Emissive Color", &pending.emissive.x);
        ImGui::InputFloat("Emissive Intensity", &pending.emissive.w);
        //ImGui::ColorEdit4("Emissive", &pending.emissive.x);
    }

    if (ImGui::Button("Add to List") && !groupKeys.empty())
    {
        m_vecCommandQueue.push_back(pending);
    }

    ImGui::Separator();

    // ---- 리스트 표시 및 삭제 ----
    ImGui::Text("Spawn Queue (%zu)", m_vecCommandQueue.size());
    for (int i = 0; i < (int)m_vecCommandQueue.size(); ++i)
    {
        auto& cmd = m_vecCommandQueue[i];
        ImGui::PushID(i);

        if (cmd.kind == SPAWN_COMMAND_KIND::STANDARD)
        {
            ImGui::Text("[%s/%s] count=%u pos=(%.1f,%.1f,%.1f)",
                cmd.sGroupTag.GetDbgStr(), cmd.sTypeTag.GetDbgStr(),
                cmd.count, cmd.position.x, cmd.position.y, cmd.position.z);
        }
        else
        {
            ImGui::Text("[%s/%s] BEAM start=(%.1f,%.1f,%.1f)",
                cmd.sGroupTag.GetDbgStr(), cmd.sTypeTag.GetDbgStr(),
                cmd.beamStart.x, cmd.beamStart.y, cmd.beamStart.z);
        }

        ImGui::SameLine();
        if (ImGui::Button("Remove"))
        {
            m_vecCommandQueue.erase(m_vecCommandQueue.begin() + i);
            ImGui::PopID();
            break;   // 벡터가 바뀌었으니 이번 프레임은 여기서 루프 중단
        }

        ImGui::PopID();
    }

    if (ImGui::Button("Clear List"))
    {
        m_vecCommandQueue.clear();
    }

    ImGui::SameLine();

    if (ImGui::Button("Execute Spawn (All)"))
    {
        ExecuteCommandQueue();
    }

    ImGui::End();
}
//void CParticleManager::UpdateGUI()
//{
//    ImGui::Begin("CParticleManager");
//
//    ImGui::InputFloat3("SwingPivot", &vSwingPivot.x);
//    ImGui::InputFloat("BladeLength", &fBladeLength);
//    ImGui::InputFloat("SwingDuration", &fSwingDuration);
//
//    // 예시: [beam][trail] 같은 키로 등록되어 있다고 가정
//    auto pTrail = static_cast<CTrail_CPU*>(GetParticle("TRAIL", "SLASH"));
//
//    if (pTrail)
//    {
//        if (ImGui::Button("Swing!"))
//        {
//            fSwingElapsed = 0.f;
//            bSwinging = true;
//        }
//
//        if (bSwinging)
//        {
//            _float fDeltaTime = ImGui::GetIO().DeltaTime;
//            fSwingElapsed += fDeltaTime;
//
//            _float t = fSwingElapsed / fSwingDuration;
//            if (t >= 1.f)
//            {
//                t = 1.f;
//                bSwinging = false;
//            }
//
//            _float fAngleDeg = fSwingStartDeg + (fSwingEndDeg - fSwingStartDeg) * t;
//            _float fAngleRad = XMConvertToRadians(fAngleDeg);
//            _float3 vDir = { 0.f, sinf(fAngleRad), cosf(fAngleRad) };
//
//            _float3 vBase = {
//                vSwingPivot.x + vDir.x * fBladeGrip,
//                vSwingPivot.y + vDir.y * fBladeGrip,
//                vSwingPivot.z + vDir.z * fBladeGrip
//            };
//            _float3 vTip = {
//                vSwingPivot.x + vDir.x * (fBladeGrip + fBladeLength),
//                vSwingPivot.y + vDir.y * (fBladeGrip + fBladeLength),
//                vSwingPivot.z + vDir.z * (fBladeGrip + fBladeLength)
//            };
//
//            pTrail->AddPoint(vBase, vTip);
//        }
//    }
//
//    static int spawnCountInput = 10;
//    static float posX = 10;
//    static float posY = 10;
//    static float posZ = 10;
//    static float size = 10;
//    static int isLoop = 0;
//    ImGui::InputInt("SpawnCount", &spawnCountInput);
//    ImGui::InputFloat("X", &posX);
//    ImGui::InputFloat("Y", &posY);
//    ImGui::InputFloat("Z", &posZ);
//    ImGui::InputFloat("SIZE", &size);
//    ImGui::InputInt("Bool", &isLoop);
//    spawnCountInput = std::clamp(spawnCountInput, 1, (int)MAX_SPAWN_PER_CALL);
//
//    
//    if (ImGui::Button("RandParticle"))
//    {
//        const uint32_t spawnCount = static_cast<uint32_t>(spawnCountInput);
//
//        std::vector<PARTICLE_SPAWN_DATA> spawnList(spawnCount);
//        for (uint32_t i = 0; i < spawnCount; i++)
//        {
//            spawnList[i].position = _float3(posX, posY, posZ);
//            spawnList[i].velocity = _float3(
//                ((rand() % 100) / 10.f) - 5.f,
//                ((rand() % 100) / 10.f) + 5.f,
//                ((rand() % 100) / 10.f) - 5.f);
//            spawnList[i].life = 3.f;
//            spawnList[i].size = size;
//        }
//
//        if (isLoop == 0) {
//            Spawn("FIRE", "FIREBALL", spawnCount, spawnList.data());
//        }
//        else {
//            Spawn("FIRE", "FIREBALL", spawnCount, spawnList.data(), true, 0.5f);
//        }
//    }
//
//    static _float4 vRibbonStart = { 0.f, 0.f, 0.f, 1.f };
//    static _float4 vRibbonEnd = { 10.f, 0.f, 0.f, 1.f };
//
//    ImGui::InputFloat4("RibbonStart", &vRibbonStart.x);
//    ImGui::InputFloat4("RibbonEnd", &vRibbonEnd.x);
//
//    if (ImGui::Button("RandRibbon"))
//    {
//        SpawnRibbon(vRibbonStart, vRibbonEnd);
//    }
//
//    if (ImGui::Button("Test"))
//    {
//        bSpawn = true;
//       
//    }
//
//    if (bSpawn) {
//        fTime += 0.5f;
//        if (fTime >= 1.f) {
//            fTime = 0;
//
//            const uint32_t spawnCount = static_cast<uint32_t>(spawnCountInput);
//            std::vector<PARTICLE_SPAWN_DATA> spawnList(spawnCount);
//            for (uint32_t i = 0; i < spawnCount; i++)
//            {
//                //posY += 1;
//    
//                spawnList[i].position = _float3(posX, posY, posZ);
//                spawnList[i].velocity = _float3(0, 0, 0);
//                spawnList[i].life = 3.f;
//                spawnList[i].size = size;
//                spawnList[i].color = _float4(1, 1, 1,1);
//                posX += 1;
//
//            }
//            Spawn("FIRE", "FIRESMOKE", spawnCount, spawnList.data());
//            iCount++;
//            posZ += 1;
//            posX = 0;
//        }
//
//        if (iCount == 30) {
//            bSpawn = false;
//            iCount = 0;
//        }
//    }
//
//    ImGui::End();
//}

void CParticleManager::Update(_float fTimeDelta)
{
    for (auto& [groupTag, typeMap] : m_Particles)
    {
        for (auto& [typeTag, particle] : typeMap)
        {
            particle->PriorityUpdate(fTimeDelta);
            particle->Update(fTimeDelta);
            particle->LateUpdate(fTimeDelta);
        }
    }

    // 반복 스폰 요청 처리
    for (auto& req : m_LoopRequests)
    {
        req.fElapsed += fTimeDelta;
        if (req.fElapsed < req.fSpawnInterval)
            continue;

        req.fElapsed -= req.fSpawnInterval;

        // bLoop=false로 내부 호출 - 여기서 또 등록해버리면 무한 중복 등록됨
        Spawn(req.sGroupTag, req.sTypeTag, (uint32_t)req.vecSpawnData.size(), req.vecSpawnData.data());
    }
}

HRESULT CParticleManager::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
    for (auto& [groupTag, typeMap] : m_Particles)
    {
        for (auto& [typeTag, particle] : typeMap)
        {
            particle->Render(pContext, ctx);
        }
    }
    return S_OK;
}

HRESULT CParticleManager::Add_Particle(const StringID& sGroupTag, const StringID& sTypeTag, UPtr<CParticle> particle)
{
    if (particle == nullptr)
        return E_FAIL;

    if (FAILED(particle->Initialize(nullptr)))
        return E_FAIL;

    auto& typeMap = m_Particles[sGroupTag];   // 없으면 자동 생성됨
    if (typeMap.contains(sTypeTag))
        return E_FAIL;  // 이미 등록된 [그룹][타입]

    typeMap[sTypeTag] = std::move(particle);
    return S_OK;
}

HRESULT CParticleManager::Spawn(const StringID& sGroupTag, const StringID& sTypeTag,
    uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData,
    _bool bLoop, _float fSpawnInterval)
{
    if (pSpawnData == nullptr || count == 0)
        return E_FAIL;

    auto groupIt = m_Particles.find(sGroupTag);
    if (groupIt == m_Particles.end())
        return E_FAIL;

    auto typeIt = groupIt->second.find(sTypeTag);
    if (typeIt == groupIt->second.end())
        return E_FAIL;

    HRESULT hr = typeIt->second->Spawn(count, pSpawnData);
    if (FAILED(hr))
        return hr;

    if (bLoop)
    {
        PARTICLE_LOOP_REQUEST req{};
        req.sGroupTag = sGroupTag;
        req.sTypeTag = sTypeTag;
        req.vecSpawnData.assign(pSpawnData, pSpawnData + count);
        req.fSpawnInterval = fSpawnInterval;
        req.fElapsed = 0.f;

        m_LoopRequests.push_back(std::move(req));
    }

    return hr;
}

HRESULT CParticleManager::SpawnRandomInGroup(const StringID& sGroupTag,
    uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData,
    _bool bLoop, _float fSpawnInterval)
{
    if (pSpawnData == nullptr || count == 0)
        return E_FAIL;

    auto groupIt = m_Particles.find(sGroupTag);
    if (groupIt == m_Particles.end() || groupIt->second.empty())
        return E_FAIL;

    auto& typeMap = groupIt->second;
    uint32_t randIndex = rand() % (uint32_t)typeMap.size();

    auto it = typeMap.begin();
    std::advance(it, randIndex);

    HRESULT hr = it->second->Spawn(count, pSpawnData);
    if (FAILED(hr))
        return hr;

    if (bLoop)
    {
        PARTICLE_LOOP_REQUEST req{};
        req.sGroupTag = sGroupTag;
        req.sTypeTag = it->first;   // 이번에 뽑힌 소분류로 고정 (반복 시엔 매번 재추첨 안 함)
        req.vecSpawnData.assign(pSpawnData, pSpawnData + count);
        req.fSpawnInterval = fSpawnInterval;
        req.fElapsed = 0.f;

        m_LoopRequests.push_back(std::move(req));
    }

    return hr;
}

HRESULT CParticleManager::SpawnAllInGroup(const StringID& sGroupTag,
    uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData)
{
    if (pSpawnData == nullptr || count == 0)
        return E_FAIL;

    auto groupIt = m_Particles.find(sGroupTag);
    if (groupIt == m_Particles.end() || groupIt->second.empty())
        return E_FAIL;

    HRESULT hr = S_OK;
    for (auto& [typeTag, particle] : groupIt->second)
    {
        if (FAILED(particle->Spawn(count, pSpawnData)))
            hr = E_FAIL;
    }
    return hr;
}

HRESULT CParticleManager::SpawnRibbon(uint32_t quantity, const _float4& start, const _float4& end, _float fDisplacementAmplitude, _float iDisplacementIterations, _float fDisplacementDamping, _float fFlickerInterval, _float4 emissive, _float fDuration)
{
    auto pParticle = GetParticle("BEAM", "ATTACK");
    if (!pParticle)
        return E_FAIL; 
    auto pBeam = static_cast<CBeam_CPU*>(pParticle);

    for (uint32_t i = 0; i < quantity; i++) {
        int32_t idx1 = pBeam->AddBeam(start, end, fDisplacementAmplitude, (uint32_t)iDisplacementIterations, fDisplacementDamping, fFlickerInterval, emissive, fDuration);
    }
    return S_OK;
}

CParticle* CParticleManager::GetParticle(const StringID& sGroupTag, const StringID& sTypeTag) const
{
    auto groupIt = m_Particles.find(sGroupTag);
    if (groupIt == m_Particles.end())
        return nullptr;

    auto typeIt = groupIt->second.find(sTypeTag);
    if (typeIt == groupIt->second.end())
        return nullptr;

    return typeIt->second.get();
}

bool CParticleManager::HasGroup(const StringID& sGroupTag) const
{
    return m_Particles.find(sGroupTag) != m_Particles.end();
}

UPtr<CParticleManager> CParticleManager::Create()
{
    return UPtr<CParticleManager>(new CParticleManager{});
}

HRESULT CParticleManager::ExecuteCommandQueue()
{
    HRESULT hr = S_OK;
    for (auto& cmd : m_vecCommandQueue)
    {

        //파일 path를 읽어서 
        //cmd.baemStart를 본인 특정 좌표로 
        if (cmd.kind == SPAWN_COMMAND_KIND::STANDARD)
        {
            std::vector<PARTICLE_SPAWN_DATA> spawnList(cmd.count);
            for (auto& s : spawnList)
            {
                s.position = cmd.position;
                s.velocity = cmd.velocity;
                s.life = cmd.life;
                s.size = cmd.size;
                s.color = cmd.color;   // PARTICLE_SPAWN_DATA에 color 필드가 있다는 전제
                s.emissive = cmd.emissive;
            }

            if (FAILED(Spawn(cmd.sGroupTag, cmd.sTypeTag, cmd.count, spawnList.data(), cmd.bLoop, cmd.fSpawnInterval)))
                hr = E_FAIL;
        }
        else if (cmd.kind == SPAWN_COMMAND_KIND::BEAM)
        {
            auto pParticle = GetParticle(cmd.sGroupTag, cmd.sTypeTag);
            if (pParticle)
            {
                auto pBeam = static_cast<CBeam_CPU*>(pParticle);
                pBeam->AddBeam(cmd.beamStart, cmd.beamEnd,
                    cmd.fDisplacementAmplitude, (uint32_t)cmd.iDisplacementIterations, cmd.fDisplacementDamping,
                    cmd.flickerTimeInverval, cmd.emissive,cmd.beamDuration);
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }

  //  m_vecCommandQueue.clear();   // 실행 후 큐 비우기 (원하시면 안 비우게 바꿀 수도 있음)
    return hr;
}

//HRESULT CParticleManager::ExecuteCommandQueue()
//{
//    HRESULT hr = S_OK;
//
//    // 그룹/타입별로 스폰 데이터를 모으기
//    std::map<std::pair<StringID, StringID>, std::vector<PARTICLE_SPAWN_DATA>> batched;
//
//    for (auto& cmd : m_vecCommandQueue)
//    {
//        if (cmd.kind == SPAWN_COMMAND_KIND::STANDARD)
//        {
//            auto& vec = batched[{cmd.sGroupTag, cmd.sTypeTag}];
//            for (uint32_t i = 0; i < cmd.count; ++i)
//            {
//                PARTICLE_SPAWN_DATA s{};
//                s.position = cmd.position;
//                s.velocity = cmd.velocity;
//                s.life = cmd.life;
//                s.size = cmd.size;
//                s.color = cmd.color;
//                s.emissive = cmd.emissive;
//                vec.push_back(s);
//            }
//        }
//        else if (cmd.kind == SPAWN_COMMAND_KIND::BEAM)
//        {
//            // 기존과 동일
//            auto pParticle = GetParticle(cmd.sGroupTag, cmd.sTypeTag);
//            if (pParticle)
//            {
//                auto pBeam = static_cast<CBeam_CPU*>(pParticle);
//                pBeam->AddBeam(cmd.beamStart, cmd.beamEnd,
//                    cmd.fDisplacementAmplitude, (uint32_t)cmd.iDisplacementIterations, cmd.fDisplacementDamping,
//                    cmd.flickerTimeInverval,cmd.emissive, cmd.beamDuration);
//            }
//            else hr = E_FAIL;
//        }
//    }
//
//    // 그룹/타입별로 한 번씩만 Spawn 호출
//    for (auto& [key, spawnList] : batched)
//    {
//        if (FAILED(Spawn(key.first, key.second, (uint32_t)spawnList.size(), spawnList.data())))
//            hr = E_FAIL;
//    }
//
//    return hr;
//}
