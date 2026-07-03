#include "pch.h"
#include "ComCollider.h"
#include "Collider.h"
#include "CollBox.h"
#include "CollSphere.h"
#include "CollOrientedBox.h"
#include "CollFrustum.h"

NS_USING(Engine)

void CComCollider::UpdateGUI()
{
    CComponent::UpdateGUI();

    _string sCollName{};
    switch (m_pCollider->GetCollType())
    {
    case CollType::Box:
        sCollName = "Box";
        break;
    case CollType::OrientedBox:
        sCollName = "OrientedBox";
        break;
    case CollType::Sphere:
        sCollName = "Sphere";
        break;
    case CollType::Frustum:
        sCollName = "Frustum";
        break;
    }
    ImGui::Text("CollType : %s", sCollName.c_str());

    ImGui::Text("Local Transform");
    //if (ImGui::DragFloat3("Offset", &m_vLocalOffset.x, 0.05f))
    //{
    //    LocalTransform(XMMatrixTranslation(
    //        m_vLocalOffset.x,
    //        m_vLocalOffset.y,
    //        m_vLocalOffset.z));
    //}

    ImGui::Text("Collider Info");

    switch (m_pCollider->GetCollType())
    {
    case CollType::Box:
    {
        auto* pCol = static_cast<CCollBox*>(m_pCollider.get());
        const auto& box = pCol->GetLocalBoundingBox();

        XMFLOAT3 center = box.Center;
        XMFLOAT3 extents = box.Extents;

        bool bDirty = false;
        bDirty |= ImGui::DragFloat3("Center", &center.x, 0.05f);
        bDirty |= ImGui::DragFloat3("Extents", &extents.x, 0.05f, 0.f);

        if (bDirty)
            pCol->SetLocalBoundingBox(center, extents);

        break;
    }

    case CollType::OrientedBox:
    {
        auto* pCol = static_cast<CCollOrientedBox*>(m_pCollider.get());
        const auto& box = pCol->GetLocalBoundingOrientedBox();

        XMFLOAT3 center = box.Center;
        XMFLOAT3 extents = box.Extents;
        XMFLOAT4 orientation = box.Orientation;

        bool bDirty = false;
        bDirty |= ImGui::DragFloat3("Center", &center.x, 0.05f);
        bDirty |= ImGui::DragFloat3("Extents", &extents.x, 0.05f, 0.f);
        bDirty |= ImGui::DragFloat4("Orientation", &orientation.x, 0.01f);

        if (bDirty)
        {
            XMVECTOR q = XMQuaternionNormalize(XMLoadFloat4(&orientation));
            XMStoreFloat4(&orientation, q);

            pCol->SetLocalBoundingOrientedBox(center, extents, orientation);
        }

        break;
    }

    case CollType::Sphere:
    {
        auto* pCol = static_cast<CCollSphere*>(m_pCollider.get());
        const auto& sphere = pCol->GetLocalBoundingSphere();

        XMFLOAT3 center = sphere.Center;
        float radius = sphere.Radius;

        bool bDirty = false;
        bDirty |= ImGui::DragFloat3("Center", &center.x, 0.05f);
        bDirty |= ImGui::DragFloat("Radius", &radius, 0.05f, 0.f);

        if (bDirty)
            pCol->SetLocalBoundingSphere(center, radius);

        break;
    }

    case CollType::Frustum:
    {
        auto* pCol = static_cast<CCollFrustum*>(m_pCollider.get());
        const auto& frustum = pCol->GetLocalBoundingFrustum();

        XMFLOAT3 origin = frustum.Origin;
        XMFLOAT4 orientation = frustum.Orientation;

        float rightSlope = frustum.RightSlope;
        float leftSlope = frustum.LeftSlope;
        float topSlope = frustum.TopSlope;
        float bottomSlope = frustum.BottomSlope;
        float nearPlane = frustum.Near;
        float farPlane = frustum.Far;

        //bool bDirty = false;
        //bDirty |= ImGui::DragFloat3("Origin", &origin.x, 0.05f);
        //bDirty |= ImGui::DragFloat4("Orientation", &orientation.x, 0.01f);
        //bDirty |= ImGui::DragFloat("RightSlope", &rightSlope, 0.01f);
        //bDirty |= ImGui::DragFloat("LeftSlope", &leftSlope, 0.01f);
        //bDirty |= ImGui::DragFloat("TopSlope", &topSlope, 0.01f);
        //bDirty |= ImGui::DragFloat("BottomSlope", &bottomSlope, 0.01f);
        //bDirty |= ImGui::DragFloat("Near", &nearPlane, 0.05f, 0.f);
        //bDirty |= ImGui::DragFloat("Far", &farPlane, 0.05f, nearPlane);

        //if (bDirty)
        //{
        //    XMVECTOR q = XMQuaternionNormalize(XMLoadFloat4(&orientation));
        //    XMStoreFloat4(&orientation, q);

        //    pCol->SetLocalFrustum(
        //        origin,
        //        orientation,
        //        rightSlope,
        //        leftSlope,
        //        topSlope,
        //        bottomSlope,
        //        nearPlane,
        //        farPlane);
        //}

        break;
    }
    }
}

CComCollider::CComCollider()
{
}

CComCollider::~CComCollider()
{
}

HRESULT CComCollider::Initialize(void* pArg)
{
    auto* pDesc = static_cast<DESC*>(pArg);
    if (FAILED(CComponent::Initialize(pArg)))
    {
        return E_FAIL;
    }

    InitializeCollider(pDesc);



    
    return S_OK;
}

HRESULT CComCollider::InitializeCollider(const DESC* pDesc)
{
    switch (pDesc->eCollType)
    {
    case CollType::Box:
        m_pCollider = CCollBox::Create(pDesc->vCenter, pDesc->vExtents);
        break;
    case CollType::OrientedBox:
        m_pCollider = CCollOrientedBox::Create(pDesc->vCenter, pDesc->vExtents, pDesc->quatOritented);
        break;
    case CollType::Sphere:
        m_pCollider = CCollSphere::Create(pDesc->vCenter, pDesc->fRadius);
        break;
    case CollType::Frustum:
        m_pCollider = CCollFrustum::Create(XMLoadFloat4x4(&pDesc->matFrustum));
        break;
    }

    

    m_pCollider->SetInnerPointer(GetGameObject());
    m_pCollider->SetInnerHint(pDesc->CollCastHint);
    return S_OK;
}

//void CComCollider::LocalTransform(_fmatrix mat)
//{
//    m_pCollider->LocalTransform(mat);
//}

void CComCollider::Transform(_fmatrix mat)
{
    m_pCollider->Transform(mat);
}

UPtr<CComCollider> CComCollider::Create()
{
    auto pInstance = ToUPtr(new CComCollider{});
    if (FAILED(pInstance->InitializePrototype()))
    {
        MSG_BOX("Failed to Created : CComCollider");
        return nullptr;
    }
    return pInstance;
}

UPtr<CPrototype> CComCollider::Clone(void* pArg)
{
    auto pInstance = ToUPtr(new CComCollider{ *this });
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned: CComCollider");
        return nullptr;
    }
    return pInstance;
}