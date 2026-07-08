#include "pch.h"
#include "NavMeshManager.h"
#include "GameInstance.h"
#include "DbgLineRender.h"

#include <filesystem>
#include <fstream>
#include <recastnavigation/Recast.h>

NS_USING(Engine)

CNavMeshManager::CNavMeshManager()
{

}

CNavMeshManager::~CNavMeshManager()
{
	Clear();
}


HRESULT CNavMeshManager::Initialize()
{
	return S_OK;
}
_bool CNavMeshManager::Build(
	const std::vector<_float3>& vertices,
	const std::vector<uint32_t>& indices,
	const NAVMESH_BUILD_DESC& desc)
{
	Clear();

	if (vertices.empty() || indices.size() < 3 || indices.size() % 3 != 0)
		return false;

	std::vector<float> verts;
	verts.reserve(vertices.size() * 3);

	for (const auto& v : vertices)
	{
		verts.push_back(v.x);
		verts.push_back(v.y);
		verts.push_back(v.z);
	}

	std::vector<int> tris;
	tris.reserve(indices.size());

	for (uint32_t index : indices)
		tris.push_back(static_cast<int>(index));

	const int vertCount = static_cast<int>(vertices.size());
	const int triCount = static_cast<int>(indices.size() / 3);

	rcContext ctx;
	rcConfig cfg{};
	cfg.cs = desc.cellSize;
	cfg.ch = desc.cellHeight;
	cfg.walkableSlopeAngle = desc.agentMaxSlope;
	cfg.walkableHeight = static_cast<int>(ceilf(desc.agentHeight / cfg.ch));
	cfg.walkableClimb = static_cast<int>(floorf(desc.agentMaxClimb / cfg.ch));
	cfg.walkableRadius = static_cast<int>(ceilf(desc.agentRadius / cfg.cs));
	cfg.maxEdgeLen = static_cast<int>(desc.maxEdgeLen / cfg.cs);
	cfg.maxSimplificationError = desc.maxSimplificationError;
	cfg.minRegionArea = rcSqr(desc.minRegionArea);
	cfg.mergeRegionArea = rcSqr(desc.mergeRegionArea);
	cfg.maxVertsPerPoly = desc.maxVertsPerPoly;
	cfg.detailSampleDist = desc.detailSampleDist < 0.9f ? 0.0f : cfg.cs * desc.detailSampleDist;
	cfg.detailSampleMaxError = cfg.ch * desc.detailSampleMaxError;

	rcCalcBounds(verts.data(), vertCount, cfg.bmin, cfg.bmax);
	rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);

	rcHeightfield* solid = rcAllocHeightfield();
	if (!solid)
		return false;

	if (!rcCreateHeightfield(&ctx, *solid, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch))
	{
		rcFreeHeightField(solid);
		return false;
	}

	std::vector<unsigned char> triAreas(triCount, RC_NULL_AREA);

	rcMarkWalkableTriangles(
		&ctx,
		cfg.walkableSlopeAngle,
		verts.data(),
		vertCount,
		tris.data(),
		triCount,
		triAreas.data());

	for (const auto& [triangleIndex, areaType] : m_TriangleAreas)
	{
		if (areaType == ENavAreaType::Blocked && triangleIndex < triAreas.size())
		{
			triAreas[triangleIndex] = RC_NULL_AREA;
		}
	}

	if (!rcRasterizeTriangles(
		&ctx,
		verts.data(),
		vertCount,
		tris.data(),
		triAreas.data(),
		triCount,
		*solid,
		cfg.walkableClimb))
	{
		rcFreeHeightField(solid);
		return false;
	}

	rcFilterLowHangingWalkableObstacles(&ctx, cfg.walkableClimb, *solid);
	rcFilterLedgeSpans(&ctx, cfg.walkableHeight, cfg.walkableClimb, *solid);
	rcFilterWalkableLowHeightSpans(&ctx, cfg.walkableHeight, *solid);

	rcCompactHeightfield* chf = rcAllocCompactHeightfield();
	if (!chf)
	{
		rcFreeHeightField(solid);
		return false;
	}

	if (!rcBuildCompactHeightfield(&ctx, cfg.walkableHeight, cfg.walkableClimb, *solid, *chf))
	{
		rcFreeCompactHeightfield(chf);
		rcFreeHeightField(solid);
		return false;
	}

	rcFreeHeightField(solid);

	if (!rcErodeWalkableArea(&ctx, cfg.walkableRadius, *chf))
	{
		rcFreeCompactHeightfield(chf);
		return false;
	}

	if (!rcBuildDistanceField(&ctx, *chf))
	{
		rcFreeCompactHeightfield(chf);
		return false;
	}

	if (!rcBuildRegions(&ctx, *chf, 0, cfg.minRegionArea, cfg.mergeRegionArea))
	{
		rcFreeCompactHeightfield(chf);
		return false;
	}

	rcContourSet* cset = rcAllocContourSet();
	if (!cset)
	{
		rcFreeCompactHeightfield(chf);
		return false;
	}

	if (!rcBuildContours(&ctx, *chf, cfg.maxSimplificationError, cfg.maxEdgeLen, *cset))
	{
		rcFreeContourSet(cset);
		rcFreeCompactHeightfield(chf);
		return false;
	}

	rcPolyMesh* pmesh = rcAllocPolyMesh();
	if (!pmesh)
	{
		rcFreeContourSet(cset);
		rcFreeCompactHeightfield(chf);
		return false;
	}

	if (!rcBuildPolyMesh(&ctx, *cset, cfg.maxVertsPerPoly, *pmesh))
	{
		rcFreePolyMesh(pmesh);
		rcFreeContourSet(cset);
		rcFreeCompactHeightfield(chf);
		return false;
	}

	rcPolyMeshDetail* dmesh = rcAllocPolyMeshDetail();
	if (!dmesh)
	{
		rcFreePolyMesh(pmesh);
		rcFreeContourSet(cset);
		rcFreeCompactHeightfield(chf);
		return false;
	}

	if (!rcBuildPolyMeshDetail(&ctx, *pmesh, *chf, cfg.detailSampleDist, cfg.detailSampleMaxError, *dmesh))
	{
		rcFreePolyMeshDetail(dmesh);
		rcFreePolyMesh(pmesh);
		rcFreeContourSet(cset);
		rcFreeCompactHeightfield(chf);
		return false;
	}

	rcFreeContourSet(cset);
	rcFreeCompactHeightfield(chf);

	m_pPolyMesh = pmesh;
	m_pDetailMesh = dmesh;

	return true;
}

