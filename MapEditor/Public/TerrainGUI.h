#pragma once

#include "GUIWindow.h"

NS_BEGIN(Client)

class CTerrainPickingPass;
class CTerrainBrushController;

class CTerrainGUI final : public CGUIWindow
{
public:
	DECLARE_DERIVED_TYPE(CTerrainGUI, CGUIWindow)

private:
	CTerrainGUI() = default;
	~CTerrainGUI() override = default;

public:
	void UpdateGUI(E::_float fTimeDelta) override;
	bool IsSculptEnabled() const { return m_bSculptEnabled || m_bTexturePaintEnabled; }
	static E::UPtr<CTerrainGUI> Create(E::CHandle* selectedObject);

private:
	E::UPtr<CTerrainPickingPass> m_pPickingPass{};
	E::UPtr<CTerrainBrushController> m_pBrushController{};
	std::optional<E::_float3> m_PickedPosition{};
	bool m_bPickingDebug = false;
	bool m_bSculptEnabled = false;
	bool m_bTexturePaintEnabled = false;
};

NS_END
