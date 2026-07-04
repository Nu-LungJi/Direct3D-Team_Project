#include "pch.h"

#include "ComTransform.h"
#include "GameInstance.h"

NS_USING(Engine)

void CComTransform::UpdateGUI()
{
    if (!ImGui::IsAnyItemActive())
    {
        //UpdateEulerFromQuat();
    }

    if (ImGui::DragFloat3("Pos", (float*)&m_vPos, 0.1f))
    {
        m_bDirty = true;
    }

    if (ImGui::DragFloat3("Scale", (float*)&m_vScale, 0.1f))
    {
        m_bDirty = true;
    }
    //_float3 look;
    //XMStoreFloat3(&look, GetState(STATE::LOOK));
    //ImGui::InputFloat4("look", (float*)&look, "%.3f", ImGuiInputTextFlags_ReadOnly);

    if (ImGui::DragFloat3("Euler", (float*)&m_vEuler, 0.1f))
    {
        SetRotationEuler(m_vEuler);
        m_bDirty = true;
    }

    ImGui::InputFloat4("Quat", (float*)&m_vQuat, "%.3f", ImGuiInputTextFlags_ReadOnly);
}

CComTransform::CComTransform()
{
}

//CComTransform::CComTransform(const CComTransform& rhs)
//    : CComponent{ rhs }
//{
//}

CComTransform::~CComTransform()
{
}

_bool CComTransform::Update()
{
    _bool bIsUpdated = false;
    if (m_bDirty)
    {
        _matrix matScale = XMMatrixScalingFromVector(XMLoadFloat3(&m_vScale));

        _matrix matRotation = XMMatrixRotationQuaternion(XMLoadFloat4(&m_vQuat));

        _matrix matTranslation = XMMatrixTranslationFromVector(XMLoadFloat3(&m_vPos));

        _matrix matWorld = matScale * matRotation * matTranslation;

        //if (m_pGameObject)
        //{
        //        if (auto* pParentObj = m_pGameObject->GetParentNode())
        //        {
        //            _matrix matParent = XMLoadFloat4x4(pParentObj->GetTransform().GetWorldMatrix());

        //            if (m_bParentScaleNormalize)
        //            {
        //                matParent.r[0] = XMVector3Normalize(matParent.r[0]);
        //                matParent.r[1] = XMVector3Normalize(matParent.r[1]);
        //                matParent.r[2] = XMVector3Normalize(matParent.r[2]);
        //            }

        //            matWorld *= matParent;
        //        }
        //    
        //       
        //    for (const auto& pChildObj : m_pGameObject->GetChildrenNode())
        //    {
        //            pChildObj->GetTransform().SetDirty(true);
        //    }
        //}

        XMStoreFloat4x4(&m_WorldMatrix, matWorld);

        if (m_ParentWorldMatrix)
        {
            XMStoreFloat4x4(&m_CombinedWorldMatrix, matWorld * XMLoadFloat4x4(&m_ParentWorldMatrix.value()));
        }
        else
        {
            XMStoreFloat4x4(&m_CombinedWorldMatrix, matWorld);
        }

        UpdateEulerFromQuat();

        m_bDirty = false;

        bIsUpdated = true;
    }

    return bIsUpdated;
}

void CComTransform::GoStraight(_float fDist)
{
    SetPosition(GetLoadedPostion() + GetState(STATE::LOOK) * fDist);
}

void CComTransform::GoBackward(_float fDist)
{
    SetPosition(GetLoadedPostion() - GetState(STATE::LOOK) * fDist);
}

void CComTransform::GoLeft(_float fDist)
{
    SetPosition(GetLoadedPostion() - GetState(STATE::RIGHT) * fDist);
}

void CComTransform::GoRight(_float fDist)
{
    SetPosition(GetLoadedPostion() + GetState(STATE::RIGHT) * fDist);
}

void CComTransform::GoUp(_float fDist)
{
    SetPosition(GetLoadedPostion() + GetState(STATE::UP) * fDist);
}

void CComTransform::GoDown(_float fDist)
{
    SetPosition(GetLoadedPostion() - GetState(STATE::UP) * fDist);
}

void CComTransform::LookAt(_fvector vAt, _fvector vWorldUp)
{
    //_vector vLook = vAt - GetState(STATE::POSITION);
    //_vector vRight = XMVector3Cross(XMVectorSet(0.f, 1.f, 0.f, 0.f), vLook);
    //_vector vUp = XMVector3Cross(vLook, vRight);

    _vector vLook = XMVector3Normalize(vAt - GetState(STATE::POSITION));

    _vector vRight = XMVector3Normalize(
        XMVector3Cross(vWorldUp, vLook));

    _vector vUp = XMVector3Cross(vLook, vRight);

    _float3 scale = GetScale();
    //SetState(STATE::RIGHT, XMVector3Normalize(vRight) * scale.x);
    //SetState(STATE::UP, XMVector3Normalize(vUp) * scale.y);
    //SetState(STATE::LOOK, XMVector3Normalize(vLook) * scale.z);

    _matrix matRot = XMMatrixIdentity();
    matRot.r[0] = vRight;
    matRot.r[1] = vUp;
    matRot.r[2] = vLook;

    XMStoreFloat4(&m_vQuat, XMQuaternionRotationMatrix(matRot));

    m_bDirty = true;
}

void CComTransform::Chase(_fvector vGoal, _float fDist, _float fLimit)
{
    auto vDir = vGoal - GetLoadedPostion();
    //auto fLength = XMVectorGetX(XMVector3Length(vDir));
    //if (fLimit < fLength)
    //{
    //    SetPosition(GetPosition() + XMVector3Normalize(vDir) * fDist);
    //}

    _float fLengthSq = XMVectorGetX(XMVector3LengthSq(vDir));
    if (fLengthSq > fLimit * fLimit) // 리미트도 제곱해서 비교
    {
        SetPosition(GetLoadedPostion() + XMVector3Normalize(vDir) * fDist);
    }
}

UPtr<CComTransform> CComTransform::Create()
{
    auto pInstance = ToUPtr(new CComTransform{});
    if (FAILED(pInstance->InitializePrototype()))
    {
        MSG_BOX("Failed to Created : CComTransform");
        return nullptr;
    }
    return pInstance;
}

UPtr<CPrototype> CComTransform::Clone(void* pArg)
{
    auto pInstance = ToUPtr(new CComTransform{ *this });
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned: CComTransform");
        return nullptr;
    }
    return pInstance;
}