void CNavMeshManager::Clear()
{
	if (m_pDetailMesh)
	{
		rcFreePolyMeshDetail(m_pDetailMesh);
		m_pDetailMesh = nullptr;
	}

	if (m_pPolyMesh)
	{
		rcFreePolyMesh(m_pPolyMesh);
		m_pPolyMesh = nullptr;
	}
}

void CNavMeshManager::SetTriangleArea(uint32_t triangleIndex, ENavAreaType areaType)
{
	if (areaType == ENavAreaType::Walkable)
	{
		m_TriangleAreas.erase(triangleIndex);
		return;
	}

	m_TriangleAreas[triangleIndex] = areaType;
}

ENavAreaType CNavMeshManager::GetTriangleArea(uint32_t triangleIndex) const
{
	const auto iter = m_TriangleAreas.find(triangleIndex);
	if (iter == m_TriangleAreas.end())
	{
		return ENavAreaType::Walkable;
	}

	return iter->second;
}

void CNavMeshManager::ClearTriangleAreas()
{
	m_TriangleAreas.clear();
}

uint32_t CNavMeshManager::GetTriangleAreaCount(ENavAreaType areaType) const
{
	uint32_t count = 0;
	for (const auto& [triangleIndex, triangleAreaType] : m_TriangleAreas)
	{
		if (triangleAreaType == areaType)
		{
			++count;
		}
	}

	return count;
}

void CNavMeshManager::SetTriangleBlocked(uint32_t triangleIndex, _bool blocked)
{
	SetTriangleArea(triangleIndex, blocked ? ENavAreaType::Blocked : ENavAreaType::Walkable);
}

void CNavMeshManager::ClearBlockedTriangles()
{
	for (auto iter = m_TriangleAreas.begin(); iter != m_TriangleAreas.end();)
	{
		if (iter->second == ENavAreaType::Blocked)
		{
			iter = m_TriangleAreas.erase(iter);
			continue;
		}

		++iter;
	}
}

_bool CNavMeshManager::IsTriangleBlocked(uint32_t triangleIndex) const
{
	return GetTriangleArea(triangleIndex) == ENavAreaType::Blocked;
}

