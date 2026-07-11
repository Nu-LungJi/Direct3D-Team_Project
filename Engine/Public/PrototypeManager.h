#pragma once
#include "Prototype.h"

NS_BEGIN(Engine)

class CPrototypeManager final : public CEngineBase
{
public:
	typedef std::unordered_map<StringID, UPtr<CPrototype>> PROTOTYPES;

private:
	explicit CPrototypeManager(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CPrototypeManager() override;

public:
	void UpdateGUI();

public:
	HRESULT Initialize();
	HRESULT AddPrototype(const StringID& svGroupTag, const StringID& svPrototypeTag, UPtr<CPrototype> pPrototype);

	UPtr<CPrototype> ClonePrototype(const StringID& svGroupTag, const StringID& svPrototypeTag, void* pArg);
	void DelPrototype(const StringID& sGroupTag);
	const PROTOTYPES* GetPrototype(const StringID& svGroupTag) const;
private:
	std::unordered_map<StringID, PROTOTYPES> m_pPrototypes{};

private:
	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};

private:
	PROTOTYPES* Find_Group(const StringID& svGroupTag);
	CPrototype* Find_Prototype(const StringID& svGroupTag, const StringID& svPrototypeTag);

public:
	static UPtr<CPrototypeManager> Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
};

NS_END
