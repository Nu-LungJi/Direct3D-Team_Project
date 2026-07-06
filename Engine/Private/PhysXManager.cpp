#include "pch.h"
#include "PhysXManager.h"
#include "PhysxManagerListener.h"
#include "GameInstance.h"

#ifdef _DEBUG
// 라이브러리 설정 전후로 매크로 잠시 해제
#undef new
#endif
#include "PxShape.h"
#include "PxPhysicsAPI.h"

#ifdef _DEBUG
#define new DBG_NEW
#endif


using namespace physx;
static physx::PxDefaultAllocator gDefaultAllocator;
static physx::PxDefaultErrorCallback gDefaultErrorCallback;

NS_USING(Engine)

CPhysXManager::CPhysXManager()
{
}

CPhysXManager::~CPhysXManager()
{
}

void CPhysXManager::UpdateGUI()
{
    ImGui::Begin("CPhysXManager");
    ImGui::Text("m_bDbgRender: %i", m_bDbgRender);
    if (ImGui::Button("DebugRender"))
    {
        m_bDbgRender = !m_bDbgRender;
    }
    ImGui::End();
}

void CPhysXManager::UpdateDebugRender(_float fTimeDelta)
{
    if (m_bDbgRender)
    {
        const PxRenderBuffer& renderBuffer = m_pScene->getRenderBuffer();
        const PxU32 nbLines = renderBuffer.getNbLines();
        const PxDebugLine* lines = renderBuffer.getLines();

        // TODO:순회가 아니라 memcpy방식으로 수정
        for (PxU32 i = 0; i < nbLines; i++) {
            const PxDebugLine& line = lines[i];
            CGameInstance::Get().GetDbgLineRender()
                ->AddLine(
                    _float3{ line.pos0.x, line.pos0.y, line.pos0.z },
                    _float3{ line.pos1.x, line.pos1.y, line.pos1.z }, ColorIntToFloat4(line.color0));
        }
    }
}

void CPhysXManager::Update(_float fTimeDeta)
{
    UpdateDebugRender(fTimeDeta);
}

static physx::PxFilterFlags MyFilterShader(
    physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
    physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
    physx::PxPairFlags& pairFlags, const void* constantBlock, physx::PxU32 constantBlockSize)
{
    // 기본 PhysX 필터링 로직을 그대로 호출
    physx::PxFilterFlags flags = physx::PxDefaultSimulationFilterShader(
        attributes0, filterData0, attributes1, filterData1, pairFlags, constantBlock, constantBlockSize);

    
    pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_FOUND;
    pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_LOST;

    return flags;
}

