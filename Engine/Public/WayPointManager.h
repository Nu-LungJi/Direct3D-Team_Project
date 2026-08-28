#pragma once
#include "Engine_Defines.h"
#include "Engine_Base.h"
typedef struct strWayNames
{
	_string MajorName{};
	_string MinorName{};
}WAY_NAME;

NS_BEGIN(Engine)
class ENGINE_DLL CWayPointManager final : public CEngineBase
{

private:
	CWayPointManager();
	CWayPointManager& operator=(const CWayPointManager& rhs) = delete;
	~CWayPointManager();

public:
	void UpdateGUI();
	void RegistWayTag(const _string& WayName, const _string& MajorName, const _string& MinorName);
	void LoadWay(const _string& WayName, std::vector<_float3>& OutName);

public:
	void Update(_float fTimeDelta);
	HRESULT Render();
private:
	const WAY_NAME FInd_Way(const _string& WayName);
	void Way_Debug();
	void WayPoint(_float3& vPos, uint32_t iID);
private:
	_bool m_bPopup{}, m_bPopupL{};
	std::vector<_float3>	m_WayPoint{};
	_string				m_WayName{};

	std::map<_string, WAY_NAME> m_WayNames;
public:
	static UPtr<CWayPointManager> Create();
};


NS_END
