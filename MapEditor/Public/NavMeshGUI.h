#pragma once
#include "GUIWindow.h"
#include "NavMeshManager.h"

NS_BEGIN(Engine)
class CTerrain;
NS_END

NS_BEGIN(Client)

class CMapNaviPosPickPass;
class CNavMeshGUI : public CGUIWindow
{
public:
	DECLARE_DERIVED_TYPE(CNavMeshGUI, CGUIWindow)

private:
	CNavMeshGUI();
	~CNavMeshGUI() override;

public:
	void UpdateGUI(E::_float fTimeDelta) override;
	void SaveNavMesh(const std::string& mapPath);
	void LoadNavMesh(const std::string& mapPath);

public:
	static E::UPtr<CNavMeshGUI> Create(E::CHandle* pSelectedObject);
	bool IsMouseEditing() const {return m_bPathPickWithMouse || m_bPaintWithMouse || m_bManualTrianglePickWithMouse;}
private:
	bool BuildNavMeshFromTerrain(E::CTerrain& terrain, E::CNavMeshManager& navMeshManager);
	bool BuildManualNavMesh(E::CNavMeshManager& navMeshManager);
	E::CTerrain* FindFirstTerrain();
private:
	E::NAVMESH_BUILD_DESC m_NavDesc{};
	bool m_bDebugDrawNavMesh = true;
	bool m_bBuildTried = false;
	bool m_bBuildSucceeded = false;
	int m_iEditTriangleIndex = 0;
	int m_iPaintMode = 0;
	bool m_bPaintWithMouse = false;
	float m_fBrushRadius = 1.0f;
	bool m_bPickSucceeded = false;
	uint32_t m_iPickedTriangleIndex = 0;
	bool m_bPathPickWithMouse = false;
	int m_iPathPickTarget = 0;
	bool m_bPathFindTried = false;
	bool m_bPathFindSucceeded = false;

	// 수동 네비메시 추가용
	UPtr<CMapNaviPosPickPass> m_pMapNaviPosPickPass{};
	_bool		m_bManualTrianglePickWithMouse = false;
	uint32_t	m_iManualPickCount = 0;
	_float		m_fManualVertexSnapDistance = 3.f;
	_float3		m_vManualPickPoints[3]{};
};

NS_END
