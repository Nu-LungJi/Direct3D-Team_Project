#pragma once
#include "GUIWindow.h"

NS_BEGIN(Client)

class CMapChunkGUI : public CGUIWindow
{
public:
	DECLARE_DERIVED_TYPE(CMapChunkGUI, CGUIWindow)

private:
	CMapChunkGUI();
	~CMapChunkGUI() override;

public:
	void UpdateGUI(E::_float fTimeDelta) override;

public:
	static E::UPtr<CMapChunkGUI> Create(E::CHandle* pSelectedObject);

private:
	E::MAPCHUNK_COORD m_SelectedCoord{};
	int64_t m_ViewY = 0;
	_bool m_bHasSelection = false;
	_bool m_bAutoRebuild = false;
	_bool m_bFilterY = false;
	_bool m_bDebugDrawChunk = false;

private:
};

NS_END
