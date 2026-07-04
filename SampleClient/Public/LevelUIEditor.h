#pragma once

#include "Client_Defines.h"
#include "Level.h"
#include "Handle.h"

NS_BEGIN(Client)

class CUIObject;
class CTexUI;

class CLevelUIEditor final : public Engine::CLevel
{
public:
	enum class UiEditorMode
	{
		ARRANGE,
		PREFAB,

		END
	};
	enum class UiButtonMode
	{
		DEFAULT,
		CREATE,
		SELECT,

		END
	};
private:
	explicit CLevelUIEditor();
	~CLevelUIEditor() override;

public:
	HRESULT Initialize() override;
	void Update(E::_float fTimeDelta) override;
	HRESULT Render() override;
	void UpdateGUI() override;
	void FrameStart(E::_float fTimeDelta) override;

private:
	std::optional<CHandle> Target_UI{};
	std::optional<CHandle> m_oSelectHandle{};
	char m_sParentName[256] = "None";

	uint32_t m_iEditorMode;
	uint32_t m_iButtonMode;

private:
	_float m_fX{}, m_fY{}, m_fSizeX{}, m_fSizeY{}, m_fAlpha{};
	int m_iWeight{};
	char m_cName[256] = "";
	char m_cLevelName[256] = "";
	char m_cPrefabName[256] = "";
	std::vector<std::string> m_vResTag{};

	uint32_t count{};
	_float2 m_vDragOffset{};

private:
	void CreateMode();
	void SelectMode();

	void ArrangeMode();
	void PrefabMode();

private:
	void Picking();
	void PickingOnlyRoot();

	void Save();
	void Load();

	void PrefabSave();
	void PrefabLoad();

	void SaveUIRecursive(CTexUI* pUI, nlohmann::ordered_json& obj);
	CTexUI* LoadUIRecursive(const nlohmann::ordered_json& obj, CTexUI* parent);

	void StateView();
	void LocalStateView();

	void DeleteUIRecursive(std::optional<CHandle> targetHandle);
public:
	static Engine::UPtr<CLevelUIEditor> Create();

private:
	void Free() override;
};

NS_END

