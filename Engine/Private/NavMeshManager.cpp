#include "pch.h"
#include "NavMeshManager.h"
#include "GameInstance.h"
#include "DbgLineRender.h"

#include <filesystem>
#include <fstream>
#include <recastnavigation/Recast.h>
#include <recastnavigation/DetourNavMesh.h>
#include <recastnavigation/DetourNavMeshBuilder.h>
#include <recastnavigation/DetourNavMeshQuery.h>

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

	for (int i = 0; i < pmesh->npolys; ++i)
	{
		if (pmesh->areas[i] == RC_WALKABLE_AREA)
		{
			pmesh->areas[i] = 0;
			pmesh->flags[i] = 1;
		}
	}

	dtNavMeshCreateParams params{};
	params.verts = pmesh->verts;
	params.vertCount = pmesh->nverts;
	params.polys = pmesh->polys;
	params.polyAreas = pmesh->areas;
	params.polyFlags = pmesh->flags;
	params.polyCount = pmesh->npolys;
	params.nvp = pmesh->nvp;
	params.detailMeshes = dmesh->meshes;
	params.detailVerts = dmesh->verts;
	params.detailVertsCount = dmesh->nverts;
	params.detailTris = dmesh->tris;
	params.detailTriCount = dmesh->ntris;
	params.walkableHeight = desc.agentHeight;
	params.walkableRadius = desc.agentRadius;
	params.walkableClimb = desc.agentMaxClimb;
	rcVcopy(params.bmin, pmesh->bmin);
	rcVcopy(params.bmax, pmesh->bmax);
	params.cs = cfg.cs;
	params.ch = cfg.ch;
	params.buildBvTree = true;

	unsigned char* navData = nullptr;
	int navDataSize = 0;
	if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
	{
		rcFreePolyMeshDetail(dmesh);
		rcFreePolyMesh(pmesh);
		rcFreeContourSet(cset);
		rcFreeCompactHeightfield(chf);
		return false;
	}

	dtNavMesh* detourNavMesh = dtAllocNavMesh();
	if (!detourNavMesh)
	{
		dtFree(navData);
		rcFreePolyMeshDetail(dmesh);
		rcFreePolyMesh(pmesh);
		rcFreeContourSet(cset);
		rcFreeCompactHeightfield(chf);
		return false;
	}

	if (dtStatusFailed(detourNavMesh->init(navData, navDataSize, DT_TILE_FREE_DATA)))
	{
		dtFreeNavMesh(detourNavMesh);
		rcFreePolyMeshDetail(dmesh);
		rcFreePolyMesh(pmesh);
		rcFreeContourSet(cset);
		rcFreeCompactHeightfield(chf);
		return false;
	}

	dtNavMeshQuery* navMeshQuery = dtAllocNavMeshQuery();
	if (!navMeshQuery)
	{
		dtFreeNavMesh(detourNavMesh);
		rcFreePolyMeshDetail(dmesh);
		rcFreePolyMesh(pmesh);
		rcFreeContourSet(cset);
		rcFreeCompactHeightfield(chf);
		return false;
	}

	if (dtStatusFailed(navMeshQuery->init(detourNavMesh, 2048)))
	{
		dtFreeNavMeshQuery(navMeshQuery);
		dtFreeNavMesh(detourNavMesh);
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
	m_pDetourNavMesh = detourNavMesh;
	m_pNavMeshQuery = navMeshQuery;
	m_PathTestPoints.clear();

	return true;
}

void CNavMeshManager::Clear()
{
	if (m_pNavMeshQuery)
	{
		dtFreeNavMeshQuery(m_pNavMeshQuery);
		m_pNavMeshQuery = nullptr;
	}

	if (m_pDetourNavMesh)
	{
		dtFreeNavMesh(m_pDetourNavMesh);
		m_pDetourNavMesh = nullptr;
	}

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

	m_PathTestPoints.clear();
}

void CNavMeshManager::SetPathTestStart(const _float3& position)
{
	m_PathTestStart = position;
	m_bHasPathTestStart = true;
	m_PathTestPoints.clear();
}

void CNavMeshManager::SetPathTestEnd(const _float3& position)
{
	m_PathTestEnd = position;
	m_bHasPathTestEnd = true;
	m_PathTestPoints.clear();
}

