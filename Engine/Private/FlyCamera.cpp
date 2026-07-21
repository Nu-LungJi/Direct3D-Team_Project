#include "pch.h"

#include "FlyCamera.h"
#include "GameInstance.h"
#include "CollFrustum.h"

NS_USING(Engine)

CFlyCamera::CFlyCamera()
{
}

CFlyCamera::CFlyCamera(const CFlyCamera& Prototype)
    : CCameraObject{ 
		Prototype }
{
}

CFlyCamera::~CFlyCamera()
{
}
void CFlyCamera::UpdateGUI()
{
    CCameraObject::UpdateGUI();

    ImGui::Text("intersect: %i", m_iColliderIntersect);
}

HRESULT CFlyCamera::Initialize(void* pArg)
{
    if (FAILED(CCameraObject::Initialize(pArg)))
    {
        return E_FAIL;
    }

    m_pCollider = CCollFrustum::Create(XMLoadFloat4x4(&m_matProj));

    return S_OK;
}

void CFlyCamera::PriorityUpdate(E::_float fTimeDelta)
{
	_float moveSpeed = 10.f;
	_float speedRatio = 1.f;

	if (CGameInstance::Get().KeyPressing(DIK_LSHIFT))
		speedRatio = 5.f;
	else
		speedRatio = 1.f;

    if (CGameInstance::Get().GetActiveCamera() == this)
    {
        if (CGameInstance::Get().KeyPressing(DIK_W))
        {
            GetTransform().GoStraight(fTimeDelta * moveSpeed * speedRatio);
        }

        if (CGameInstance::Get().KeyPressing(DIK_A))
        {
            GetTransform().GoLeft(fTimeDelta * moveSpeed * speedRatio);
        }

        if (CGameInstance::Get().KeyPressing(DIK_S))
        {
            GetTransform().GoBackward(fTimeDelta * moveSpeed * speedRatio);
        }

        if (CGameInstance::Get().KeyPressing(DIK_D))
        {
            GetTransform().GoRight(fTimeDelta * moveSpeed * speedRatio);
        }

        if (CGameInstance::Get().KeyPressing(DIK_Q))
        {
            GetTransform().GoUp(fTimeDelta * moveSpeed * speedRatio);
        }

        if (CGameInstance::Get().KeyPressing(DIK_E))
        {
            GetTransform().GoDown(fTimeDelta * moveSpeed * speedRatio);
        }

        if (CGameInstance::Get().GetMouseFix())
        {
            if (auto a = CGameInstance::Get().MouseMove(MOUSEMOVESTATE::X))
            {
                _vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
                GetTransform().AddRotation(vUp, fTimeDelta * 10.f * a);
            }


            //if (CGameInstance::Get().KeyPressing(DIK_I))
            //{
            //    GetTransform().AddRotation(vRight, fTimeDelta * -90.f);
            //}
            //if (CGameInstance::Get().KeyPressing(DIK_K))
            //{
            //    GetTransform().AddRotation(vRight, fTimeDelta * 90.f);
            //}



            //if (auto a = CGameInstance::Get().MouseMove(MOUSEMOVESTATE::Y))
            //{
            //    _float3 look;
            //    XMStoreFloat3(&look, GetTransform().GetState(STATE::LOOK));

            //   
            //    // 위쪽 제한
            //    if (look.y > 0.59f && a > 0)
            //        return;

            //    // 아래쪽 제한
            //    if (look.y < -0.59f && a < 0)
            //        return;

            //    _vector vRight = GetTransform().GetState(STATE::RIGHT);
            //    GetTransform().AddRotation(vRight, fTimeDelta * 10.f * a);
            //    
            //}

            if (auto a = CGameInstance::Get().MouseMove(MOUSEMOVESTATE::Y))
            {
                _float3 euler = GetTransform().GetRotationEuler();

                float delta = fTimeDelta * 10.f * a;
                float next = euler.x + delta;

                if (next <= 89.f && next >= -89.f)
                {
                    _vector vRight = GetTransform().GetState(STATE::RIGHT);
                    GetTransform().AddRotation(vRight, delta);
                }
            }
        }

    }


    

}

void CFlyCamera::Update(E::_float fTimeDelta)
{

}

void CFlyCamera::LateUpdate(E::_float fTimeDelta)
{
    


    m_pComTransform->Update();

    E::CGameInstance::Get().AddColliderGroup("Coll_FlyCamera", m_pCollider.get());

    m_pCollider->Transform(m_pComTransform->GetLoadedWorldMatrix());

    CCameraObject::UpdateViewMatrix();

    m_iColliderIntersect = 0;
    //if (auto colliders = E::CGameInstance::Get().GetColliders())
    //{
    //    for (const auto& [key, value] : *colliders)
    //    {
    //        for (const auto& p : value)
    //        {
    //            if (m_pCollider.get() == p)
    //            {
    //                continue;
    //            }

    //            if (m_pCollider->Intersect(*p))
    //            {
    //                ++m_iColliderIntersect;
    //            }
    //        }
    //    }
    //}


}

const CCollFrustum* CFlyCamera::GetFrustumCollider() const
{
    return static_cast<const CCollFrustum*>(m_pCollider.get());
}

Engine::UPtr<CFlyCamera> CFlyCamera::Create()
{
    auto pInstance = ToUPtr(new CFlyCamera{});
    if (FAILED(pInstance->InitializePrototype()))
    {
        MSG_BOX("Failed to Create: CFlyCamera");
        return nullptr;
    }

    return pInstance;
}

Engine::UPtr<CPrototype> CFlyCamera::Clone(void* pArg)
{
    auto pInstance = ToUPtr(new CFlyCamera{ *this });
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned: CFlyCamera");
        return nullptr;
    }

    return pInstance;
}
