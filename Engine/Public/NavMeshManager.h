#pragma once
#include "Engine_Base.h"

struct rcPolyMesh;
struct rcPolyMeshDetail;

NS_BEGIN(Engine)

struct NAVMESH_BUILD_DESC
{
	float cellSize = 0.3f;
	float cellHeight = 0.2f;

	float agentHeight = 2.0f;
	float agentRadius = 0.5f;
	float agentMaxClimb = 0.5f;
	float agentMaxSlope = 45.0f;

	float maxEdgeLen = 12.0f;
	float maxSimplificationError = 1.3f;
	int minRegionArea = 8;
	int mergeRegionArea = 20;
	int maxVertsPerPoly = 6;

	float detailSampleDist = 6.0f;
	float detailSampleMaxError = 1.0f;
};

class ENGINE_DLL CNavMeshManager : public CEngineBase
{
public:
	CNavMeshManager(const CNavMeshManager&) = delete;
	CNavMeshManager& operator=(const CNavMeshManager& rhs) = delete;

private:
	CNavMeshManager();
	~CNavMeshManager() override;

private:
	HRESULT Initialize();

public:
	_bool Build(const std::vector<_float3>& vertices, const std::vector<uint32_t>& indices, const NAVMESH_BUILD_DESC& desc);

	void Clear();
	void DrawDebug();

	void SetDebugDraw(_bool draw) { m_bDebugDraw = draw; }
	_bool IsDebugDraw() const { return m_bDebugDraw; }
	_bool IsBuilt() const { return m_pPolyMesh != nullptr; }

	//void Save();
	//void Load();

public:
	static UPtr<CNavMeshManager> Create();

private:
	rcPolyMesh* m_pPolyMesh = nullptr;
	rcPolyMeshDetail* m_pDetailMesh = nullptr;
	_bool m_bDebugDraw = true;
};

NS_END
