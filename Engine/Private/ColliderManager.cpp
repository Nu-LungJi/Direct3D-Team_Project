#include "pch.h"
#include "ColliderManager.h"

#include "GameInstance.h"
#include "Collider.h"
#include "CollBox.h"
#include "CollOrientedBox.h"
#include "CollFrustum.h"
#include "CollSphere.h"

#include "Resources.h"

#include "CameraObject.h"

NS_USING(Engine)

CColliderManager::CColliderManager(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
    : m_pDevice{pDevice}
    , m_pContext{pContext}
{
}

CColliderManager::~CColliderManager()
{
}

void CColliderManager::UpdateGUI()
{
    ImGui::Begin("CColliderManager");
    
    {
        ImGui::Text("RenderEnable: %i", m_bRender);
        ImGui::SameLine();
        if (ImGui::Button("RenderEnable"))
        {
            m_bRender = !m_bRender;
        }
    }

    for (const auto& [key, coll] : m_Colliders)
    {
        auto [iter, inserted] = m_DbgRenders.try_emplace(key, true);

        ImGui::PushID(key.GetDbgStr());

        ImGui::Text("%s: %i", key.GetDbgStr(), iter->second);

        ImGui::SameLine();

        if (ImGui::Button("Render"))
        {
            iter->second = !iter->second;
        }

        ImGui::PopID();
    }

    ImGui::End();
}

void CColliderManager::FrameStart()
{
}

void CColliderManager::FrameEnd()
{
    ClearColliderGroup();
}

static const uint32_t edge[24] =
{
    0,1, 1,2, 2,3, 3,0,
    4,5, 5,6, 6,7, 7,4,
    0,4, 1,5, 2,6, 3,7
};

HRESULT CColliderManager::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
    if (!m_bRender)
    {
        return S_OK;
    }

    m_Vertices.clear();

    if (auto cam = E::CGameInstance::Get().GetActiveCamera())
    {
        CB_COLL_PER_FRAME cbPerFrame{};
        cbPerFrame.viewProjMatrix = cam->GetView() * cam->GetProj();

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        if (SUCCEEDED(m_pContext->Map(m_pPerFrame->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
            memcpy(mappedResource.pData, &cbPerFrame, sizeof(CB_COLL_PER_FRAME));
            m_pContext->Unmap(m_pPerFrame->GetCBuffer().Get(), 0);
        }
        //9
        m_pContext->VSSetConstantBuffers(9, 1, m_pPerFrame->GetCBuffer().GetAddressOf());
    }


    const auto& vs = m_pDbgVShader;
    const auto& ps = m_pDbgPShader;
    const auto& viBuffer = m_pDbgBuffer;

    m_pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
    m_pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

    ID3D11Buffer* vertexBuffers[] = {
        viBuffer->GetVertexBuffer().Get()
    };
    uint32_t strides[] = {
        viBuffer->GetVertexStride()
    };
    uint32_t offsets[] = {
        0
    };
    m_pContext->IASetInputLayout(vs->GetInputLayout().Get());
    m_pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
    //m_pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
    m_pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());


    for (const auto& [key, value] : m_Colliders)
    {
        {
            auto iter = m_DbgRenders.find(key);
            if (iter == m_DbgRenders.end())
            {
                continue;
            }
            else
            {
                if (!iter->second)
                {
                    continue;
                }
            }
        }


        for (auto pCollider : value)
        {
            switch (pCollider->GetCollType())
            {
            case CCollider::CollType::Box:
            {
                _float3 corners[BoundingBox::CORNER_COUNT];
                static_cast<const CCollBox*>(pCollider)->GetBoundingBox().GetCorners(corners);

                const auto& color = m_DbgColor[pCollider];

                for (int i = 0; i < 24; i += 2)
                {
                    if (m_Vertices.size() + 2 >= m_iVertexCnt)
                        break;

                    const auto& p0 = corners[edge[i]];
                    const auto& p1 = corners[edge[i + 1]];

                    m_Vertices.push_back({ p0, color });
                    m_Vertices.push_back({ p1, color });
                }
            }
            break;
            case CCollider::CollType::OrientedBox:
            {
                _float3 corners[BoundingOrientedBox::CORNER_COUNT];
                static_cast<const CCollOrientedBox*>(pCollider)->GetBoundingOrientedBox().GetCorners(corners);

                const auto& color = m_DbgColor[pCollider];
                for (int i = 0; i < 24; i += 2)
                {
                    if (m_Vertices.size() + 2 >= m_iVertexCnt)
                        break;

                    const auto& p0 = corners[edge[i]];
                    const auto& p1 = corners[edge[i + 1]];

                    m_Vertices.push_back({ p0, color });
                    m_Vertices.push_back({ p1, color });
                }
            }
            break;
            case CCollider::CollType::Frustum:
            {
                _float3 corners[BoundingFrustum::CORNER_COUNT];
                static_cast<const CCollFrustum*>(pCollider)->GetBoundingFrustum().GetCorners(corners);

                const auto& color = m_DbgColor[pCollider];
                for (int i = 0; i < 24; i += 2)
                {
                    if (m_Vertices.size() + 2 >= m_iVertexCnt)
                        break;

                    const auto& p0 = corners[edge[i]];
                    const auto& p1 = corners[edge[i + 1]];

                    m_Vertices.push_back({ p0, color });
                    m_Vertices.push_back({ p1, color });
                }
            }
            break;
            case CCollider::CollType::Sphere:
            {
                _float3 corners[CCollSphere::CornerCnt];
                static_cast<const CCollSphere*>(pCollider)->GetCorners(corners);
                const auto& color = m_DbgColor[pCollider];
                for (size_t i = 0; i < CCollSphere::CornerCnt; ++i)
                {
                    if (m_Vertices.size() + 1 >= m_iVertexCnt)
                        break;

                    m_Vertices.push_back({ corners[i],color });
                }
            }
            break;
            }
        }
    }


    {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        if (SUCCEEDED(m_pContext->Map(viBuffer->GetVertexBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
            memcpy(mappedResource.pData, m_Vertices.data(), sizeof(VTX_COL) * m_Vertices.size());
            m_pContext->Unmap(viBuffer->GetVertexBuffer().Get(), 0);
        }
    }

    m_pContext->Draw((uint32_t)m_Vertices.size(), 0);
    return S_OK;
}

void CColliderManager::AddColliderGroup(const StringID& groupTag, const CCollider* pCollider)
{
    if (m_bRender)
    {
        m_DbgColor[pCollider] = pCollider->GetOriginalColor();
    }

    auto iter = m_Colliders.find(groupTag);
    if (iter == m_Colliders.end())
    {
        std::vector< const CCollider*> newVec{};
        newVec.push_back(pCollider);
        m_Colliders.emplace(groupTag, newVec);
    }
    else
    {
        iter->second.push_back(pCollider);
    }
}

const std::vector<const CCollider*>* CColliderManager::GetColliderGroup(const StringID& groupTag) const
{
    auto iter = m_Colliders.find(groupTag);
    if (iter == m_Colliders.end())
    {
        return nullptr;
    }

    return &iter->second;
}

const CCollider* CColliderManager::GetColliderGroupFirst(const StringID& groupTag) const
{
    auto iter = m_Colliders.find(groupTag);
    if (iter == m_Colliders.end())
    {
        return nullptr;
    }

    if (iter->second.empty())
    {
        return nullptr;
    }

    return iter->second[0];
}

_bool CColliderManager::IntersectColl(const CCollider* pColl1, const CCollider* pColl2)
{
    if (pColl1->Intersect(*pColl2))
    {
        if (m_bRender)
        {
            m_DbgColor[pColl1] = pColl1->GetIntersectColor();
            m_DbgColor[pColl2] = pColl2->GetIntersectColor();
        }

        return true;
    }

    return false;
}

void CColliderManager::ClearColliderGroup()
{
    m_DbgColor.clear();
    m_Colliders.clear();
}

HRESULT CColliderManager::Initialize()
{
    if (auto res = E::CGameInstance::Get()
        .AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_ColliderDbg", "./Resources/Engine/Shader/Collider/Collider.hlsl"))
    {
        if (FAILED(res->Load()))
        {
            return E_FAIL;
        }
        m_pDbgVShader = res;
    }
    if (auto res = E::CGameInstance::Get()
        .AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_ColliderDbg", "./Resources/Engine/Shader/Collider/Collider.hlsl"))
    {
        if (FAILED(res->Load()))
        {
            return E_FAIL;
        }
        m_pDbgPShader = res;
    }
    if (auto res = E::CGameInstance::Get()
        .AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "CB_ColliderDbgPerFrame", E::CResCBuffer::Create()))
    {
        if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_COLL_PER_FRAME) })))
        {
            return E_FAIL;
        }
        m_pPerFrame = res;
    }
    if (auto res = CGameInstance::Get()
        .AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "DVI_ColliderDbg", CResDynamicVIBuffer::Create()))
    {
        CResDynamicVIBuffer::DESC desc{};
        desc.ePrimitiveType = D3D_PRIMITIVE_TOPOLOGY_LINELIST;

        desc.iVertexStride = sizeof(VTX_COL);
        desc.iNumVertices = m_iVertexCnt;
        desc.vertexDesc = {
            .ByteWidth = desc.iVertexStride * desc.iNumVertices,
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_VERTEX_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        };

        if (FAILED(res->Load(desc)))
        {
            return E_FAIL;
        }

        m_pDbgBuffer = res;
    }
    m_Vertices.reserve(m_iVertexCnt);
    return S_OK;
}

UPtr<CColliderManager> CColliderManager::Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
{
    auto pInstance = ToUPtr(new CColliderManager{ pDevice, pContext });

    if (FAILED(pInstance->Initialize()))
    {
        return nullptr;
    }

    return pInstance;
}
