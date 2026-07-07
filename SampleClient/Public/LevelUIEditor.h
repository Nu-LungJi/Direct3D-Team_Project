#pragma once

#include "Client_Defines.h"
#include "Level.h"
#include "Handle.h"

NS_BEGIN(Engine)
class CUIObject;
NS_END

NS_BEGIN(Client)

class CTexUI;
class CFlipBook;

class CLevelUIEditor final : public Engine::CLevel
{
public:
	struct JsonFileInfo {
		std::string fileName; 
		std::string fullPath; 
	};
public:
	enum class UiEditorMode
	{
		ARRANGE,
		PREFAB,
		FLIPBOOK,

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
	_float m_fX{}, m_fY{}, m_fSizeX{}, m_fSizeY{}, m_fAlpha{}, m_fRotation;
	int m_iWeight{};
	_float m_fCellSize{}, m_fDuration{};
	int m_iTotalFrame{};
	char m_cName[256] = "";
	char m_cLevelName[256] = "";
	char m_cPrefabName[256] = "";
	char m_cResTag[256] = "";
	std::vector<std::string> m_vResTag{};
	std::vector<std::string> m_vFlipBookResTag{};

	uint32_t count{};
	_float2 m_vDragOffset{};

	_float3 m_vColor{0.f, 0.f, 0.f};
	uint32_t m_iEffectType{};

private:
	void CreateMode();
	void SelectMode();

	void ArrangeMode();
	void PrefabMode();
	void FlipbookMode();

private:
	void Picking();
	void PickingOnlyRoot();

	void Save();
	void Load();

	void PrefabSave();
	void PrefabLoad();
	void FlipBookMake();

	void SaveUIRecursive(E::CUIObject* pUI, nlohmann::ordered_json& obj);
	E::CUIObject* LoadUIRecursive(const nlohmann::ordered_json& obj, E::CUIObject* parent);

	void StateView();
	void LocalStateView();
	void DrawFileExplorer();

	void DeleteUIRecursive(std::optional<CHandle> targetHandle);

	void RefreshJsonFileList();
	void DrawJsonFileLoader(uint32_t EditorMode);

private:
	std::vector<JsonFileInfo> g_JsonFiles;
	std::vector<JsonFileInfo> g_ImageFiles;
	bool g_IsFileGridInitialized = false; // 최초 1회 로드 체크용
	char g_BasePath[256] = "./Resources/SampleClient/UIData/LevelUI/";
public:
	static Engine::UPtr<CLevelUIEditor> Create();

private:
	void Free() override;
};

NS_END