void CNavMeshManager::ClearPathTest()
{
	m_bHasPathTestStart = false;
	m_bHasPathTestEnd = false;
	m_PathTestPoints.clear();
}

_bool CNavMeshManager::BuildPathTest()
{
	m_PathTestPoints.clear();

	if (!m_bHasPathTestStart || !m_bHasPathTestEnd)
	{
		return false;
	}

	return FindPath(m_PathTestStart, m_PathTestEnd, m_PathTestPoints);
}

_bool CNavMeshManager::FindPath(const _float3& start, const _float3& end, std::vector<_float3>& outPath) const
{
	outPath.clear();

	if (!m_pNavMeshQuery)
	{
		return false;
	}

	const float startPos[3] = { start.x, start.y, start.z };
	const float endPos[3] = { end.x, end.y, end.z };
	const float halfExtents[3] = { 2.0f, 4.0f, 2.0f };

	dtQueryFilter filter{};
	filter.setIncludeFlags(0xffff);
	filter.setExcludeFlags(0);

	dtPolyRef startRef = 0;
	dtPolyRef endRef = 0;
	float nearestStart[3]{};
	float nearestEnd[3]{};

	if (dtStatusFailed(m_pNavMeshQuery->findNearestPoly(startPos, halfExtents, &filter, &startRef, nearestStart)) || startRef == 0)
	{
		return false;
	}

	if (dtStatusFailed(m_pNavMeshQuery->findNearestPoly(endPos, halfExtents, &filter, &endRef, nearestEnd)) || endRef == 0)
	{
		return false;
	}

	constexpr int MaxPolys = 256;
	dtPolyRef polys[MaxPolys]{};
	int polyCount = 0;

	if (dtStatusFailed(m_pNavMeshQuery->findPath(startRef, endRef, nearestStart, nearestEnd, &filter, polys, &polyCount, MaxPolys)) || polyCount == 0)
	{
		return false;
	}

	constexpr int MaxStraightPath = 256;
	float straightPath[MaxStraightPath * 3]{};
	unsigned char straightPathFlags[MaxStraightPath]{};
	dtPolyRef straightPathRefs[MaxStraightPath]{};
	int straightPathCount = 0;

	if (dtStatusFailed(m_pNavMeshQuery->findStraightPath(
		nearestStart,
		nearestEnd,
		polys,
		polyCount,
		straightPath,
		straightPathFlags,
		straightPathRefs,
		&straightPathCount,
		MaxStraightPath)) ||
		straightPathCount == 0)
	{
		return false;
	}

	outPath.reserve(static_cast<size_t>(straightPathCount));
	for (int i = 0; i < straightPathCount; ++i)
	{
		const float* pos = &straightPath[i * 3];
		outPath.push_back({ pos[0], pos[1] + 0.18f, pos[2] });
	}

	return outPath.size() >= 2;
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

void CNavMeshManager::DrawPathTest()
{
	auto* dbg = CGameInstance::Get().GetDbgLineRender();
	if (!dbg)
	{
		return;
	}

	if (m_bHasPathTestStart)
	{
		dbg->SetColor({ 0.0f, 1.0f, 0.15f, 1.0f });
		dbg->AddSphere(0.35f, XMMatrixTranslation(m_PathTestStart.x, m_PathTestStart.y + 0.35f, m_PathTestStart.z));
	}

	if (m_bHasPathTestEnd)
	{
		dbg->SetColor({ 1.0f, 0.25f, 0.1f, 1.0f });
		dbg->AddSphere(0.35f, XMMatrixTranslation(m_PathTestEnd.x, m_PathTestEnd.y + 0.35f, m_PathTestEnd.z));
	}

	if (m_PathTestPoints.size() < 2)
	{
		return;
	}

	dbg->SetColor({ 1.0f, 0.9f, 0.0f, 1.0f });
	for (size_t i = 1; i < m_PathTestPoints.size(); ++i)
	{
		dbg->AddLine(m_PathTestPoints[i - 1], m_PathTestPoints[i]);
	}
}

void CNavMeshManager::DrawDebug()
{
	if (CGameInstance::Get().KeyPressing(DIK_LCONTROL) && CGameInstance::Get().KeyPressing(DIK_LSHIFT)
		&& CGameInstance::Get().KeyDown(DIK_A))
		m_bDebugDraw = !m_bDebugDraw;

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

	DrawPathTest();
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