void CNavMeshManager::DrawBlockedTriangles(const std::vector<_float3>& vertices, const std::vector<uint32_t>& indices)
{
	if (m_TriangleAreas.empty())
	{
		return;
	}

	auto* dbg = CGameInstance::Get().GetDbgLineRender();
	if (!dbg)
	{
		return;
	}

	const uint32_t triangleCount = static_cast<uint32_t>(indices.size() / 3);
	dbg->SetColor({ 1.f, 0.f, 0.f, 1.f });

	for (const auto& [triangleIndex, areaType] : m_TriangleAreas)
	{
		if (areaType != ENavAreaType::Blocked)
		{
			continue;
		}

		if (triangleIndex >= triangleCount)
		{
			continue;
		}

		const uint32_t i0 = indices[triangleIndex * 3 + 0];
		const uint32_t i1 = indices[triangleIndex * 3 + 1];
		const uint32_t i2 = indices[triangleIndex * 3 + 2];

		if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size())
		{
			continue;
		}

		_float3 p0 = vertices[i0];
		_float3 p1 = vertices[i1];
		_float3 p2 = vertices[i2];

		p0.y += 0.12f;
		p1.y += 0.12f;
		p2.y += 0.12f;

		dbg->AddTriangle(p0, p1, p2);
	}
}

void CNavMeshManager::DrawDebug()
{
	if (!m_bDebugDraw || !m_pPolyMesh)
		return;

	auto* dbg = CGameInstance::Get().GetDbgLineRender();
	if (!dbg)
		return;

	dbg->SetColor({ 0.0f, 0.85f, 1.0f, 1.0f });

	const int nvp = m_pPolyMesh->nvp;

	for (int i = 0; i < m_pPolyMesh->npolys; ++i)
	{
		const unsigned short* poly = &m_pPolyMesh->polys[i * nvp * 2];

		for (int j = 0; j < nvp; ++j)
		{
			if (poly[j] == RC_MESH_NULL_IDX)
				break;

			int next = j + 1;
			if (next >= nvp || poly[next] == RC_MESH_NULL_IDX)
				next = 0;

			const unsigned short* v0 = &m_pPolyMesh->verts[poly[j] * 3];
			const unsigned short* v1 = &m_pPolyMesh->verts[poly[next] * 3];

			_float3 p0{
				m_pPolyMesh->bmin[0] + v0[0] * m_pPolyMesh->cs,
				m_pPolyMesh->bmin[1] + v0[1] * m_pPolyMesh->ch + 0.08f,
				m_pPolyMesh->bmin[2] + v0[2] * m_pPolyMesh->cs
			};

			_float3 p1{
				m_pPolyMesh->bmin[0] + v1[0] * m_pPolyMesh->cs,
				m_pPolyMesh->bmin[1] + v1[1] * m_pPolyMesh->ch + 0.08f,
				m_pPolyMesh->bmin[2] + v1[2] * m_pPolyMesh->cs
			};

			dbg->AddLine(p0, p1);
		}
	}
}

HRESULT CNavMeshManager::Save(const std::string& path) const
{
	const std::filesystem::path filePath(path);
	std::error_code ec;
	std::filesystem::create_directories(filePath.parent_path(), ec);
	if (ec)
	{
		return E_FAIL;
	}

	nlohmann::ordered_json rootJson = {};
	rootJson["version"] = 1;
	rootJson["triangleAreas"] = nlohmann::ordered_json::array();

	for (const auto& [triangleIndex, areaType] : m_TriangleAreas)
	{
		if (areaType == ENavAreaType::Walkable)
		{
			continue;
		}

		rootJson["triangleAreas"].push_back(nlohmann::ordered_json
		{
			{"triangle", triangleIndex},
			{"area", static_cast<uint32_t>(areaType)}
		});
	}

	std::ofstream outFile(filePath.string());
	if (!outFile.is_open())
	{
		return E_FAIL;
	}

	outFile << rootJson.dump(4);
	outFile.close();

	return S_OK;
}

HRESULT CNavMeshManager::Load(const std::string& path)
{
	ClearTriangleAreas();

	const std::filesystem::path filePath(path);
	if (!std::filesystem::exists(filePath))
	{
		return S_OK;
	}

	std::ifstream inFile(filePath.string());
	if (!inFile.is_open())
	{
		return E_FAIL;
	}

	nlohmann::ordered_json rootJson;
	inFile >> rootJson;
	inFile.close();

	if (!rootJson.contains("triangleAreas"))
	{
		return S_OK;
	}

	for (const auto& areaJson : rootJson["triangleAreas"])
	{
		if (!areaJson.contains("triangle") || !areaJson.contains("area"))
		{
			continue;
		}

		const uint32_t triangleIndex = areaJson["triangle"].get<uint32_t>();
		const uint32_t areaValue = areaJson["area"].get<uint32_t>();
		if (areaValue > static_cast<uint32_t>(ENavAreaType::Reserved_Climbable))
		{
			continue;
		}

		SetTriangleArea(triangleIndex, static_cast<ENavAreaType>(areaValue));
	}

	return S_OK;
}

UPtr<CNavMeshManager> CNavMeshManager::Create()
{
	auto pInstance = ToUPtr(new CNavMeshManager{});
	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CNavMeshManager");
		return nullptr;
	}
	return pInstance;
}
