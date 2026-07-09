#pragma once
#include "Engine_Base.h"
#include <cstdint>
#include <unordered_map>

struct rcPolyMesh;
struct rcPolyMeshDetail;
class dtNavMesh;
class dtNavMeshQuery;

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

enum class ENavAreaType : uint8_t
{
	Walkable,
	Blocked,
	Reserved_Climbable,
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
	void DrawBlockedTriangles(const std::vector<_float3>& vertices, const std::vector<uint32_t>& indices);

	void SetDebugDraw(_bool draw) { m_bDebugDraw = draw; }
	_bool IsDebugDraw() const { return m_bDebugDraw; }
	_bool IsBuilt() const { return m_pPolyMesh != nullptr; }

	void SetPathTestStart(const _float3& position);
	void SetPathTestEnd(const _float3& position);
	void ClearPathTest();
	_bool BuildPathTest();
	_bool HasPathTestStart() const { return m_bHasPathTestStart; }
	_bool HasPathTestEnd() const { return m_bHasPathTestEnd; }
	uint32_t GetPathTestPointCount() const { return static_cast<uint32_t>(m_PathTestPoints.size()); }

	void SetTriangleArea(uint32_t triangleIndex, ENavAreaType areaType);
	ENavAreaType GetTriangleArea(uint32_t triangleIndex) const;
	void ClearTriangleAreas();
	uint32_t GetTriangleAreaCount(ENavAreaType areaType) const;

	void SetTriangleBlocked(uint32_t triangleIndex, _bool blocked);
	void ClearBlockedTriangles();
	_bool IsTriangleBlocked(uint32_t triangleIndex) const;
	uint32_t GetBlockedTriangleCount() const { return GetTriangleAreaCount(ENavAreaType::Blocked); }

	//void Save();
	//void Load();
	HRESULT Save(const std::string& path) const;
	HRESULT Load(const std::string& path);

public:
	static UPtr<CNavMeshManager> Create();

private:
	_bool FindPath(const _float3& start, const _float3& end, std::vector<_float3>& outPath) const;
	void DrawPathTest();

private:
	// NavMesh 빌드
	rcPolyMesh* m_pPolyMesh = nullptr;
	rcPolyMeshDetail* m_pDetailMesh = nullptr;

	// NavMesh 빌드결과 바이너리화된 결과, 이걸로 런타임에 길찾기
	dtNavMesh* m_pDetourNavMesh = nullptr;
	dtNavMeshQuery* m_pNavMeshQuery = nullptr;

	std::unordered_map<uint32_t, ENavAreaType> m_TriangleAreas{};

	// 길찾기 테스트
	_float3 m_PathTestStart{};
	_float3 m_PathTestEnd{};
	std::vector<_float3> m_PathTestPoints{};
	_bool m_bHasPathTestStart = false;
	_bool m_bHasPathTestEnd = false;
	_bool m_bDebugDraw = true;
};

NS_END
