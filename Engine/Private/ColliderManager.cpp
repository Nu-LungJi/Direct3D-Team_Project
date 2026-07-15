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

namespace
{
	constexpr uint32_t edge[24] =
	{
		0,1, 1,2, 2,3, 3,0,
		4,5, 5,6, 6,7, 7,4,
		0,4, 1,5, 2,6, 3,7
	};
}

CColliderManager::CColliderManager()
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

	for (auto& [key, v] : m_DbgRenders)
	{
		ImGui::PushID(key.GetDbgStr());

		ImGui::Text("%s: %i", key.GetDbgStr(), v);

		ImGui::SameLine();

		if (ImGui::Button("Render"))
		{
			v = !v;
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

void CColliderManager::Update()
{
	if (!m_bRender)
	{
		return;
	}
	auto pLineRender = CGameInstance::Get().GetDbgLineRender();
	if (!pLineRender)
	{
		return ;
	}


	// 기존 색상 캐싱
	auto cachecol = pLineRender->GetColor();

	for (const auto& [key, val] : m_DbgRenders)
	{
		if (!val) continue;
		auto iter = m_Colliders.find(key);
		if (iter == m_Colliders.end())
		{
			continue;
		}
		
		for (auto pCollider : iter->second)
		{
			// 콜라이더별 색상 지정
			const auto& color = m_DbgColor[pCollider];
			pLineRender->SetColor(color);

			switch (pCollider->GetCollType())
			{
			case CollType::Box:
			{
				_float3 corners[BoundingBox::CORNER_COUNT];
				static_cast<const CCollBox*>(pCollider)->GetBoundingBox().GetCorners(corners);

				for (int i = 0; i < 24; i += 2)
				{
					pLineRender->AddLine(corners[edge[i]], corners[edge[i + 1]]);
				}
			}
			break;
			case CollType::OrientedBox:
			{
				_float3 corners[BoundingOrientedBox::CORNER_COUNT];
				static_cast<const CCollOrientedBox*>(pCollider)->GetBoundingOrientedBox().GetCorners(corners);

				for (int i = 0; i < 24; i += 2)
				{
					pLineRender->AddLine(corners[edge[i]], corners[edge[i + 1]]);
				}
			}
			break;
			case CollType::Frustum:
			{
				_float3 corners[BoundingFrustum::CORNER_COUNT];
				static_cast<const CCollFrustum*>(pCollider)->GetBoundingFrustum().GetCorners(corners);

				for (int i = 0; i < 24; i += 2)
				{
					pLineRender->AddLine(corners[edge[i]], corners[edge[i + 1]]);
				}
			}
			break;
			case CollType::Sphere:
			{
				_float3 corners[CCollSphere::CornerCnt];
				static_cast<const CCollSphere*>(pCollider)->GetCorners(corners);

				for (size_t i = 0; i < CCollSphere::CornerCnt; i += 2)
				{
					pLineRender->AddLine(corners[i], corners[i + 1]);
				}
			}
			break;
			}
		}
	}

	// 렌더링 완료 후 캐싱해둔 색상으로 복구
	pLineRender->SetColor(cachecol);
	for (const auto& [key, coll] : m_Colliders)
	{
		auto [iter, inserted] = m_DbgRenders.try_emplace(key, true);
	}
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
		std::vector<const CCollider*> newVec{};
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

	return S_OK;
}

UPtr<CColliderManager> CColliderManager::Create()
{
	auto pInstance = ToUPtr(new CColliderManager{  });

	if (FAILED(pInstance->Initialize()))
	{
		return nullptr;
	}

	return pInstance;
}
