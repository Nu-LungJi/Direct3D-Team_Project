#include "pch.h"
#include "NavMeshManager.h"
#include "GameInstance.h"
#include "DbgLineRender.h"

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