HRESULT CPhysXManager::Initialize()
{
    m_pListener = CPhysxManagerListener::Create();
    // Foundation 생성
    {
        m_pFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocator, gDefaultErrorCallback);
        if (!m_pFoundation)
        {
            MSG_BOX("PxCreateFoundation FAILED");
            return E_FAIL;
        }
    }
    
    // 피직스 생성
    {
        bool recordMemoryAllocations = false;

        // 피직스 비주얼디버거 설정
#ifdef _DEBUG
        recordMemoryAllocations = true;
        m_pPvd = physx::PxCreatePvd(*m_pFoundation);
        physx::PxPvdTransport* pPvdTransport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
        m_pPvd->connect(*pPvdTransport, PxPvdInstrumentationFlag::eALL);
#endif // DEBUG

        // ToleranceScale: 현실의 물리 법칙을 얼마나 정밀하게 계산할 것인가
        physx::PxTolerancesScale scale{ PxTolerancesScale() };
        m_pPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_pFoundation, scale, recordMemoryAllocations, m_pPvd);
        if (!m_pPhysics)
        {
            MSG_BOX("PxCreatePhysics FAILED");
            return E_FAIL;
        }

        if (!PxInitExtensions(*m_pPhysics, m_pPvd)) {
            MSG_BOX("PxInitExtensions FAILED");
            return E_FAIL;
        }
    }


    //{
    //    PxCudaContextManagerDesc cudaDesc;
    //    PxCudaContextManager* cudaContext =
    //        PxCreateCudaContextManager(*m_pFoundation, cudaDesc, nullptr);

    //    if (!cudaContext || !cudaContext->contextIsValid())
    //    {
    //        MSG_BOX("CUDA FAIL")
    //        // GPU PhysX 불가
    //    }
    //}

    // Scene 생성
    {
        _float fGravitY = -9.81f;
        uint32_t iCpuDispatcherCnt = 4;


        physx::PxSceneDesc sceneDesc(m_pPhysics->getTolerancesScale());
        sceneDesc.gravity = physx::PxVec3(0.0f, fGravitY, 0.0f);
        physx::PxDefaultSimulationFilterShader;
        sceneDesc.filterShader = MyFilterShader;
        sceneDesc.flags |= physx::PxSceneFlag::eENABLE_ACTIVE_ACTORS;
        sceneDesc.simulationEventCallback = m_pListener.get();

        m_pCpuDispatcher = physx::PxDefaultCpuDispatcherCreate(iCpuDispatcherCnt);
        if (!m_pCpuDispatcher)
        {
            MSG_BOX("PxDefaultCpuDispatcherCreate FAIL");
            return E_FAIL;
        }
        sceneDesc.cpuDispatcher = m_pCpuDispatcher;

        m_pScene = m_pPhysics->createScene(sceneDesc);
        if (!m_pScene)
        {
            MSG_BOX("createScene FAIL");
            return E_FAIL;
        }

        // for debug
        {
            m_pScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f); // 전체 스케일
            m_pScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f); // 충돌체 그리기
            m_pScene->setVisualizationParameter(PxVisualizationParameter::eACTOR_AXES, 1.0f);
            //m_pScene->setVisualizationParameter(PxVisualizationParameter::eCONTACT_POINT, 1.0f);
            //m_pScene->setVisualizationParameter(PxVisualizationParameter::eCONTACT_NORMAL, 1.0f);
            //m_pScene->setVisualizationParameter(PxVisualizationParameter::eCONTACT_FORCE, 1.0f);
            //m_pScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_DYNAMIC, 1.0f);
        }
    }

    
	return S_OK;
}

void CPhysXManager::StepSimulation(float fixedDeltaTime)
{
    {
        ZoneScopedN("CPhysXManager_simulate And fetchResults");
        m_pScene->simulate(fixedDeltaTime);
        m_pScene->fetchResults(true);
    }

    {
        ZoneScopedN("CPhysXManager_SyncPhysicsToComponents");
        SyncPhysicsToComponents();
    }
}

void CPhysXManager::SyncPhysicsToComponents()
{
    physx::PxU32 nbActiveActors = 0;
    physx::PxActor** activeActors = m_pScene->getActiveActors(nbActiveActors);

    for (physx::PxU32 i = 0; i < nbActiveActors; ++i)
    {
        auto actor = static_cast<physx::PxRigidActor*>(activeActors[i]);
        if (!actor)
        {
            continue;
        }

        auto pComponent = Cast<CComponent>(static_cast<CEngineBase*>(actor->userData));
        if (!pComponent)
        {
            continue;
        }
        auto pObj = pComponent->GetGameObject();
        if (!pObj)
        {
            continue;
        }

        {
            physx::PxTransform pose = actor->getGlobalPose();

            PHYSX_SYNC_DATA data{};
            data.vPos = { pose.p.x, pose.p.y, pose.p.z };
            data.vQuat = { pose.q.x , pose.q.y, pose.q.z, pose.q.w };
            pObj->SyncActivePhysXData(data);
        }
    }
}

UPtr<CPhysXManager> CPhysXManager::Create()
{
    auto pInstance = ToUPtr(new CPhysXManager{ });
    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX("Failed to Created : CPhysXManager");
        return nullptr;
    }
    return pInstance;
}

void CPhysXManager::Free()
{
    // 해제는 생성의 역순
    if (m_pScene) m_pScene->release();

    if (m_pPhysics) m_pPhysics->release();
    if (m_pCpuDispatcher) m_pCpuDispatcher->release();

    PxCloseExtensions();

    if (m_pPvd) {
        auto transport = m_pPvd->getTransport();
        m_pPvd->release();
        if (transport) transport->release();
    }

    if (m_pFoundation) m_pFoundation->release();

    CEngineBase::Free();
}
